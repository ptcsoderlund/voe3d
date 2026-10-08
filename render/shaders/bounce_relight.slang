// The relight's compute entries (ADR-0326 points 5 and 6), dispatched by
// render/src/bounce_relight.c between passes, after the begun target's capture
// passes have copied their pictures into its atlases.
//
// A PROBE'S PICTURE: probe index x + 24y + 288z (toroidal), at x 48x + 8f,
// y 8(12z + y) of the atlases, face f in point_shadow.slangh's order and texel
// orientation, so texel (u, v) of face f looks along the face's view
// (2u − 1, 1 − 2v, 1), u and v at the texel's centre over the face.
//
// settle: one workgroup of 64 per listed probe. A list word: the probe in bits
// 0–15, its readiness 0 to FADE in bits 16–20, bit 31 set when it holds a
// picture. The first `changed` words are changed probes, the rest fading only.
// - Readiness / FADE into the validity image's G, for every word.
// - A fading-only word stops there: R and the moments stay as they are.
// - Validity, R at the probe's coordinate: 1, or 0 when it holds no picture or
//   more than a quarter of its texels (96) see a back face, normal · direction
//   from the probe above nought: a probe inside something. No hit has a normal
//   of nought, so it is never a back face.
// - Moments, at the texel's own place: the mean and mean² of the distances
//   over its 3×3 within its face, a neighbour past the edge clamped to it.
//
// relight: one workgroup of 64 per probe, over its 384 texels: each of the grid
// with `whole` 1, else each of the first `changed` listed (ADR-0389 point 5);
// into level (chain, level) — grid chain(chain − 1)/2 + level − 1. Each texel weighs
// its solid angle; no hit (distance at the volume's reach, 12 × the record's
// spacing) or a back face is nought;
// else at the surface point (probe + direction × distance) radiance = albedo ×
// - level 1: the chain's lights, colour × intensity × bounce strength ×
//   max(0, n·l) × shadow: every sun (the first, each `more` entry) by its own
//   layer of the volume's map (ADR-0329, 0357), one compare pushed a texel
//   along n, lit outside its box and when this frame drew none; a lamp faded
//   as lighting.slangh's falloff, with its slot's compare when ready;
// - level k > 1: bounce_read.slangh's E of level (chain, k − 1) / π;
// summed as six-axis irradiance (ADR-0327): per axis a, radiance ×
// max(0, direction_a) into E(+a) and × max(0, −direction_a) into E(−a), six RGB
// values reduced in group memory. That is the exact clamped-cosine irradiance
// along each axis, so flat ground and walls take nothing from behind their own
// plane, where first-order harmonics lit an upward normal from a ring below
// its horizon. An invalid probe writes nought.
// BLOCKERS (ADR-0350 point 5), the record's by blockers.slangh's pass, with
// its `walls` and `indoors`: a texel's p is its hit pushed PUSH along its
// normal. It feeds its probe only when the pass holds from the probe to p;
// level 1's sun lights it when the pass holds from that sun's mask along its
// segment, a lamp from the lamp's mask and position; level k reads
// through bounce_read.slangh's gated read.
// THE TEXELS. Grid g's image 3g + a is axis a (x, y, z), 48 × 12 × 24: probe
// t's E(+a) at (2t.x, t.y, t.z), E(−a) at (2t.x + 1, t.y, t.z), RGB, alpha 0.
// sum: grid 6 = the sum of grids 0 to 5, one thread per texel of the 48-wide
// images with `whole`, else a listed probe's two texels in its own workgroup, as
// relight; a level not in use is nought, bounce_relight.c clears it.
//
// The push block matches struct relight_push in bounce_relight.c, the record at
// binding 11 its struct relight_record: the structs below repeat bindings.slangh's
// light and point light, which this file cannot include (its set 0).
//
// CONSTRAINTS. A workgroup of 64, under Vulkan's guaranteed 128. The grids are
// written with unrolled constant indices: the device enables no dynamic
// storage-image indexing. Nine loads a texel for its moments, and the level
// read's eight probes a texel, where group-shared memory would take fewer; a
// texel's blocker masks (its own, each lit lamp's) and pass tests are worked
// out per texel.

#include "point_shadow.slangh"
#include "blockers.slangh"
#include "bounce_read.slangh"

static const uint3 SIZE = uint3(24, 12, 24);
static const uint TOTAL = 24 * 12 * 24;
static const uint FACE = 8;
static const uint TEXELS = 6 * FACE * FACE;
static const uint GROUP = 64;
static const uint HOLDS = 0x80000000u;
static const uint PROBE = 0xffffu;
static const uint READY_SHIFT = 16;
static const uint READY = 0x1fu;
// VOE_RENDER_BOUNCE_FADE: the places a new picture takes to full weight.
static const float FADE = 16.0;
static const uint LAMPS = 16;
static const uint BLOCKERS = 32;
static const uint LEVELS = 6;
// VOE_RENDER_DIRECTIONAL_LIGHTS − 1: the further suns the record holds.
static const uint MORE = 3;
// A probe's reach in its volume's cells; × the record's spacing, metres.
static const float REACH_CELLS = 12.0;
// VOE_RENDER_LIGHT_BLOCKER_PUSH in render/device.h: a hit is masked this far
// off its surface, along its normal.
static const float PUSH = 0.05;
static const float PI = 3.14159265358979;

struct relight_push {
	uint first;
	uint count;
	uint chain;
	uint level;
	uint sun_chain;
	uint lamps;
	uint point_ready;
	uint changed;
	uint whole;
};

struct relight_light {
	float3 direction;
	float intensity;
	float3 colour;
	uint unshaded;
	float3 fill;
	float reserved;
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

// voe_render_light_blocker: three rows into the box's unit space and a bounding
// sphere, about the eye.
struct relight_blocker {
	float4 rows[3];
	float4 sphere;
};

// struct voe_render_relight_sun: three pad words, not an array, which std140
// would stride 16 bytes apart.
struct relight_sun {
	relight_light light;
	float4x4 map;
	float texel;
	uint drawn;
	float strength;
	uint mask;
	uint bounces;
	uint pad0;
	uint pad1;
	uint pad2;
};

struct relight_record {
	relight_light sun;
	float4x4 sun_map;
	float sun_texel;
	uint sun_drawn;
	uint blocker_count;
	uint reserved;
	float3 corner;
	float sun_strength;
	uint3 cell;
	float spacing;
	relight_lamp lamps[LAMPS];
	relight_blocker blockers[BLOCKERS];
	// The blockers' kinds and the sun's mask (ADR-0350 point 2).
	uint walls;
	uint indoors;
	uint sun_mask;
	uint pad;
	// The further suns that bounce (ADR-0357 point 4), more[i] on layer i + 1.
	uint more_count;
	uint more_pad0;
	uint more_pad1;
	uint more_pad2;
	relight_sun more[MORE];
};

[[vk::binding(0, 0)]]
[format("rgba16f")]
RWTexture2D<float4> relight_normal;
[[vk::binding(1, 0)]]
[format("rg16f")]
RWTexture2D<float2> relight_moments;
[[vk::binding(2, 0)]]
[format("rg16f")]
RWTexture3D<float2> relight_validity;
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

// The record's blockers as blockers.slangh's list.
struct record_blockers : voe_render_blocker_list {
	uint count()
	{
		return min(relight_lights.blocker_count, BLOCKERS);
	}
	float4 sphere(uint i)
	{
		return relight_lights.blockers[i].sphere;
	}
	float4 row(uint i, uint r)
	{
		return relight_lights.blockers[i].rows[r];
	}
};

groupshared uint settle_back_faces;
groupshared float3 relight_sums[6][GROUP];

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
	uint3 t = probe_coordinate(entry & PROBE);
	uint2 origin = picture_origin(t);
	float readiness = float((entry >> READY_SHIFT) & READY) / FADE;

	// Uniform over the group, so it leaves before any barrier together.
	if (group.x >= relight_current.changed) {
		if (thread == 0)
			relight_validity[t] = float2(relight_validity[t].r, readiness);
		return;
	}
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
		relight_validity[t] = float2((entry & HOLDS) != 0 &&
						     settle_back_faces <= TEXELS / 4 ?
						     1.0 :
						     0.0,
					     readiness);
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

// How much of a sun reaches `world` facing `n`: one compare in layer `layer` of
// the volume's map, drawn with `map` at `texel`, pushed a texel along n; 1
// outside its box or when this frame drew none (`drawn` 0).
float sun_visibility(float4x4 map, float texel, uint drawn, uint layer,
		     float3 world, float3 n)
{
	if (drawn == 0)
		return 1.0;
	float3 pushed = world + n * texel;
	float4 clip = mul(map, float4(pushed, 1.0));
	float3 ndc = clip.xyz / clip.w;

	if (any(abs(ndc.xy) > 1.0) || ndc.z < 0.0 || ndc.z > 1.0)
		return 1.0;
	float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
	return relight_sun_shadow.SampleCmpLevelZero(float3(uv, float(layer)),
						     ndc.z);
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

// Whether the record's blockers pass light from a source of mask `source` at
// `s` to a receiver of mask `receiver` at `p`.
bool record_passes(uint source, uint receiver, float3 p, float3 s)
{
	record_blockers blockers;

	return voe_render_blockers_pass(blockers, relight_lights.walls,
					relight_lights.indoors, source, receiver,
					p, s);
}

// One sun's level 1 term at `world` facing `n`, whose mask is `mask`, `p` the
// hit pushed PUSH: gated by the sun's own mask along its own segment and
// shadowed by its own layer.
float3 sun_light(relight_light sun, float strength, uint sun_mask,
		 float4x4 map, float texel, uint drawn, uint layer,
		 float3 world, float3 n, float3 p, uint mask)
{
	float n_dot_l = saturate(dot(n, -sun.direction));

	if (n_dot_l <= 0.0 ||
	    !record_passes(sun_mask, mask, p,
			   voe_render_sun_segment_end(p, sun.direction)))
		return float3(0.0, 0.0, 0.0);
	return sun.colour * sun.intensity * strength * n_dot_l *
	       sun_visibility(map, texel, drawn, layer, world, n);
}

// Level 1's light at `world` facing `n`, whose mask is `mask`: the chain's suns
// and its lamps, each where the blockers pass it to the hit pushed PUSH.
float3 direct_light(float3 world, float3 n, uint mask)
{
	relight_push push = relight_current;
	record_blockers blockers;
	float3 p = world + n * PUSH;
	float3 sum = float3(0.0, 0.0, 0.0);

	if (push.sun_chain == push.chain)
		sum += sun_light(relight_lights.sun, relight_lights.sun_strength,
				 relight_lights.sun_mask, relight_lights.sun_map,
				 relight_lights.sun_texel, relight_lights.sun_drawn,
				 0, world, n, p, mask);
	for (uint s = 0; s < relight_lights.more_count && s < MORE; s++) {
		relight_sun more = relight_lights.more[s];

		if (more.bounces == push.chain)
			sum += sun_light(more.light, more.strength, more.mask,
					 more.map, more.texel, more.drawn, s + 1,
					 world, n, p, mask);
	}
	for (uint i = 0; i < push.lamps && i < LAMPS; i++) {
		relight_lamp lamp = relight_lights.lamps[i];
		float3 to = lamp.position - world;
		float d = length(to);

		if (lamp.bounces != push.chain || d >= lamp.range || d < 1e-7 ||
		    !record_passes(voe_render_blockers_mask(blockers,
							    lamp.position),
				   mask, p, lamp.position))
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

// Level (chain, level − 1)'s E at `world` facing `n`, whose mask is `mask`,
// over π.
float3 level_below(float3 world, float3 n, uint mask)
{
	relight_push push = relight_current;
	record_blockers blockers;
	uint below = 3 * (push.chain * (push.chain - 1) / 2 + push.level - 2);

	return voe_render_bounce_read(relight_levels[below],
				      relight_levels[below + 1],
				      relight_levels[below + 2],
				      relight_validity_read, relight_moments_read,
				      relight_lights.corner, relight_lights.cell,
				      relight_lights.spacing, blockers,
				      relight_lights.walls, relight_lights.indoors,
				      mask, world, n) /
	       PI;
}

// Probe `t`'s six-axis irradiance `e` (+x, −x, +y, −y, +z, −z) into grid
// `grid`'s three images, by constant index, at the header's texels.
void store_grid(uint grid, uint3 t, float3 e[6])
{
	uint3 positive = uint3(2 * t.x, t.y, t.z);
	uint3 negative = positive + uint3(1, 0, 0);

	[ForceUnroll]
	for (uint g = 0; g < LEVELS + 1; g++)
		if (g == grid) {
			[ForceUnroll]
			for (uint a = 0; a < 3; a++) {
				relight_grids[3 * g + a][positive] = float4(e[2 * a], 0.0);
				relight_grids[3 * g + a][negative] =
					float4(e[2 * a + 1], 0.0);
			}
		}
}

[shader("compute")]
[numthreads(64, 1, 1)]
void relight(uint3 group: SV_GroupID, uint thread: SV_GroupIndex)
{
	relight_push push = relight_current;
	uint3 t = probe_coordinate(
		push.whole != 0 ? group.x :
				  relight_list[push.first + group.x] & PROBE);
	uint2 origin = picture_origin(t);
	uint3 placed = (t + SIZE - relight_lights.cell % SIZE) % SIZE;
	float3 centre = relight_lights.corner +
			(float3(placed) + 0.5) * relight_lights.spacing;
	float reach = REACH_CELLS * relight_lights.spacing;
	record_blockers blockers;
	uint probe_mask = voe_render_blockers_mask(blockers, centre);
	float3 e[6];

	for (uint d = 0; d < 6; d++)
		e[d] = float3(0.0);
	if (relight_validity[t].r > 0.5)
		for (uint i = thread; i < TEXELS; i += GROUP) {
			uint face = i / (FACE * FACE);
			int2 at = int2(i % FACE, i / FACE % FACE);
			uint2 texel = origin + uint2(FACE * face, 0) + uint2(at);
			float2 uv = (float2(at) + 0.5) / FACE;
			float3 direction = normalize(face_direction(
				float3(2.0 * uv.x - 1.0, 1.0 - 2.0 * uv.y, 1.0), face));
			float4 hit = relight_normal[texel];

			if (hit.w >= reach * 0.999 || !(dot(hit.xyz, direction) < 0.0))
				continue;
			float3 world = centre + direction * hit.w;
			float3 n = normalize(hit.xyz);
			float3 p = world + n * PUSH;
			uint mask = voe_render_blockers_mask(blockers, p);

			if (!record_passes(probe_mask, mask, p, centre))
				continue;
			float3 light = push.level == 1 ? direct_light(world, n, mask) :
							 level_below(world, n, mask);
			float3 radiance = relight_albedo.Load(int3(texel, 0)).rgb *
					  light * texel_solid_angle(at);

			for (uint a = 0; a < 3; a++) {
				e[2 * a] += radiance * max(0.0, direction[a]);
				e[2 * a + 1] += radiance * max(0.0, -direction[a]);
			}
		}
	for (uint c = 0; c < 6; c++)
		relight_sums[c][thread] = e[c];
	GroupMemoryBarrierWithGroupSync();
	for (uint stride = GROUP / 2; stride > 0; stride /= 2) {
		if (thread < stride)
			for (uint c = 0; c < 6; c++)
				relight_sums[c][thread] += relight_sums[c][thread + stride];
		GroupMemoryBarrierWithGroupSync();
	}
	if (thread == 0) {
		for (uint c = 0; c < 6; c++)
			e[c] = relight_sums[c][0];
		store_grid(push.chain * (push.chain - 1) / 2 + push.level - 1, t,
			   e);
	}
}

[shader("compute")]
[numthreads(64, 1, 1)]
void sum(uint3 id: SV_DispatchThreadID, uint3 group: SV_GroupID,
	 uint thread: SV_GroupIndex)
{
	uint3 t;

	if (relight_current.whole != 0) {
		if (id.x >= 2 * TOTAL)
			return;
		t = uint3(id.x % (2 * SIZE.x), id.x / (2 * SIZE.x) % SIZE.y,
			  id.x / (2 * SIZE.x * SIZE.y));
	} else {
		// A listed probe's two texels, E(+a) and E(−a).
		if (thread >= 2)
			return;
		uint3 p = probe_coordinate(
			relight_list[relight_current.first + group.x] & PROBE);
		t = uint3(2 * p.x + thread, p.y, p.z);
	}
	float4 axes[3] = { float4(0.0), float4(0.0), float4(0.0) };

	[ForceUnroll]
	for (uint g = 0; g < LEVELS; g++) {
		axes[0] += relight_grids[3 * g][t];
		axes[1] += relight_grids[3 * g + 1][t];
		axes[2] += relight_grids[3 * g + 2][t];
	}
	relight_grids[3 * LEVELS][t] = axes[0];
	relight_grids[3 * LEVELS + 1][t] = axes[1];
	relight_grids[3 * LEVELS + 2][t] = axes[2];
}
