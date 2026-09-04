// THE ONE THING THIS SHADER DOES IS READ A MATRIX AND SAY WHAT IT SAW. It exists
// because -matrix-layout-row-major is a flag in cmake/voe.cmake that nothing
// could previously check: with it, float4x4 in a shader has the layout
// voe_math_float4x4 has in C; without it slangc reads the same sixteen floats
// column-major, every transform comes out transposed, and nothing fails to
// compile. A flag whose absence is invisible is a flag that will one day go
// missing, so render/tests/matrix.c uploads a known matrix through the engine's
// own descriptor and requires this shader to report the elements back.
//
// HOW THE TWO LAYOUTS DIFFER, WHICH IS THE WHOLE TEST. The C side writes element
// (row, column) at float row * 4 + column. Row-major means m[0][3] here reads
// float 3, which is the element C wrote at [0][3]. Column-major means m[0][3]
// reads float 12, which is the element C wrote at [3][0]. The test fills the
// matrix so that every element is a different value, so the three floats written
// out below name which layout was in force and cannot be mistaken for the other.
//
// IT REPORTS THROUGH COLOUR BECAUSE COLOUR IS WHAT A TEST CAN READ BACK. There
// is no other path from a shader to the CPU in this engine — no storage buffer
// and nothing that reads one — so the three elements come out as the three
// channels of an unorm colour and render/tests/matrix.c allows a byte or two of
// quantisation. The values it picks are far enough apart that a rounding mode
// cannot turn one layout's answer into the other's.
//
// IT SHARES THE DRAW PIPELINE'S DESCRIPTOR, ON PURPOSE. Set 0, binding 0, so
// what is under test is the path the engine actually uploads through and not a
// second one written to be tested. device.c's descriptor set layout serves both.
//
// IT DECLARES ONLY THE FRONT OF THAT BLOCK. draw.slang's binding 0 holds the
// camera and the sun; this reads the first matrix and nothing else, and a
// uniform block declared shorter than the buffer behind it is allowed. There is
// nothing to keep in step here beyond that first member's offset, which is
// zero.
//
// IT READS view, WHICH IS THE FIRST MEMBER, AND WHICH MEMBER IT IS DOES NOT
// MATTER. What is under test is the layout of the sixteen floats inside one
// float4x4, so any member of this struct would do; the first one is chosen so
// that the offset in the buffer is zero and the test writes at the start of what
// it mapped. It used to be a member called model, which moved out to a push
// constant when a second cube arrived and there was a per-object matrix to push.
//
// NO VERTEX BUFFER, NO DEPTH, NO CULLING. Three vertices from SV_VertexID cover
// the whole target, so every pixel carries the answer and the test may sample
// wherever it likes. probe.c builds this pipeline without a depth attachment and
// with culling off, because winding and depth are other tests' claims and a
// probe that could be culled would report nothing at all.

struct voe_render_view {
	float4x4 view;
	float4x4 projection;
};

[[vk::binding(0, 0)]]
ConstantBuffer<voe_render_view> voe_render_camera;

struct voe_render_matrix_probe_out {
	float4 position : SV_Position;
};

// A triangle twice the size of clip space, so it covers the target whichever way
// the viewport flips it. Written in clip space directly: the matrix under test is
// read, never applied.
static const float2 VOE_RENDER_MATRIX_PROBE_POSITIONS[3] = {
	float2(-1.0, -1.0),
	float2(3.0, -1.0),
	float2(-1.0, 3.0),
};

[shader("vertex")]
voe_render_matrix_probe_out
voe_render_matrix_probe_vertex(uint index : SV_VertexID)
{
	voe_render_matrix_probe_out out;

	out.position =
		float4(VOE_RENDER_MATRIX_PROBE_POSITIONS[index], 1.0, 1.0);
	return out;
}

// Row 0 column 3, row 1 column 3, row 2 column 3 — the translation column of a
// matrix that has one, and three floats a long way apart in memory from the ones
// the other layout would hand back.
[shader("fragment")]
float4 voe_render_matrix_probe_fragment(voe_render_matrix_probe_out vertex)
	: SV_Target
{
	float4x4 m = voe_render_camera.view;

	return float4(m[0][3], m[1][3], m[2][3], 1.0);
}
