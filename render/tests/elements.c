// The element path's rectangles: many of them from many small records, all in
// one draw command. Ten claims, each of them one a wrong implementation would
// get wrong quietly:
//
// - four colours in one draw, element y = 0 at the top;
// - the clip rectangle clips;
// - paint order is submission order, both ways round;
// - the blend is premultiplied;
// - overrunning the element room is refused and the next frame is fine;
// - a mesh drawn after an element draw is still right;
// - an empty frame draws nothing;
// - a device with no element room refuses rather than asserts;
// - two ranges of the one buffer are two matrices and two draws;
// - a range past what was submitted is refused.
//
// Letters are tests/glyphs.c; the arithmetic that needs no device is
// tests/element_transform.c.
//
// IT READS THE PICTURE BACK, through tests/element_scene.h, because nothing
// about this path can be checked any other way. A submit that wrote at the
// wrong offset, a vertex shader that built the corners the wrong way round, a
// clip test comparing the wrong pair of numbers and a blend that multiplied
// twice all leave the bookkeeping perfect.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include "element_scene.h"

#include <math/float3.h>

// Four, which is what the busiest test below submits, so that submitting a
// fifth is the overrun.
#define MAX_ELEMENTS 4

// One quad and two objects wearing two records, which is what
// a_mesh_after_an_element_draw_is_still_right needs; every other test here
// draws no mesh at all.
static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 2,
	.shadings = 2,
	.elements = MAX_ELEMENTS,
	.passes = 1,
};

// Two records for the mesh half, both unlit so that a pixel's colour is the
// record's base colour and nothing about a sun has to be right for a count to
// mean something, and both reading the white default texture over the whole of
// itself. The same shape tests/transient.c uses.
static const voe_render_shading_values MESH_RED = {
	.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.unlit = 1,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};
static const voe_render_shading_values MESH_BLUE = {
	.base_colour = { 0.0f, 0.0f, 1.0f, 1.0f },
	.roughness = 1.0f,
	.unlit = 1,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// A quad over the top-left quadrant of clip space — x from -1 to 0 and y from 0
// to 1, and clip y = +1 is the top of the screen because of the engine's one Y
// flip. Corner for corner and index for index the winding tests/offscreen.c
// already proves is a front face. Depth 0.5, which is above the clear of 0 under
// the GREATER test this engine runs.
static void quad(voe_render_vertex vertices[4], uint32_t indices[6])
{
	vertices[0] = (voe_render_vertex){ { -1.0f, 1.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 0.0f } };
	vertices[1] = (voe_render_vertex){ { 0.0f, 1.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 0.0f } };
	vertices[2] = (voe_render_vertex){ { 0.0f, 0.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 1.0f } };
	vertices[3] = (voe_render_vertex){ { -1.0f, 0.0f, 0.5f },
					   { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 1.0f } };
	indices[0] = 3;
	indices[1] = 2;
	indices[2] = 1;
	indices[3] = 3;
	indices[4] = 1;
	indices[5] = 0;
}

// One drawn object: the quad shifted along x by `shift` clip-space units and
// wearing `shading`.
static voe_render_object shifted(float shift, voe_render_shading shading)
{
	voe_math_float3 by = { shift, 0.0f, 0.0f };

	return (voe_render_object){
		.world = voe_math_float4x4_from_translation(by),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// Three quadrants of three colours and the fourth left as the clear, drawn by
// one draw command, and the arrangement is asymmetrical so that a picture the
// wrong way up or the wrong way round cannot pass.
//
// RED IS TOP-LEFT IN ELEMENT SPACE AND HAS TO BE TOP-LEFT IN THE PICTURE, which
// is the whole of the Y-direction claim: element y = 0 is the top.
//
// ONE DRAW COMMAND IS COUNTED AND NOT ASSUMED. voe_render_frame_draw_count is
// what the whole path exists to make true, so the test that draws four
// rectangles of four colours asserts that the frame held exactly one draw. A
// picture alone would look the same whether it took one draw or forty.
//
// Y RUNS DOWN AND THAT IS THE CLAIM WITH THE MOST WAYS TO BE ACCIDENTALLY
// RIGHT. Element space puts nought at the top; the engine's one Y flip puts +1
// in clip space at the top of the screen, so the negation in
// voe_render_element_transform is what reconciles them. A second flip, or none,
// leaves a picture that is upside down — which a symmetrical arrangement of
// rectangles would hide entirely. So the arrangement here is deliberately not
// symmetrical: three quadrants of three colours and the fourth left as the
// clear.
static void four_colours_in_one_draw(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;

	// Element space, millimetres, y down: (0,0) is the top-left corner.
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, HALF, HALF, RED)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(HALF, 0, HALF, HALF, GREEN)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, HALF, HALF, HALF, BLUE)));
	// The bottom-right quadrant is deliberately not submitted.

	VOE_TEST_CHECK(draw_everything(device));

	// THE CLAIM THIS CARD EXISTS TO PROVE, MEASURED. Three rectangles of
	// three different colours, and the frame holds one draw command.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);

	VOE_TEST_CHECK(close_frame(device));
	// Still one after the end: the count describes the frame that was just
	// submitted and is not cleared by ending it.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);

	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	// Each quadrant is exactly its own colour and nothing else is.
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, HALF, HALF, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, HALF, 0, SIDE, HALF, IS_GREEN),
			   QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, 0, HALF, HALF, SIDE, IS_BLUE),
			   QUADRANT);
	// The quadrant nothing was submitted for is untouched, and none of the
	// three colours reached it.
	VOE_TEST_CHECK_INT(count_in(image, HALF, HALF, SIDE, SIDE, NEITHER),
			   QUADRANT);

	// And no colour is anywhere but its own quadrant, which is what fails
	// when the picture is mirrored or turned over rather than merely wrong.
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_GREEN),
			   QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_BLUE),
			   QUADRANT);
}

// An element covering the whole surface, clipped to its top half. The bottom
// half has to come out as the clear — a clip test that compared the wrong pair
// of numbers, or one the fragment stage never ran, leaves the whole surface
// filled and every other check in this file still passing.
//
// THE CLIP RECTANGLE IS THE ONE FIELD WITH NO CALLER YET. An element covering
// the whole surface, clipped to its top half, has to come out as the top half
// and nothing else — untested surface being worse than absent surface.
static void the_clip_rectangle_clips(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;
	voe_render_element element = solid(0, 0, SIDE, SIDE, GREEN);

	// The top half of the surface, in the same millimetres the bounds are in.
	element.clip = (voe_math_float4){ 0.0f, 0.0f, SIDE, HALF };

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(device, element));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	// The top half is the element and the bottom half was never written, and
	// the clip rectangle is in element space, so "top" here is element y = 0
	// — the same direction the test above established.
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, HALF, IS_GREEN),
			   SIDE * HALF);
	VOE_TEST_CHECK_INT(count_in(image, 0, HALF, SIDE, SIDE, NEITHER),
			   SIDE * HALF);
}

// Two opaque elements over the whole surface, and the second one submitted is
// what is seen. Both ways round, because one order alone passes on an
// implementation that draws them backwards.
//
// ORDER IS PAINT ORDER, PROVEN BOTH WAYS ROUND. Two opaque elements over the
// whole surface, submitted in each order, and the second one submitted is what
// is seen. Asserting only one order would pass on an implementation that drew
// them in reverse.
static void order_is_paint_order(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, RED)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, GREEN)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_GREEN),
			   SIDE * SIDE);

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, GREEN)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, RED)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED),
			   SIDE * SIDE);
}

// A half-alpha green over an opaque red, and the green channel is what says
// whether the multiply happened exactly once.
//
// 188 IS THE sRGB ENCODING OF A HALF, and it is the whole test. Half of green
// over half of red is a linear 0.5 in both channels; the target's sRGB format
// encodes that as about 188. A shader that never multiplied by alpha would leave
// the green at 255 and one that multiplied twice would leave it near 137, so a
// window around 188 tells all three apart and nothing else has to be exact.
//
// AND THE BLEND IS PREMULTIPLIED, WHICH IS THE FAILURE THAT LOOKS LIKE A COLOUR
// SOMEBODY CHOSE. A half-alpha green over an opaque red is half of each: the
// green channel comes out around 188, which is the sRGB encoding of a half.
// Forgetting the multiply in the shader leaves it at 255 and doing it twice
// leaves it near 137, so one number separates all three.
static void the_blend_is_premultiplied(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *pixel;
	voe_math_float4 half_green = { 0.0f, 1.0f, 0.0f, 0.5f };

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, RED)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, half_green)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);

	// The middle of the picture, which every element above covers.
	pixel = (const unsigned char *)scene->pixels +
		((SIDE / 2) * SIDE + SIDE / 2) * 4;
	// BGRA. Both channels are a linear half and both should read about 188.
	VOE_TEST_CHECK(pixel[1] > 170 && pixel[1] < 205);
	VOE_TEST_CHECK(pixel[2] > 170 && pixel[2] < 205);
	// And nothing put anything in blue, which would mean the wrong record
	// was read.
	VOE_TEST_CHECK(pixel[0] < 64);
}

// The buffer holds four elements a frame. The fifth submit is refused, the four
// that fitted still draw, and the frame after it is an ordinary frame again —
// the specific failure this guards against is a refusal that leaves the count
// half advanced, which is a program that works until the frame it first runs out
// of room.
static void overrunning_is_refused_and_the_next_frame_is_fine(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	for (uint32_t i = 0; i < MAX_ELEMENTS; i++)
		VOE_TEST_CHECK(voe_render_frame_submit_element(
			device, solid(0, 0, SIDE, SIDE, RED)));
	// One too many.
	VOE_TEST_CHECK(!voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, GREEN)));
	// The four that fitted are still there and still draw, and the refusal
	// left the count where it was rather than one past it.
	VOE_TEST_CHECK_INT(device->element_count, MAX_ELEMENTS);
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED),
			   SIDE * SIDE);

	// And the next frame starts empty and draws its own thing, which is what
	// says the counter went back rather than staying full.
	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, GREEN)));
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_GREEN),
			   SIDE * SIDE);
}

// A mesh, then the elements, then a mesh again — and all three land. The quad
// is drawn into the top-left quadrant, one green element into the bottom-right,
// and the quad again shifted into the top-right; the three do not overlap, so
// each one's quadrant is an exact count.
//
// THE LAST DRAW IS THE ONE UNDER TEST. It comes after the element pipeline has
// been bound and after that pipeline pushed sixty-four bytes into the range a
// mesh draw pushes four into. If the two pipelines' layouts were incompatible
// the descriptor set would have been disturbed and this mesh would read
// nothing; if the element pipeline had bound buffers of its own the mesh would
// come out of the wrong one. Either way the top-right quadrant is what says so.
//
// AND THERE ARE TWO ELEMENT DRAWS AND NOT ONE, because a frame now holds one per
// surface. Each pushes its own sixty-four bytes immediately before its own draw,
// so the mesh that follows has had the push constant overwritten twice — and a
// pipeline that pushed once for several draws, or a mesh that read a stale push,
// is exactly what two of them turns from a possibility into a case.
//
// A MESH DRAWN AFTER AN ELEMENT DRAW IS STILL DRAWN RIGHT, WHICH IS THE CLAIM
// WITH NO PICTURE OF ITS OWN. The element pipeline shares the mesh pipelines'
// layout precisely so that the descriptor set frame.c binds once as a pass
// opens survives an element draw; two layouts differing in their push
// constant ranges would be incompatible and would disturb that set for
// everything drawn afterwards. The bound vertex and index buffers have to
// survive it too. So one test draws a mesh, then elements, then a mesh again,
// and all three have to land — and the failure it guards against shows up on
// the mesh drawn last, which no amount of looking at the elements would find.
static void a_mesh_after_an_element_draw_is_still_right(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;

	// The mesh, into the top-left quadrant.
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->quad,
					     shifted(0.0f, scene->red)));
	// The elements, into the bottom-right quadrant of element space, which
	// is the bottom-right of the picture because element y runs down. Two
	// records and two draws, one range each, so the quadrant is covered
	// twice by the same green — what is being counted here is the draws, not
	// which of them won.
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(HALF, HALF, HALF, HALF, GREEN)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(HALF, HALF, HALF, HALF, GREEN)));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target(), 0,
						      1));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target(), 1,
						      1));
	// And the mesh again, into the top-right quadrant. This is the draw the
	// test exists for.
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->quad,
					     shifted(1.0f, scene->blue)));

	// Two mesh draws and two element draws: four commands.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 4);

	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	VOE_TEST_CHECK_INT(count_in(image, 0, 0, HALF, HALF, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, HALF, 0, SIDE, HALF, IS_BLUE),
			   QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, HALF, HALF, SIDE, SIDE, IS_GREEN),
			   QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, 0, HALF, HALF, SIDE, NEITHER),
			   QUADRANT);
}

// Nothing submitted records no draw command, which is not a refusal: an
// interface with nothing in it this frame is not a caller that has gone wrong.
static void an_empty_frame_draws_nothing(struct scene *scene)
{
	voe_render_device *device = scene->device;

	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 0);
	VOE_TEST_CHECK(close_frame(device));
}

// A device opened with no element room refuses the submit rather than
// asserting, and the draw that follows records nothing.
static void no_element_room_is_a_refusal(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_capacities room = CAPACITIES;
	voe_render_device *device;
	voe_base_error error = VOE_BASE_OK;

	room.elements = 0;

	device = voe_render_device_new_headless(arena, size, room, &error);
	if (device == NULL)
		return;

	if (open_frame(device)) {
		VOE_TEST_CHECK(!voe_render_frame_submit_element(
			device, solid(0, 0, SIDE, SIDE, RED)));
		VOE_TEST_CHECK(draw_everything(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 0);
		VOE_TEST_CHECK(close_frame(device));
	}

	voe_render_device_destroy(device);
}

// TWO RANGES OF THE ONE BUFFER, TWO MATRICES, TWO DRAWS — the claim card 032
// added, and the one a wrong implementation gets wrong in the most plausible
// way: by ignoring `first` and drawing from the start of the buffer every time.
//
// THE TWO RANGES HOLD THE SAME TWO RECTANGLES ON PURPOSE. Both ranges put a
// rectangle in the top-left and the top-right of *their own* element space, so
// the only thing that can move them apart in the picture is the matrix. The
// second matrix is the first one shifted half the surface down in millimetres,
// so the second range lands in the bottom half — and a draw that ignored `first`
// would put the first range's red and blue down there instead of the second
// range's green, which every count below catches.
//
// A DRAW TAKES A RANGE, AND THE WAY TO GET THAT WRONG IS TO IGNORE IT. One
// buffer holds every surface a frame draws, so two surfaces are two ranges and
// two matrices out of the one buffer. The test that proves it puts the same two
// rectangles in both ranges and moves only the matrix: an implementation that
// drew from the start of the buffer every time would put the first range's
// colours where the second range's belong, and would otherwise look perfect.
static void two_ranges_two_matrices_two_draws(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;
	// The whole target, with element space slid down by half the surface
	// first. Composition reads right to left, so the translation happens to
	// the millimetres before the surface is mapped onto the target.
	voe_math_float4x4 lower = voe_math_float4x4_mul(
		whole_target(),
		voe_math_float4x4_from_translation(
			(voe_math_float3){ 0.0f, HALF, 0.0f }));

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;

	// The first surface: red at its top-left, blue at its top-right.
	VOE_TEST_CHECK_INT(voe_render_frame_elements_submitted(device), 0);
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, HALF, HALF, RED)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(HALF, 0, HALF, HALF, BLUE)));
	// The second surface: the same two places, both green.
	VOE_TEST_CHECK_INT(voe_render_frame_elements_submitted(device), 2);
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, HALF, HALF, GREEN)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(HALF, 0, HALF, HALF, GREEN)));
	VOE_TEST_CHECK_INT(voe_render_frame_elements_submitted(device), 4);

	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target(), 0,
						      2));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, lower, 2, 2));

	// Two ranges, two draw commands, and four rectangles between them.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 2);

	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	// The first range where its own matrix put it.
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, HALF, HALF, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, HALF, 0, SIDE, HALF, IS_BLUE),
			   QUADRANT);
	// And the second range where its own matrix put it, which is the half a
	// draw ignoring `first` would have filled with red and blue.
	VOE_TEST_CHECK_INT(count_in(image, 0, HALF, HALF, SIDE, IS_GREEN),
			   QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, HALF, HALF, SIDE, SIDE, IS_GREEN),
			   QUADRANT);
}

// A range past what was submitted is refused, and the frame is otherwise an
// ordinary frame: the elements that were submitted still draw, through a range
// that fits, and the count of draw commands never counted the refusal.
//
// IT IS A RETURNED FALSE AND NOT AN ASSERT BECAUSE ORDINARY STALENESS REACHES
// IT. A panel component holds last frame's range until something rewrites it,
// and the buffer is empty at the top of every frame, so a program that skipped
// a surface's rebuild for one frame arrives here — that costs one surface and
// must not cost the program.
//
// AND A RANGE THAT RUNS PAST WHAT WAS SUBMITTED IS REFUSED RATHER THAN
// ASSERTED, because a component holding last frame's range is how it is reached
// and that must cost one surface rather than the program. The frame carries on
// and the elements that were submitted still draw.
static void a_range_past_what_was_submitted_is_refused(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;

	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, HALF, HALF, RED)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(HALF, 0, HALF, HALF, BLUE)));

	// One past the end, counted from the start.
	VOE_TEST_CHECK(!voe_render_frame_draw_elements(device, whole_target(), 0,
						       3));
	// Inside the buffer but running off the end of it, which is the shape a
	// stale range actually has.
	VOE_TEST_CHECK(!voe_render_frame_draw_elements(device, whole_target(), 1,
						       2));
	// A first past the end with nothing asked for, which is the shape a
	// stale range has when the frame it was taken in held more elements
	// than this one does. It is still a refusal and not a quiet nothing.
	VOE_TEST_CHECK(!voe_render_frame_draw_elements(device, whole_target(), 3,
						       0));
	// Three refusals and no draw command among them.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 0);

	// And the frame is intact: what was submitted still draws.
	VOE_TEST_CHECK(draw_everything(device));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);

	VOE_TEST_CHECK(close_frame(device));
	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	VOE_TEST_CHECK_INT(count_in(image, 0, 0, HALF, HALF, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, HALF, 0, SIDE, HALF, IS_BLUE),
			   QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, 0, HALF, SIDE, SIDE, NEITHER),
			   QUADRANT * 2);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

	if (!open_scene(&scene, arena, CAPACITIES)) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// The mesh half, uploaded before any frame as a static range is, and used
	// by one test.
	{
		voe_render_vertex vertices[4];
		uint32_t indices[6];

		quad(vertices, indices);
		VOE_TEST_CHECK(voe_render_geometry_create(scene.device, vertices,
							  4, indices, 6,
							  &scene.quad, &error));
		VOE_TEST_CHECK(voe_render_shading_create(scene.device, MESH_RED,
							 &scene.red, &error));
		VOE_TEST_CHECK(voe_render_shading_create(scene.device, MESH_BLUE,
							 &scene.blue, &error));
	}

	if (scene.pixels != NULL) {
		four_colours_in_one_draw(&scene);
		the_clip_rectangle_clips(&scene);
		order_is_paint_order(&scene);
		the_blend_is_premultiplied(&scene);
		overrunning_is_refused_and_the_next_frame_is_fine(&scene);
		a_mesh_after_an_element_draw_is_still_right(&scene);
		two_ranges_two_matrices_two_draws(&scene);
		a_range_past_what_was_submitted_is_refused(&scene);
		an_empty_frame_draws_nothing(&scene);
	} else {
		VOE_TEST_CHECK(scene.pixels != NULL);
	}

	close_scene(&scene);

	no_element_room_is_a_refusal(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
