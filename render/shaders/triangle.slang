// The first shader in the engine, and for now the only one. Three vertices with
// no vertex buffer: the draw is vkCmdDraw(3, 1, 0, 0) and everything a vertex
// needs is looked up here by its index.
//
// ONE FILE, TWO ENTRY POINTS, ONE SPIR-V MODULE. slangc compiles this whole file
// to triangle.spv with both entry points in it, and device.c names them when it
// builds the pipeline. The names are the engine's names, not `main`, which is
// what -fvk-use-entrypoint-name in cmake/voe.cmake is for; two entry points
// cannot both be called `main` anyway, so this is the shape a second shader
// stage forces and it may as well be the shape the first one has.
//
// THE TRIANGLE IS WOUND COUNTER-CLOCKWISE WITH +Y UP, which is glTF's
// front-facing winding and so this engine's. It stays counter-clockwise on
// screen, because the viewport flips Y and that is what the flip is for: the
// pipeline in device.c names the same winding this file does, with no
// translation between them. Culling is on, so this is proved rather than
// asserted — render/tests/offscreen.c draws these three vertices and requires
// them to survive, and draws them mirrored and requires them not to.
//
// IT IS ASYMMETRIC IN Y ON PURPOSE. The apex is up and the base is down, so a
// person looking at the window can see whether the Y flip went the right way.
// A triangle symmetric about the horizon would look identical either way and
// would tell them nothing. The three corner colours are the other half of that:
// an interpolated gradient is a rasteriser and a fragment stage doing their job,
// which a flat fill would not distinguish from a differently-coloured clear.

struct voe_render_triangle_vertex_out {
	float4 position : SV_Position;
	float3 colour : COLOUR;
};

// Clip space directly — there is no matrix in this engine yet and this card does
// not add one. z is 1.0 because this engine's near plane is 1.0 and its far
// plane is 0.0; there is no depth buffer here to care, and picking 0.0 would
// read to the next person as "at the far plane" when it means nothing at all.
static const float3 VOE_RENDER_TRIANGLE_POSITIONS[3] = {
	float3(0.0, 0.6, 1.0),
	float3(-0.6, -0.6, 1.0),
	float3(0.6, -0.6, 1.0),
};

static const float3 VOE_RENDER_TRIANGLE_COLOURS[3] = {
	float3(1.0, 0.0, 0.0),
	float3(0.0, 1.0, 0.0),
	float3(0.0, 0.0, 1.0),
};

[shader("vertex")]
voe_render_triangle_vertex_out voe_render_triangle_vertex(uint index : SV_VertexID)
{
	voe_render_triangle_vertex_out out;

	out.position = float4(VOE_RENDER_TRIANGLE_POSITIONS[index], 1.0);
	out.colour = VOE_RENDER_TRIANGLE_COLOURS[index];
	return out;
}

[shader("fragment")]
float4 voe_render_triangle_fragment(voe_render_triangle_vertex_out vertex) : SV_Target
{
	return float4(vertex.colour, 1.0);
}
