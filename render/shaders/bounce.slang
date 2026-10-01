// The bounce grid update's two compute entries (ADR-0308 point 4), dispatched
// by render/src/bounce_grid.c between passes, after a bounce pass has drawn the
// sun's bounce map.
//
// reduce: one thread per 8×8 block of the 512² map, 64×64 threads, each writing
// one virtual point light (VPL): the mean position about the eye of the block's
// covered texels, unprojected from depth through the inverse of the bounce
// pass's projection × view; their mean normal; and their flux summed × one
// texel's area in m², which is the block's mean flux × the block's area. A
// texel is covered when its normal's w is 1; a block with none is a VPL of w 0
// that gather skips.
//
// gather: one thread per listed probe. A listed index is toroidal, x + 32y +
// 1024z of the probe's world cell mod 32, and is also where it is stored in the
// grid's images. Each VPL's radiance toward the probe is flux × max(0, n·−ω) ×
// w / (π × max(d², 1)), ω the unit direction from the probe to the VPL, d the
// distance and w = saturate((n · (probe − VPL) + o) / spacing), n its unit
// normal: the probe's standoff in front of the VPL's face (ADR-0313) plus the
// allowance o, VOE_BOUNCE_STANDOFF, 0.25 m (ADR-0314), so a probe up to o
// behind the face's plane still takes a share and one cell less o in front
// takes it whole; the shadow side's faint trace that leaves is ADR-0315's
// bound. It is projected into L1 SH and blended into the old value with the
// listed blend. A VPL nearer the probe than 1 mm adds nothing, its direction
// undefined, so no NaN reaches the grid, where the blend would keep it. Each
// of the three images is one colour, R, G, B, holding (c0, cx, cy, cz):
// c0 = Σ L × 0.282095 and c(xyz) = Σ L × 0.488603 × ω.xyz.
//
// The push block matches struct bounce_push in bounce_grid.c, offsets and all.
//
// CONSTRAINTS. gather loops over every VPL from every thread, 4096 reads per
// probe from the storage buffer; tiling the VPLs through group-shared memory
// would lift that if a profile names it. Nothing occludes a VPL from a probe.

static const uint TEXELS = 512;
static const uint BLOCK = 8;
static const uint VPLS_SIDE = TEXELS / BLOCK;
static const uint SIDE = 32;
static const uint VPLS = VPLS_SIDE * VPLS_SIDE;
static const float PI = 3.14159265;
// How far behind a VPL's face, in metres, a probe still takes its light
// (ADR-0314): the probe serving a box's foot sits just behind its face.
static const float VOE_BOUNCE_STANDOFF = 0.25;

struct vpl {
	float4 position;
	float4 normal;
	float4 flux;
};

struct bounce_push {
	float4x4 unproject;
	float3 corner;
	uint probes;
	uint3 cell;
	uint count;
	uint blends;
	float spacing;
	uint reserved0;
	uint reserved1;
};

[[vk::binding(0, 0)]]
Texture2D<float> bounce_depth;
[[vk::binding(1, 0)]]
Texture2D<float4> bounce_flux;
[[vk::binding(2, 0)]]
Texture2D<float4> bounce_normal;
[[vk::binding(3, 0)]]
RWStructuredBuffer<vpl> bounce_vpls;
// The probe indices, then their blends as float bits, at push offsets.
[[vk::binding(4, 0)]]
StructuredBuffer<uint> bounce_list;
[[vk::binding(5, 0)]]
[format("rgba16f")]
RWTexture3D<float4> bounce_grid[3];

[[vk::push_constant]]
ConstantBuffer<bounce_push> bounce_current;

// The point about the eye under framebuffer position `pixel` at `depth`,
// through the viewport's one Y flip.
float3 unproject(float2 pixel, float depth)
{
	float2 ndc = float2(2.0 * pixel.x / TEXELS - 1.0,
			    1.0 - 2.0 * pixel.y / TEXELS);
	float4 p = mul(bounce_current.unproject, float4(ndc, depth, 1.0));

	return p.xyz / p.w;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void reduce(uint3 id: SV_DispatchThreadID)
{
	uint2 origin = id.xy * BLOCK;
	float3 position = float3(0.0);
	float3 normal = float3(0.0);
	float3 flux = float3(0.0);
	float depth = 0.0;
	uint covered = 0;
	vpl out;

	if (id.x >= VPLS_SIDE || id.y >= VPLS_SIDE)
		return;
	for (uint i = 0; i < BLOCK * BLOCK; i++) {
		int3 texel = int3(origin + uint2(i % BLOCK, i / BLOCK), 0);
		float4 n = bounce_normal.Load(texel);
		float d = bounce_depth.Load(texel);

		if (n.w <= 0.0)
			continue;
		position += unproject(float2(texel.xy) + 0.5, d);
		normal += n.xyz;
		flux += bounce_flux.Load(texel).rgb;
		depth += d;
		covered++;
	}
	out.position = float4(0.0);
	out.normal = float4(0.0);
	out.flux = float4(0.0);
	if (covered > 0) {
		float2 o = float2(origin);
		float d = depth / covered;
		float3 a = unproject(o, d);
		float3 area = cross(unproject(o + float2(BLOCK, 0.0), d) - a,
				    unproject(o + float2(0.0, BLOCK), d) - a);

		out.position = float4(position / covered, 1.0);
		out.normal = float4(normalize(normal), 0.0);
		out.flux = float4(flux / (BLOCK * BLOCK) * length(area), 0.0);
	}
	bounce_vpls[id.y * VPLS_SIDE + id.x] = out;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void gather(uint3 id: SV_DispatchThreadID)
{
	float4 sh[3] = { float4(0.0), float4(0.0), float4(0.0) };
	uint probe;
	float blend;
	uint3 slot;
	float3 centre;

	if (id.x >= bounce_current.count)
		return;
	probe = bounce_list[bounce_current.probes + id.x];
	blend = asfloat(bounce_list[bounce_current.blends + id.x]);
	slot = uint3(probe % SIDE, (probe / SIDE) % SIDE, probe / (SIDE * SIDE));
	centre = bounce_current.corner +
		 (float3((slot - bounce_current.cell) & (SIDE - 1)) + 0.5) *
			 bounce_current.spacing;

	for (uint i = 0; i < VPLS; i++) {
		vpl light = bounce_vpls[i];
		float3 v = light.position.xyz - centre;
		float d2 = dot(v, v);
		float3 w;
		float standoff;
		float3 radiance;

		if (light.position.w <= 0.0 || d2 < 1e-6)
			continue;
		w = v * rsqrt(d2);
		standoff = saturate((dot(light.normal.xyz, -v) +
				     VOE_BOUNCE_STANDOFF) /
				    bounce_current.spacing);
		radiance = light.flux.rgb * max(0.0, dot(light.normal.xyz, -w)) *
			   standoff / (PI * max(d2, 1.0));
		for (uint c = 0; c < 3; c++)
			sh[c] += radiance[c] *
				 float4(0.282095, 0.488603 * w);
	}
	for (uint c = 0; c < 3; c++)
		bounce_grid[c][slot] = lerp(bounce_grid[c][slot], sh[c], blend);
}
