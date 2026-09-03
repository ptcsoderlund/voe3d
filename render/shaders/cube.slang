// The cube: eight vertices and thirty-six indices, fed in from buffers, placed
// by three matrices. It replaced triangle.slang, which had no vertex buffer and
// no matrix behind it and so had nothing left to prove once these existed.
//
// ONE FILE, TWO ENTRY POINTS, ONE SPIR-V MODULE. slangc compiles this whole file
// to cube.spv with both entry points in it, and device.c names them by the names
// spelled here — see -fvk-use-entrypoint-name in cmake/voe.cmake, which is what
// keeps those two strings true.
//
// THE MATRICES ARE READ ROW-MAJOR AND THAT COSTS A FLAG. slangc is invoked with
// -matrix-layout-row-major, so float4x4 here has the layout voe_math_float4x4
// has in C and the upload is a straight copy of sixteen floats. slangc's own
// default is the other one and getting it wrong transposes every transform
// without failing to compile — which is why matrix_probe.slang exists and why
// render/tests/matrix.c refuses to pass without the flag.
//
// mul(M, v) IS M·v, AND COMPOSITION READS RIGHT TO LEFT. Vectors are columns, so
// the model matrix is applied first and appears innermost below. That is the
// same convention voe_math_float4x4_mul_float4 implements on the CPU, so a
// transform written here and one written there mean the same thing.
//
// NOTHING HERE FLIPS Y. Vulkan's clip space is Y-down and this engine is Y-up,
// and the whole of that reconciliation is the negative viewport height in
// frame.c. A projection matrix that negated a row as well would flip twice,
// which looks exactly like not flipping at all until something is culled.
//
// THE COLOUR IS THE CORNER'S POSITION AND THAT IS WHAT MAKES THE CUBE READABLE.
// Each of the eight corners carries a different colour, so every face is a
// gradient rather than a flat fill and a person can tell which face they are
// looking at — which is the whole of how "solid, near faces hiding far ones" is
// checked by eye. There are eight vertices and not twenty-four because there are
// no texture coordinates to disagree about per face; the card that adds them
// rewrites this data.

struct voe_render_cube_uniforms {
	float4x4 model;
	float4x4 view;
	float4x4 projection;
};

// Set 0, binding 0, and the only descriptor this engine has. device.c builds the
// layout that matches it and frame.c binds one of these per frame slot.
[[vk::binding(0, 0)]]
ConstantBuffer<voe_render_cube_uniforms> voe_render_cube_matrices;

// The locations are stated rather than left to declaration order, because the C
// side names the same two numbers in its VkVertexInputAttributeDescription and
// two lists that have to agree should not agree by counting.
struct voe_render_cube_vertex_in {
	[[vk::location(0)]] float3 position : POSITION;
	[[vk::location(1)]] float3 colour : COLOUR;
};

struct voe_render_cube_vertex_out {
	float4 position : SV_Position;
	float3 colour : COLOUR;
};

[shader("vertex")]
voe_render_cube_vertex_out
voe_render_cube_vertex(voe_render_cube_vertex_in vertex)
{
	voe_render_cube_vertex_out out;
	float4 world;
	float4 eye;

	world = mul(voe_render_cube_matrices.model, float4(vertex.position, 1.0));
	eye = mul(voe_render_cube_matrices.view, world);
	out.position = mul(voe_render_cube_matrices.projection, eye);
	out.colour = vertex.colour;
	return out;
}

[shader("fragment")]
float4 voe_render_cube_fragment(voe_render_cube_vertex_out vertex) : SV_Target
{
	return float4(vertex.colour, 1.0);
}
