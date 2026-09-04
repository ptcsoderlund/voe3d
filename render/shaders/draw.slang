// The one pipeline: a vertex out of the shared pool, placed by three matrices —
// two of them the camera's, shared by every draw in a frame, and one of them the
// object's, read out of a buffer by the object's own number — and shaded by a
// record read the same way. It replaced cube.slang, which had the cube's two
// entry points in it and a per-object matrix in a push constant.
//
// THE ONLY PUSH CONSTANT IS THE OBJECT'S NUMBER, AND EVERYTHING ELSE IS A BUFFER
// READ. That is what a draw is told that the draw beside it is not; the matrix
// and the shading it names are in memory both stages can index. It is the shape
// an indirect draw needs — the number would come from the instance index instead
// and nothing else here would change — and it is why the CPU side has no push
// constant bigger than four bytes.
//
// THREE NUMBERS AND NONE OF THEM IS A HANDLE: object, shading, texture. The
// object number picks a record, the record names a shading index, the shading
// record names texture indices, and a texture index is a subscript into the
// array below. Every one of them is the same integer the CPU handed out
// (ADR-0018), so there is no table anywhere between the two sides of the bus.
//
// ONE FILE, TWO ENTRY POINTS, ONE SPIR-V MODULE. slangc compiles this whole file
// to draw.spv with both entry points in it, and device.c names them by the names
// spelled here — see -fvk-use-entrypoint-name in cmake/voe.cmake, which is what
// keeps those two strings true.
//
// THE MATRICES ARE READ ROW-MAJOR AND THAT COSTS A FLAG. slangc is invoked with
// -matrix-layout-row-major, so float4x4 here has the layout voe_math_float4x4
// has in C and an upload is a straight copy of sixteen floats. slangc's own
// default is the other one and getting it wrong transposes every transform
// without failing to compile — which is why matrix_probe.slang exists and why
// render/tests/matrix.c refuses to pass without the flag.
//
// mul(M, v) IS M·v, AND COMPOSITION READS RIGHT TO LEFT. Vectors are columns, so
// the world matrix is applied first and appears innermost below. That is the
// same convention voe_math_float4x4_mul_float4 implements on the CPU.
//
// NOTHING HERE FLIPS Y. Vulkan's clip space is Y-down and this engine is Y-up,
// and the whole of that reconciliation is the negative viewport height in
// frame.c. A projection matrix that negated a row as well would flip twice,
// which looks exactly like not flipping at all until something is culled.
//
// NOTHING HERE FLIPS THE TEXTURE COORDINATE EITHER. (0,0) is the top-left of the
// image in Vulkan and in glTF, and assets decodes rows top-down, so all three
// agree and the picture comes out the right way up. The flip everyone adds is
// OpenGL's, whose texture origin is at the bottom.
//
// THE SHADING RECORD IS READ IN FULL AND ONLY THE BASE COLOUR IS USED. Card 018
// parses and uploads every channel glTF gives — metalness, roughness, occlusion,
// emission and their textures — and card 019 is the one that lights anything
// with them. The struct is here in full so that the layout is settled by the
// card that fills it rather than by the card that reads it.

// The camera. One per frame, and the same one for every draw in it.
struct voe_render_view {
	float4x4 view;
	float4x4 projection;
};

// Set 0, binding 0. descriptors.c builds the layout that matches it and frame.c
// writes one of these per frame slot.
[[vk::binding(0, 0)]]
ConstantBuffer<voe_render_view> voe_render_camera;

// Set 0, binding 1: every texture at once, indexed by a number out of a shading
// record.
//
// THE LENGTH IS VOE_RENDER_MAX_TEXTURES AND IT IS WRITTEN DOWN IN THREE PLACES —
// device_internal.h, the layout in descriptors.c, and here. A descriptor array
// has a fixed length in all three and they have to agree; there is no way to say
// "as many as the C side thinks" from a shader. Every element is always written,
// slots nothing has claimed pointing at a one-pixel white texture, because
// Vulkan requires the whole array to be valid whether or not this samples it.
[[vk::binding(1, 0)]]
Sampler2D voe_render_textures[64];

// One drawn object. Matches voe_render_object in render/device.h field for
// field, padding included: the matrix fills the first sixty-four bytes and the
// four uints after it fill to eighty, so every layout rule a structured buffer
// might be given puts every member in the same place.
//
// THE PADDING IS THREE SCALARS AND NOT A uint3, AND THAT IS NOT A STYLE CHOICE.
// A three-component vector has a sixteen-byte alignment in every buffer layout
// rule there is, so a uint3 after a single uint would start at eighty rather
// than sixty-eight and the record would be ninety-six bytes on this side and
// eighty on the other. Three uints have a four-byte alignment each and land
// where C puts them. The same applies to the shading record below.
struct voe_render_object {
	float4x4 world;
	uint shading;
	uint reserved0;
	uint reserved1;
	uint reserved2;
};

// Set 0, binding 2: this frame's records, one per draw. Read by both stages —
// the vertex stage wants the matrix and the fragment stage wants the shading
// index.
[[vk::binding(2, 0)]]
StructuredBuffer<voe_render_object> voe_render_objects;

// How a surface is shaded. Matches voe_render_shading_values in
// render/device.h, padding included, for the reason above.
struct voe_render_shading {
	float4 base_colour;
	float metallic;
	float roughness;
	float2 reserved_a;
	float3 emissive;
	float reserved_b;
	uint base_colour_texture;
	uint metallic_roughness_texture;
	uint normal_texture;
	uint occlusion_texture;
	uint emissive_texture;
	uint reserved_c0;
	uint reserved_c1;
	uint reserved_c2;
};

// Set 0, binding 3: every shading record, shared by every frame slot because
// nothing rewrites one after it is made.
[[vk::binding(3, 0)]]
StructuredBuffer<voe_render_shading> voe_render_shadings;

// Which object this draw is. Four bytes, and the whole of the push constant.
struct voe_render_draw_push {
	uint object;
};

[[vk::push_constant]]
ConstantBuffer<voe_render_draw_push> voe_render_draw_current;

// The locations are stated rather than left to declaration order, because the C
// side names the same three numbers in its VkVertexInputAttributeDescription and
// two lists that have to agree should not agree by counting.
struct voe_render_draw_vertex_in {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 normal : NORMAL;
	[[vk::location(2)]] float2 uv : TEXCOORD;
};

struct voe_render_draw_vertex_out {
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

[shader("vertex")]
voe_render_draw_vertex_out
voe_render_draw_vertex(voe_render_draw_vertex_in vertex)
{
	voe_render_draw_vertex_out out;
	voe_render_object object = voe_render_objects[voe_render_draw_current.object];
	float4 world;
	float4 eye;

	world = mul(object.world, float4(vertex.position, 1.0));
	eye = mul(voe_render_camera.view, world);
	out.position = mul(voe_render_camera.projection, eye);

	// Carried through so that the vertex layout the CPU describes is the
	// layout this stage reads. Card 019 is what lights anything with it.
	out.normal = vertex.normal;
	out.uv = vertex.uv;
	return out;
}

[shader("fragment")]
float4 voe_render_draw_fragment(voe_render_draw_vertex_out vertex) : SV_Target
{
	uint index = voe_render_objects[voe_render_draw_current.object].shading;
	voe_render_shading shading = voe_render_shadings[index];

	// NonUniformResourceIndex is not used and is not needed: the texture
	// index comes from a record picked by a push constant, so it is the same
	// value for every fragment in the draw. It would be needed the day the
	// index came from a vertex attribute or from the instance index.
	//
	// A MATERIAL WITH NO TEXTURE SAMPLES WHITE, WHICH IS WHY THERE IS NO
	// BRANCH HERE. VOE_RENDER_NO_TEXTURE is slot 0 and slot 0 is one white
	// pixel, so multiplying by it leaves the factor alone.
	return shading.base_colour *
	       voe_render_textures[shading.base_colour_texture].Sample(vertex.uv);
}
