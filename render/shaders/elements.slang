// The element pipeline: many small rectangles, one instanced draw, and no
// vertex buffer anywhere. One record per rectangle says where it is, what colour
// it is and what it is clipped to; this file turns each record into four corners
// and fills them.
//
// IT IS A SECOND SHADER AND NOT A BRANCH IN draw.slang, ON PURPOSE. That file
// already carries three uniform branches and there is a standing agreement to
// rethink the arrangement at the fourth (D-093), so a whole second way of
// getting a position goes here instead. Nothing in this file is shared with it
// beyond the descriptor set it happens to sit in.
//
// NOTHING IS READ FROM A VERTEX BUFFER BECAUSE NONE IS BOUND. The pipeline's
// vertex input state describes no bindings and no attributes; the vertex stage
// has only its own vertex index and its own instance index, and that is enough
// to build a rectangle. This is unusual and Vulkan is content with it — if the
// validation layers complain about something here, read what they actually say
// before reaching for a dummy buffer.
//
// FOUR VERTICES AND A TRIANGLE STRIP, NOT SIX AND A LIST. Two fewer vertex
// stage invocations per element, and the corner falls out of two bit operations
// on the vertex index rather than out of a table: 0 is (0,0), 1 is (1,0), 2 is
// (0,1) and 3 is (1,1), which is exactly the order a strip wants. The pipeline
// culls nothing, so the strip's winding is not a fact anybody has to keep right
// — a screen-space rectangle has no back face to hide.
//
// ONE INSTANCE PER ELEMENT, AND THE INSTANCE INDEX IS THE RECORD'S NUMBER. No
// push constant carries it and none could: there is one draw for the whole
// frame's worth of elements. This is the shape draw.slang's own header says an
// indirect mesh draw would take.
//
// ORDER IS PAINT ORDER AND THIS PATH RELIES ON IT. Vulkan rasterises the
// primitives of one draw in submission order, and instances in instance-index
// order, so element n blends over element n-1 and the caller's submission order
// is what a person sees. Depth writes are off, so depth decides nothing between
// two elements; take that guarantee away and an interface would need sorting it
// has no way to do.
//
// THE TRANSFORM IS THE WHOLE OF WHERE THE SURFACE IS. Element space is
// millimetres in two dimensions (ADR-0089) and the push constant below turns it
// into clip space in one multiply. voe_render_element_transform on the C side is
// where that matrix comes from and where the Y direction is decided; nothing in
// this file flips anything.
//
// IT SHARES THE PIPELINE LAYOUT WITH draw.slang AND THE PUSH CONSTANT ALIASES
// ITS. One layout means the descriptor set bound at the top of the frame stays
// bound across an element draw, which is the reason for sharing it — two layouts
// with different push constant ranges are incompatible and would disturb the set
// for every mesh draw after the elements. Both blocks start at offset 0 and both
// pipelines push their own immediately before their own draw, so neither ever
// reads the other's bytes. See the push constant range in render/src/device.c.
//
// THE OUTPUT IS PREMULTIPLIED (ADR-0069), which is the engine's contract for
// anything that writes into the colour target: rgb is multiplied by alpha once,
// at the return below, and the blend state is source-ONE,
// destination-ONE_MINUS_SRC_ALPHA to match. A shader that forgets it draws an
// interface that is slightly too bright over everything, which reads as a colour
// somebody chose badly rather than as a bug.
//
// EVERYTHING HERE IS LINEAR LIGHT. The colour in a record is linear and the
// target's own sRGB format encodes it once on the way out. There is no gamma
// arithmetic in this file and there must not be.

// One element. Matches voe_render_element in render/include/render/device.h
// field for field, padding included: render/src/descriptors.c asserts on the
// size and on every offset a layout rule could have moved, so a member that
// moves is a build error rather than an interface drawn in the wrong place.
//
// THE PADDING AFTER `kind` IS THREE SCALARS AND NOT A uint3, for the reason
// draw.slang's records give: a three-component vector has a sixteen-byte
// alignment in every buffer layout rule there is and three uints have a
// four-byte one, so only the scalars land where C put them.
struct voe_render_element {
	// xy is the top-left corner, zw the width and height, in millimetres.
	float4 bounds;
	// The same shape, and what a fragment has to be inside to survive.
	float4 clip;
	// Linear RGBA, straight. Multiplied by its own alpha at the return.
	float4 colour;
	uint kind;
	uint reserved_a0;
	uint reserved_a1;
	uint reserved_a2;
	// Where card 031's glyph rectangle goes. Sixteen bytes on a sixteen-byte
	// boundary, so consuming it moves nothing.
	float4 reserved_b;
};

// Set 0, binding 4: this frame's elements, one record per submitted element,
// read by both stages — the vertex stage wants the bounds and the fragment
// stage wants the colour and the clip.
[[vk::binding(4, 0)]]
StructuredBuffer<voe_render_element> voe_render_elements;

// Element space to clip space, for the whole surface. See
// voe_render_element_transform.
struct voe_render_element_push {
	float4x4 transform;
};

[[vk::push_constant]]
ConstantBuffer<voe_render_element_push> voe_render_element_current;

// The element-space position of a fragment, which is what the clip test needs,
// and the colour, which is uniform across the rectangle and interpolates to
// itself.
//
// `at` IS PASSED RATHER THAN RECOVERED. A fragment could get back to element
// space by inverting the transform, and that would be a matrix inverse per
// fragment to recover a number the vertex stage had in its hand.
struct voe_render_element_vertex_out {
	float4 position : SV_Position;
	float2 at : POSITION;
	float4 colour : COLOR;
	// Flat: the clip rectangle is one rectangle for the whole element and
	// interpolating four numbers that are equal at all four corners is
	// arithmetic to arrive back where it started. It is also what keeps the
	// comparison below exact.
	nointerpolation float4 clip : TEXCOORD;
};

[shader("vertex")]
voe_render_element_vertex_out
voe_render_element_vertex(uint vertex : SV_VertexID, uint element : SV_InstanceID)
{
	voe_render_element_vertex_out out;
	voe_render_element record = voe_render_elements[element];
	// Which corner this vertex is, out of the two low bits of its index:
	// 0 -> (0,0), 1 -> (1,0), 2 -> (0,1), 3 -> (1,1). The strip's two
	// triangles are (0,1,2) and (2,1,3), which together are the rectangle.
	float2 corner = float2(vertex & 1u, vertex >> 1u);

	out.at = record.bounds.xy + corner * record.bounds.zw;
	// z is nought and w is one: the transform decides the depth, because the
	// depth convention is the C side's and stating a number here would be a
	// second place for it to live.
	out.position = mul(voe_render_element_current.transform,
			   float4(out.at, 0.0, 1.0));
	out.colour = record.colour;
	out.clip = record.clip;
	return out;
}

[shader("fragment")]
float4
voe_render_element_fragment(voe_render_element_vertex_out vertex) : SV_Target
{
	// The clip rectangle, in element space, and the one thing this stage
	// decides. A zeroed rectangle discards everything, which is what
	// voe_render_element says it does: whoever builds a record says what it
	// is clipped to and nothing here repairs a caller that did not.
	float2 low = vertex.clip.xy;
	float2 high = vertex.clip.xy + vertex.clip.zw;

	if (any(vertex.at < low) || any(vertex.at > high))
		discard;

	// Premultiplied, once, and the last thing this file does. See the
	// header: the target holds premultiplied colour and the blend state is
	// written to match.
	return float4(vertex.colour.rgb * vertex.colour.a, vertex.colour.a);
}
