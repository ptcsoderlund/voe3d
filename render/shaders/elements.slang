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
// ONE INSTANCE PER ELEMENT, AND THE RECORD'S NUMBER IS WHERE THE DRAW STARTED
// PLUS THE INSTANCE. One buffer holds every surface a frame draws, so a draw
// takes a range of it and vkCmdDraw's firstInstance is how the range arrives —
// no push constant carries it and none could, because the sixty-four bytes are
// the transform and the layout is shared with draw.slang.
//
// AND THE TWO SEMANTICS BELOW ARE BOTH NEEDED, WHICH IS THE TRAP. Slang gives
// SV_InstanceID HLSL's meaning: the instance's number within this draw, counting
// from nought no matter what firstInstance was. SV_StartInstanceLocation is that
// skipped count, and the sum is the record's number. Reading SV_InstanceID alone
// draws the first `count` records of the buffer for every range but the first —
// which looks perfect on a frame with one surface in it and is wrong on the
// second one. See render/tests/elements.c, which puts the same rectangles in two
// ranges and moves only the matrix, precisely so that this cannot come back.
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
// ITS. One layout means the descriptor set bound as a pass opens stays
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
//
// A PICTURE IS A RECTANGLE WHOSE COLOUR CAME OUT OF A TEXTURE, AND THAT IS THE
// WHOLE OF THE IMAGE KIND. The same `sheet` rectangle walks the same `sheet_at`
// across the element; what is read there is multiplied into the record's colour
// rather than thresholded into its alpha. A view drawn into a target of one's
// own is shown this way, and so is an icon.
//
// A LETTER IS A RECTANGLE WHOSE COVERAGE CAME OUT OF A SHEET, AND THAT IS THE
// WHOLE OF THE GLYPH KIND. A GLYPH record carries the part of a distance-field
// sheet it reads and the texture it reads it from; this file samples that,
// takes the median of three channels, thresholds it, and multiplies the
// record's alpha by the answer. There is no material, no lighting and no
// shading record on this path — which is exactly what lets a letter and a fill
// sit in one buffer and go out as one draw command.
//
// A THRESHOLD, NOT A SMOOTHSTEP, AND NOT BY OVERSIGHT. Card 026 took the
// antialiasing out of this engine. The sheet asks for a filtered sampler
// (VOE_RENDER_SAMPLING_FIELD) so that the field's crossings land where the
// distances say they land rather than on the nearest texel boundary; that moves
// the edge, it does not blur it, and there is no sampler setting on that path
// that can produce a partly covered pixel. Softening the edge here would put
// back the thing that was deliberately removed.
//
// BUT THE CUT IS NOT AT THE FIELD'S HALF: IT SITS HALF A SCREEN PIXEL OUTSIDE
// THE OUTLINE, LESS ONE BYTE STEP OF THE FIELD (ADR-0182, ADR-0183, ADR-0184).
// A pixel is inside when its centre is within that distance of the letter, so a
// thin stroke keeps a pixel even when it falls between two pixel centres; at the
// half alone such a stroke lost every pixel and the letter lost a limb. At four
// texels per pixel, where strokes are thin, the distance is just under half a
// pixel; it only shrinks when text is magnified, where every stroke is several
// pixels wide. Less than half because an outline lying exactly on pixel boundaries has
// its nearest outside centre exactly half a pixel away, and the sheet stores
// bytes: a rounded texel can read up to half a byte step high, so the guard is a
// whole byte step in field, not a fraction of a pixel, which would fail at some
// magnification. Such text keeps its true width at every scale and does not gain
// a row; text that does not line up is at most one pixel wider, which bug 01 of
// 009 accepts.
// voe_render_element_cutoff turns the distance into a field value, and
// draw.slang's world text keeps the half.
//
// THE SHEET IS SAMPLED AT AN EXPLICIT LEVEL AND THAT IS WHY THE BRANCH BELOW IS
// FREE. draw.slang's fragment stage samples everything before it branches,
// because an implicit-level Sample needs the derivatives the rasteriser
// computes across a quad of fragments. There are no mipmaps anywhere in this
// engine, so level nought is the only level there is; asking for it by name
// needs no derivatives at all, so a branch on the record's kind costs nothing
// and needs no rule about where it may go. A solid then samples nothing rather
// than fetching a white pixel it would throw away.
//
// THE ONE DERIVATIVE THE GLYPH NEEDS IS TAKEN BEFORE ANY BRANCH. The cutoff
// needs how many sheet texels one screen pixel spans, which is fwidth of the
// interpolated sheet coordinate, and a derivative is only defined in uniform
// control flow. So it is the first thing the fragment stage does — ahead of the
// clip test's discard as well as the kind branch — and the glyph branch only
// multiplies it by its sheet's size. It is taken from the sheet coordinate and
// not from the median, whose slope flattens on the ridge of a thin stroke,
// exactly where the cut has to move (ADR-0183).
//
// AND THE TEXTURE INDEX HERE IS GENUINELY NON-UNIFORM, WHICH draw.slang'S IS
// NOT. Over there the index comes from a record a push constant picked, so it
// is one value for the whole draw. Here it comes out of the record the instance
// index picked, and one draw holds a whole surface's worth of elements — two
// letters of two different sheets in one draw is the ordinary case, not a
// corner one. So the subscript is wrapped in NonUniformResourceIndex and
// render/src/device.c asks the card for shaderSampledImageArrayNonUniformIndexing
// to match. Without both, this is undefined behaviour that happens to work on
// most drivers.

// One element. Matches voe_render_element in render/include/render/device.h
// field for field, padding included: render/src/descriptors.c asserts on the
// size and on every offset a layout rule could have moved, so a member that
// moves is a build error rather than an interface drawn in the wrong place.
//
// THE THREE WORDS AFTER `kind` ARE SCALARS AND NOT A uint3, for the reason
// draw.slang's records give: a three-component vector has a sixteen-byte
// alignment in every buffer layout rule there is and three uints have a
// four-byte one, so only the scalars land where C put them. That was true when
// all three were spare and it is what let the glyph kind take the first of them
// in place.
struct voe_render_element {
	// xy is the top-left corner, zw the width and height, in millimetres.
	float4 bounds;
	// The same shape, and what a fragment has to be inside to survive.
	float4 clip;
	// Linear RGBA, straight. Multiplied by its own alpha at the return.
	float4 colour;
	uint kind;
	// The sheet a glyph reads, as a texture id's index half. A solid never
	// reads it.
	uint sheet_texture;
	uint reserved_a0;
	uint reserved_a1;
	// What part of that sheet the glyph reads: xy the top-left corner and zw
	// the width and height, in texture coordinates. The same xy-plus-wh
	// shape as bounds and clip. A solid never reads it.
	float4 sheet;
};

// Set 0, binding 1: every texture at once, the same array draw.slang declares
// and the same one the descriptor set frame.c binds as a pass opens
// already holds. Declaring it here adds no descriptor, no binding and no set —
// the two shaders share one pipeline layout, which is the reason they can.
//
// THE LENGTH IS WRITTEN DOWN IN FOUR PLACES NOW — device_internal.h, the layout
// in descriptors.c, draw.slang and here — and all four have to agree, because a
// descriptor array's length is fixed in each of them and there is no way to say
// "as many as the C side thinks" from a shader.
[[vk::binding(1, 0)]]
Sampler2D voe_render_textures[64];

// VOE_RENDER_ELEMENT_GLYPH, which is the C enum's second value. The kinds are
// written out on both sides rather than shared, the same way this whole record
// is; descriptors.c is what catches the two drifting apart in layout and this
// number is small enough to read against voe_render_element_kind by eye.
static const uint VOE_RENDER_ELEMENT_GLYPH = 1;
// VOE_RENDER_ELEMENT_IMAGE, the third.
static const uint VOE_RENDER_ELEMENT_IMAGE = 2;

// The field's encoded range, in sheet texels either side of the outline: a
// field value of nought or one is this many texels outside or inside it. It
// must equal VOE_TEXT_FIELD_SPREAD in text/src/raster.h, which is where the
// sheets are written; it is copied because render cannot include text. Change
// one, change the other.
static const float VOE_RENDER_ELEMENT_FIELD_SPREAD = 4.0;

// The field value at and above which a glyph's pixel counts as inside: the
// half, moved half a screen pixel outward less one byte step of the field
// (ADR-0183, ADR-0184). `texels_per_pixel` is how many sheet texels one screen
// pixel spans along each axis; the larger is taken, so a squeezed axis never
// under-dilates. The byte step keeps an outline on pixel boundaries from gaining
// a row when a rounded texel reads half a step high. Clamped to at most the half
// so the cut never erodes past the outline, and to at least a five-hundred-and-
// twelfth so that a texel encoded as nought — far outside everything — is never
// inside, however small the text gets.
float voe_render_element_cutoff(float2 texels_per_pixel)
{
	float field_per_pixel = max(texels_per_pixel.x, texels_per_pixel.y)
		/ (2.0 * VOE_RENDER_ELEMENT_FIELD_SPREAD);
	return clamp(0.5 - 0.5 * field_per_pixel + 1.0 / 255.0,
		     1.0 / 512.0, 0.5);
}

// The middle of three, which is what turns three one-sided distance fields back
// into one shape with its corners intact.
//
// IT IS THE SECOND COPY AND draw.slang's voe_render_median IS THE FIRST. The
// two are the same three min/max and they must stay the same: a sheet read one
// way in the world and another way on a panel is the same letter with two
// different shapes. It is copied rather than included because the build compiles
// every .slang in shaders/ as a module of its own and tracks no #include
// dependency, so a shared file would be compiled on its own as well as leave
// both dependents stale when it changed — which is machinery in cmake/voe.cmake
// that this does not need. Change one, change the other.
float voe_render_element_median(float3 field)
{
	return max(min(field.r, field.g), min(max(field.r, field.g), field.b));
}

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
	// Where in the sheet this corner reads. Interpolated, and it is the one
	// thing here that is meant to be: the rasteriser walking a coordinate
	// across a rectangle is exactly the map from `at` inside `bounds` to a
	// point inside the record's `sheet`, done for free by the hardware that
	// is already interpolating `at`. It is built from the corner rather than
	// recovered in the fragment stage for the same reason `at` is passed
	// rather than recovered: the vertex stage has the numbers in its hand.
	//
	// NO VERTEX ATTRIBUTE IS INVOLVED AND NONE COULD BE. There is no vertex
	// buffer on this path; this is a vertex OUTPUT, computed from the corner
	// and the record.
	float2 sheet_at : TEXCOORD1;
	// Flat, both of them, for the reason `clip` is: they are one value for
	// the whole element. Interpolating a texture index would be meaningless
	// and interpolating the kind would make a rectangle half solid.
	nointerpolation uint kind : TEXCOORD2;
	nointerpolation uint sheet_texture : TEXCOORD3;
};

[shader("vertex")]
voe_render_element_vertex_out
voe_render_element_vertex(uint vertex : SV_VertexID,
			  uint instance : SV_InstanceID,
			  uint first : SV_StartInstanceLocation)
{
	voe_render_element_vertex_out out;
	uint element = first + instance;
	voe_render_element record = voe_render_elements[element];
	// Which corner this vertex is, out of the two low bits of its index:
	// 0 -> (0,0), 1 -> (1,0), 2 -> (0,1), 3 -> (1,1). The strip's two
	// triangles are (0,1,2) and (2,1,3), which together are the rectangle.
	float2 corner = float2(vertex & 1u, vertex >> 1u);

	out.at = record.bounds.xy + corner * record.bounds.zw;
	// The same corner, walked across the record's sheet rectangle instead of
	// its bounds. A solid's sheet rectangle is nothing and this is nought;
	// the fragment stage never reads it.
	out.sheet_at = record.sheet.xy + corner * record.sheet.zw;
	// z is nought and w is one: the transform decides the depth, because the
	// depth convention is the C side's and stating a number here would be a
	// second place for it to live.
	out.position = mul(voe_render_element_current.transform,
			   float4(out.at, 0.0, 1.0));
	out.colour = record.colour;
	out.clip = record.clip;
	out.kind = record.kind;
	out.sheet_texture = record.sheet_texture;
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
	// Sheet coordinate per screen pixel, for the glyph's cutoff. First, in
	// uniform control flow, before the discard and the branch below: see the
	// header. Every kind computes it and only a glyph reads it.
	float2 sheet_per_pixel = fwidth(vertex.sheet_at);

	float2 low = vertex.clip.xy;
	float2 high = vertex.clip.xy + vertex.clip.zw;

	if (any(vertex.at < low) || any(vertex.at > high))
		discard;

	// A solid covers itself completely; a glyph covers as much of itself as
	// the sheet says it does; an image is the picture times the record's
	// colour. The record's own alpha is what all three are multiplied into,
	// so a half-transparent letter fades like anything else.
	float4 colour = vertex.colour;

	if (vertex.kind == VOE_RENDER_ELEMENT_GLYPH) {
		// Level nought by name, and the subscript non-uniform: see the
		// header for both. The sheet's rgb are three signed distances
		// and its alpha is not read at all.
		float3 field = voe_render_textures[
			NonUniformResourceIndex(vertex.sheet_texture)]
			.SampleLevel(vertex.sheet_at, 0.0).rgb;

		// A hard cut half a pixel less a byte step outside the outline, not
		// draw.slang's half: see the header. The derivative was taken
		// above; here it only becomes texels by the sheet's size.
		// Inside or outside, and nothing in between.
		uint sheet_width;
		uint sheet_height;
		voe_render_textures[NonUniformResourceIndex(vertex.sheet_texture)]
			.GetDimensions(sheet_width, sheet_height);
		float2 texels_per_pixel =
			sheet_per_pixel * float2(sheet_width, sheet_height);
		colour.a *= step(voe_render_element_cutoff(texels_per_pixel),
				 voe_render_element_median(field));
	} else if (vertex.kind == VOE_RENDER_ELEMENT_IMAGE) {
		// The same explicit level and the same non-uniform subscript as
		// the glyph above, for the same reasons; the texture is a picture
		// read NEAREST, so what arrives is one texel, already decoded out
		// of sRGB by its format. All four channels, alpha included, and
		// treated as straight like the record's colour — a target's
		// picture is premultiplied, which is the same thing while it is
		// opaque. See VOE_RENDER_ELEMENT_IMAGE.
		colour *= voe_render_textures[
			NonUniformResourceIndex(vertex.sheet_texture)]
			.SampleLevel(vertex.sheet_at, 0.0);
	}

	// Premultiplied, once, and the last thing this file does — on whatever
	// colour the branches above arrived at. See the header: the target holds
	// premultiplied colour and the blend state is written to match. Every
	// kind leaves through here, which is what keeps the multiply in one
	// place.
	return float4(colour.rgb * colour.a, colour.a);
}
