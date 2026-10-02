// The relight's compute entries (ADR-0326 point 5 now, point 6 from card 09),
// dispatched by render/src/bounce_relight.c between passes, after the begun
// target's capture passes have copied their pictures into its atlases.
//
// settle: one workgroup of 64 threads per listed probe, each thread six of the
// probe's 384 texels (six 8 × 8 faces). A list entry is the probe's toroidal
// index, x + 24y + 288z, with bit 31 set when it holds a picture; its picture is
// at x 48x + 8f, y 8(12z + y) of the atlases, face f in point_shadow.slangh's
// order and texel orientation, so texel (u, v) of face f looks along the face's
// view (2u − 1, 1 − 2v, 1), u and v at the texel's centre over the face.
// - Validity, into the validity image at the probe's coordinate: 1, or 0 when
//   the probe holds no picture or more than a quarter of its texels (96) see a
//   back face, its normal · the direction from the probe above nought, which is
//   a probe standing inside something. A texel that hit nothing has a normal of
//   nought, so it is never a back face.
// - Moments, into the moments atlas at the texel's own place: the mean and the
//   mean of the square of the distances (the normal atlas's w) over its 3×3
//   within its own face, a neighbour past the face's edge clamped to the edge.
//
// The push block matches struct settle_push in bounce_relight.c: where the list
// starts in the buffer, and how many entries.
//
// CONSTRAINTS. A workgroup of 64, under Vulkan's guaranteed 128; nine storage
// loads a texel for its moments, where group-shared memory would take one.

static const uint3 SIZE = uint3(24, 12, 24);
static const uint FACE = 8;
static const uint TEXELS = 6 * FACE * FACE;
static const uint GROUP = 64;
static const uint HOLDS = 0x80000000u;

struct settle_push {
	uint first;
	uint count;
	uint reserved0;
	uint reserved1;
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

[[vk::push_constant]]
ConstantBuffer<settle_push> relight_current;

groupshared uint settle_back_faces;

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

[shader("compute")]
[numthreads(64, 1, 1)]
void settle(uint3 group: SV_GroupID, uint thread: SV_GroupIndex)
{
	uint entry = relight_list[relight_current.first + group.x];
	uint probe = entry & ~HOLDS;
	uint3 t = uint3(probe % SIZE.x, probe / SIZE.x % SIZE.y,
			probe / (SIZE.x * SIZE.y));
	uint2 origin = uint2(6 * FACE * t.x, FACE * (SIZE.y * t.z + t.y));

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
