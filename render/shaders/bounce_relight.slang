// The relight's compute entries (ADR-0326 points 5 and 6), dispatched by
// render/src/bounce_relight.c between passes, after the begun target's capture
// passes have copied their pictures into its atlases.
//
// A PROBE'S PICTURE: probe index x + 24y + 288z (toroidal), at x 48x + 8f,
// y 8(12z + y) of the atlases, face f in point_shadow.slangh's order and texel
// orientation, so texel (u, v) of face f looks along the face's view
// (2u − 1, 1 − 2v, 1), u and v at the texel's centre over the face.
//
// settle: one workgroup of 64 per listed probe (bit 31 set: it holds a picture).
// - Validity, at the probe's coordinate: 1, or 0 when it holds no picture or
//   more than a quarter of its texels (96) see a back face, normal · direction
//   from the probe above nought: a probe inside something. No hit has a normal
//   of nought, so it is never a back face.
// - Moments, at the texel's own place: the mean and mean² of the distances
//   over its 3×3 within its face, a neighbour past the edge clamped to it.
//
// relight: one workgroup of 64 per probe of the grid, over its 384 texels, into
// level (chain, level) — grid chain(chain − 1)/2 + level − 1. Each texel weighs
// its solid angle; no hit (distance at the reach) or a back face is nought;
// else at the surface point (probe + direction × distance) radiance = albedo ×
// - level 1: the chain's lights, colour × intensity × bounce strength ×
//   max(0, n·l) × shadow: the sun with the first cascade whose box holds the
//   point, lit outside them; a lamp faded as lighting.slangh's falloff, and
//   with its slot's compare when the point shadows are ready;
// - level k > 1: bounce_read.slangh's E of level (chain, k − 1) / π;
// projected to L1 SH, (Y0, Y1 · direction), and reduced in group memory. An
// invalid probe writes nought.
// sum: one thread per probe, grid 6 = the sum of grids 0 to 5; a level not in
// use is nought, bounce_relight.c clears it.
//
// The push block matches struct relight_push in bounce_relight.c, the record at
// binding 11 its struct relight_record: the structs below repeat bindings.slangh's
// light, shadow and point light, which this file cannot include (its set 0).
//
// CONSTRAINTS. A workgroup of 64, under Vulkan's guaranteed 128. The grids are
// written with unrolled constant indices: the device enables no dynamic
// storage-image indexing. Nine loads a texel for its moments, and the level
// read's eight probes a texel, where group-shared memory would take fewer.

#include "point_shadow.slangh"
#include "bounce_read.slangh"

static const uint3 SIZE = uint3(24, 12, 24);
static const uint TOTAL = 24 * 12 * 24;
static const uint FACE = 8;
static const uint TEXELS = 6 * FACE * FACE;
static const uint GROUP = 64;
static const uint HOLDS = 0x80000000u;
static const uint LAMPS = 16;
static const uint LEVELS = 6;
static const float REACH = 24.0;
static const float PI = 3.14159265358979;
static const float SH0 = 0.282095;
static const float SH1 = 0.488603;

struct relight_push {
	uint first;
	uint count;
	uint chain;
	uint level;
	uint sun_chain;
	uint lamps;
	uint point_ready;
	uint reserved;
};

struct relight_light {
	float3 direction;
	float intensity;
	float3 colour;
	uint unshaded;
	float3 fill;
	float reserved;
};

struct relight_shadow {
	float4x4 cascades[4];
	float4 splits;
	float4 texels;
	uint count;
	uint reserved0;
	uint reserved1;
	uint reserved2;
};

struct relight_lamp {
	float3 position;
	float range;
	float3 colour;
	float falloff;
	uint shadow;
	float shadow_strength;
	uint bounces;
	float bounce_strength;
};

struct relight_record {
	relight_light sun;
	relight_shadow shadow;
	float3 corner;
	float sun_strength;
	uint3 cell;
	float spacing;
	relight_lamp lamps[LAMPS];
};

[[vk::binding(0, 0)]]
[format("rgba16f")]
RWTexture2D<float4> relight_normal;
[[vk::binding(1, 0)]]
[format("rg16f")]
RWTexture2D<float2> relight_moments;
[[vk::binding(2, 0)]]
[format("r16f")]
RWTexture3D<float> relight_validity;
[[vk::binding(3, 0)]]
StructuredBuffer<uint> relight_list;
[[vk::binding(4, 0)]]
[format("rgba16f")]
RWTexture3D<float4> relight_grids[21];
[[vk::binding(5, 0)]]
Sampler3D relight_levels[18];
[[vk::binding(6, 0)]]
Sampler3D relight_validity_read;
[[vk::binding(7, 0)]]
Sampler2D relight_moments_read;
[[vk::binding(8, 0)]]
Texture2D<float4> relight_albedo;
[[vk::binding(9, 0)]]
Sampler2DArrayShadow relight_sun_shadow;
[[vk::binding(10, 0)]]
Sampler2DArrayShadow relight_point_shadow;
[[vk::binding(11, 0)]]
ConstantBuffer<relight_record> relight_lights;

[[vk::push_constant]]
ConstantBuffer<relight_push> relight_current;

groupshared uint settle_back_faces;
groupshared float4 relight_sums[3][GROUP];

// The probe-relative direction of face `face`'s view `view`, the inverse of
// point_shadow.slangh's voe_render_point_shadow_basis.
float3 face_direction(float3 view, uint face)
{
	switch (face) {
	case 0: return float3(view.z, view.y, -view.x);
	case 1: return float3(-view.z, view.y, view.x);
	case 2: return float3(view.x, view.z, -view.y);
	case 3: return float3(view.x, -view.z, view.y);
	case 4: return view;
	default: return float3(-view.x, view.y, -view.z);
	}
}

uint3 probe_coordinate(uint probe)
{
	return uint3(probe % SIZE.x, probe / SIZE.x % SIZE.y,
		     probe / (SIZE.x * SIZE.y));
}

uint2 picture_origin(uint3 t)
{
	return uint2(6 * FACE * t.x, FACE * (SIZE.y * t.z + t.y));
}

[shader("compute")]
[numthreads(64, 1, 1)]
void settle(uint3 group: SV_GroupID, uint thread: SV_GroupIndex)
{
	uint entry = relight_list[relight_current.first + group.x];
	uint3 t = probe_coordinate(entry & ~HOLDS);
	uint2 origin = picture_origin(t);

	if (thread == 0)
		settle_back_faces = 0;
	GroupMemoryBarrierWithGroupSync();
	for (uint i = thread; i < TEXELS; i += GROUP) {
		uint face = i / (FACE * FACE);
		int2 at = int2(i % FACE, i / FACE % FACE);
		uint2 tile = origin + uint2(FACE * face, 0);
		float2 uv = (float2(at) + 0.5) / FACE;
		float3 direction = face_direction(
			float3(2.0 * uv.x - 1.0, 1.0 - 2.0 * uv.y, 1.0), face);
		float2 sum = float2(0.0, 0.0);

		if (dot(relight_normal[tile + uint2(at)].xyz, direction) > 0.0)
			InterlockedAdd(settle_back_faces, 1);
		for (int dy = -1; dy <= 1; dy++)
			for (int dx = -1; dx <= 1; dx++) {
				int2 n = clamp(at + int2(dx, dy), 0, int(FACE) - 1);
				float d = relight_normal[tile + uint2(n)].w;

				sum += float2(d, d * d);
			}
		relight_moments[tile + uint2(at)] = sum / 9.0;
	}
	GroupMemoryBarrierWithGroupSync();
	if (thread == 0)
		relight_validity[t] = (entry & HOLDS) != 0 &&
					      settle_back_faces <= TEXELS / 4 ?
					      1.0 :
					      0.0;
}

// The solid angle of face texel `at`: the cube's area element integrated over
// the texel's corners on the face at distance one.
float area_element(float x, float y)
{
	return atan2(x * y, sqrt(x * x + y * y + 1.0));
}

float texel_solid_angle(int2 at)
{
	float2 low = float2(at) / FACE * 2.0 - 1.0;
	float2 high = float2(at + 1) / FACE * 2.0 - 1.0;

	return abs(area_element(low.x, low.y) - area_element(low.x, high.y) -
		   area_element(high.x, low.y) + area_element(high.x, high.y));
}

// How much of the sun reaches `world` facing `n`: one compare on the first
// cascade whose box holds it, pushed a texel along n; 1 outside every one.
float sun_visibility(float3 world, float3 n)
{
	relight_shadow shadow = relight_lights.shadow;

	for (uint c = 0; c < shadow.count && c < 4; c++) {
		float3 pushed = world + n * shadow.texels[c];
		float4 clip = mul(shadow.cascades[c], float4(pushed, 1.0));
		float3 ndc = clip.xyz / clip.w;

		if (any(abs(ndc.xy) > 1.0) || ndc.z < 0.0 || ndc.z > 1.0)
			continue;
		float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
		return relight_sun_shadow.SampleCmpLevelZero(float3(uv, c), ndc.z);
	}
	return 1.0;
}

// lighting.slangh's voe_render_point_visibility on this file's binding.
float lamp_visibility(relight_lamp lamp, float3 world, float3 n)
{
	float width;
	float height;
	float layers;

	relight_point_shadow.GetDimensions(width, height, layers);
	float3 to = world - lamp.position;
	float3 a = abs(to);
	float d = max(a.x, max(a.y, a.z));
	float3 pushed = to + n * (2.0 * d / width);
	uint face = voe_render_point_shadow_face(pushed);
	float4 clip = voe_render_point_shadow_clip(pushed, face, lamp.range);
	float3 ndc = clip.xyz / clip.w;
	float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
	float layer = float(6 * (lamp.shadow - 1) + face);

	return relight_point_shadow.SampleCmpLevelZero(float3(uv, layer), ndc.z);
}

// Level 1's light at `world` facing `n`: the chain's sun and lamps.
float3 direct_light(float3 world, float3 n)
{
	relight_push push = relight_current;
	float3 sum = float3(0.0, 0.0, 0.0);

	if (push.sun_chain == push.chain) {
		relight_light sun = relight_lights.sun;
		float n_dot_l = saturate(dot(n, -sun.direction));

		if (n_dot_l > 0.0)
			sum += sun.colour * sun.intensity *
			       relight_lights.sun_strength * n_dot_l *
			       sun_visibility(world, n);
	}
	for (uint i = 0; i < push.lamps && i < LAMPS; i++) {
		relight_lamp lamp = relight_lights.lamps[i];
		float3 to = lamp.position - world;
		float d = length(to);

		if (lamp.bounces != push.chain || d >= lamp.range || d < 1e-7)
			continue;
		float n_dot_l = saturate(dot(n, to / d));
		float fade = saturate(1.0 - pow(d / lamp.range, 2.0 / lamp.falloff));
		float shade = 1.0;

		if (push.point_ready != 0 && lamp.shadow != 0 &&
		    lamp.shadow_strength > 0.0)
			shade = lerp(1.0, lamp_visibility(lamp, world, n),
				     lamp.shadow_strength);
		sum += lamp.colour * lamp.bounce_strength * n_dot_l * fade *
		       fade * shade;
	}
	return sum;
}

// Level (chain, level − 1)'s E at `world` facing `n`, over π.
float3 level_below(float3 world, float3 n)
{
	relight_push push = relight_current;
	uint below = 3 * (push.chain * (push.chain - 1) / 2 + push.level - 2);

	return voe_render_bounce_read(relight_levels[below],
				      relight_levels[below + 1],
				      relight_levels[below + 2],
				      relight_validity_read, relight_moments_read,
				      relight_lights.corner, relight_lights.cell,
				      relight_lights.spacing, world, n) /
	       PI;
}

// Grid `grid`'s three images at `t`, by constant index.
void store_grid(uint grid, uint3 t, float4 red, float4 green, float4 blue)
{
	[ForceUnroll]
	for (uint g = 0; g < LEVELS + 1; g++)
		if (g == grid) {
			relight_grids[3 * g][t] = red;
			relight_grids[3 * g + 1][t] = green;
			relight_grids[3 * g + 2][t] = blue;
		}
}

[shader("compute")]
[numthreads(64, 1, 1)]
void relight(uint3 group: SV_GroupID, uint thread: SV_GroupIndex)
{
	relight_push push = relight_current;
	uint3 t = probe_coordinate(group.x);
	uint2 origin = picture_origin(t);
	uint3 placed = (t + SIZE - relight_lights.cell % SIZE) % SIZE;
	float3 centre = relight_lights.corner +
			(float3(placed) + 0.5) * relight_lights.spacing;
	float4 sh[3] = { float4(0.0), float4(0.0), float4(0.0) };

	if (relight_validity[t] > 0.5)
		for (uint i = thread; i < TEXELS; i += GROUP) {
			uint face = i / (FACE * FACE);
			int2 at = int2(i % FACE, i / FACE % FACE);
			uint2 texel = origin + uint2(FACE * face, 0) + uint2(at);
			float2 uv = (float2(at) + 0.5) / FACE;
			float3 direction = normalize(face_direction(
				float3(2.0 * uv.x - 1.0, 1.0 - 2.0 * uv.y, 1.0), face));
			float4 hit = relight_normal[texel];

			if (hit.w >= REACH * 0.999 || !(dot(hit.xyz, direction) < 0.0))
				continue;
			float3 world = centre + direction * hit.w;
			float3 n = normalize(hit.xyz);
			float3 light = push.level == 1 ? direct_light(world, n) :
							 level_below(world, n);
			float3 radiance = relight_albedo.Load(int3(texel, 0)).rgb *
					  light * texel_solid_angle(at);
			float4 basis = float4(SH0, SH1 * direction);

			sh[0] += radiance.r * basis;
			sh[1] += radiance.g * basis;
			sh[2] += radiance.b * basis;
		}
	for (uint c = 0; c < 3; c++)
		relight_sums[c][thread] = sh[c];
	GroupMemoryBarrierWithGroupSync();
	for (uint stride = GROUP / 2; stride > 0; stride /= 2) {
		if (thread < stride)
			for (uint c = 0; c < 3; c++)
				relight_sums[c][thread] += relight_sums[c][thread + stride];
		GroupMemoryBarrierWithGroupSync();
	}
	if (thread == 0)
		store_grid(push.chain * (push.chain - 1) / 2 + push.level - 1, t,
			   relight_sums[0][0], relight_sums[1][0], relight_sums[2][0]);
}

[shader("compute")]
[numthreads(64, 1, 1)]
void sum(uint3 id: SV_DispatchThreadID)
{
	if (id.x >= TOTAL)
		return;
	uint3 t = probe_coordinate(id.x);
	float4 sh[3] = { float4(0.0), float4(0.0), float4(0.0) };

	[ForceUnroll]
	for (uint g = 0; g < LEVELS; g++) {
		sh[0] += relight_grids[3 * g][t];
		sh[1] += relight_grids[3 * g + 1][t];
		sh[2] += relight_grids[3 * g + 2][t];
	}
	store_grid(LEVELS, t, sh[0], sh[1], sh[2]);
}
