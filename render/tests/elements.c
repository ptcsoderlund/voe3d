// The element path: many rectangles from many small records, and a letter among
// them, all in one draw command. Twelve claims, each of them one a wrong
// implementation would get wrong quietly.
//
// IT READS THE PICTURE BACK, because nothing about this path can be checked any
// other way. A submit that wrote at the wrong offset, a vertex shader that built
// the corners the wrong way round, a clip test comparing the wrong pair of
// numbers and a blend that multiplied twice all leave the bookkeeping perfect.
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
//
// THE CLIP RECTANGLE IS THE ONE FIELD WITH NO CALLER YET. An element covering
// the whole surface, clipped to its top half, has to come out as the top half
// and nothing else — untested surface being worse than absent surface.
//
// ORDER IS PAINT ORDER, PROVEN BOTH WAYS ROUND. Two opaque elements over the
// whole surface, submitted in each order, and the second one submitted is what
// is seen. Asserting only one order would pass on an implementation that drew
// them in reverse.
//
// A MESH DRAWN AFTER AN ELEMENT DRAW IS STILL DRAWN RIGHT, WHICH IS THE CLAIM
// WITH NO PICTURE OF ITS OWN. The element pipeline shares the mesh pipelines'
// layout precisely so that the descriptor set frame.c binds once at the top of
// the frame survives an element draw; two layouts differing in their push
// constant ranges would be incompatible and would disturb that set for
// everything drawn afterwards. The bound vertex and index buffers have to
// survive it too. So one test draws a mesh, then elements, then a mesh again,
// and all three have to land — and the failure it guards against shows up on
// the mesh drawn last, which no amount of looking at the elements would find.
//
// A LETTER AND A FILL ARE ONE DRAW COMMAND, WHICH IS WHAT THE GLYPH KIND EXISTS
// TO SAY. A solid and a glyph in one frame, and the frame holds one draw. That
// is the headline claim and it is the assert, not the picture, that proves it —
// two kinds drawn correctly in two draws would look identical.
//
// THE SHEET IS A HAND-MADE FIELD AND NOT A FONT, because this folder does not
// depend on `text` and must not learn to. Four texels square, its bottom-right
// quarter the inside of the shape and the rest the outside, uploaded as DATA
// with FIELD sampling exactly as an atlas is. That is enough to say everything
// a letter would: where the interior is, where the paper is, and — because the
// pattern is in a corner rather than a stripe — which way round both axes of
// the sheet rectangle go. A sheet read mirrored in u or flipped in v puts the
// drawn quarter somewhere else and every count below fails.
//
// THE COUNTS SKIP THE BAND WHERE THE FIELD CROSSES. FIELD sampling is linear by
// design (see voe_render_sampling), so between the outside texels and the
// inside ones there is a strip where the interpolated value is passing through
// a half and which side of the threshold a pixel lands on is arithmetic on a
// texel centre rather than a claim worth making. The regions counted are well
// inside and well outside it, which is where the answer is exact.
//
// AND A GLYPH THAT NAMED NO SHEET DRAWS A SOLID RECTANGLE, ASSERTED SO IT
// CANNOT CHANGE QUIETLY. VOE_RENDER_NO_TEXTURE is slot 0 and slot 0 is one
// white pixel, so such a record medians to white, thresholds to one and comes
// out as a plausible-looking rectangle rather than as anything that fails. It
// is the worst failure on this path to find by looking, so it is the one with a
// test naming it.
//
// AND THE BLEND IS PREMULTIPLIED, WHICH IS THE FAILURE THAT LOOKS LIKE A COLOUR
// SOMEBODY CHOSE. A half-alpha green over an opaque red is half of each: the
// green channel comes out around 188, which is the sRGB encoding of a half.
// Forgetting the multiply in the shader leaves it at 255 and doing it twice
// leaves it near 137, so one number separates all three.
//
// ONE MILLIMETRE IS ONE PIXEL HERE, on purpose: the surface is handed the
// target's size in millimetres, so every count below is exact rather than a
// threshold and a rectangle's edges land on pixel boundaries.
//
// It includes render's internal header by relative path, as tests/pools.c and
// tests/transient.c do: reading a target back is not something the engine does
// and must not become part of its surface so that a test can see it.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float2.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SIDE 16
#define IMAGE_BYTES (SIDE * SIDE * 4)
#define HALF (SIDE / 2)
#define QUADRANT (HALF * HALF)

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
};

// The one function this test needs that render's own code never calls, resolved
// by hand for the reason tests/offscreen.c gives: reading an image back is a
// test's business and not surface to add to the table.
static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static bool resolve_readback(voe_render_device *device)
{
	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
	return copy_image_to_buffer != NULL;
}

// The identity camera, so that a mesh's vertex is already in clip space and the
// quad below is exactly one quadrant. The element pipeline never reads binding 0
// at all; it is the mesh draws in the last test that read this.
static voe_render_view identity_camera(void)
{
	return (voe_render_view){
		.view = voe_math_float4x4_identity(),
		.projection = voe_math_float4x4_identity(),
	};
}

// Any sun: the records the mesh wears are unlit and the element pipeline has no
// lighting in it.
static voe_render_light no_sun(void)
{
	return (voe_render_light){
		.direction = { 0.0f, -1.0f, 0.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
}

// One rectangle, clipped to itself — which is what an element that is not meant
// to be clipped says, because a zeroed clip rect clips everything away.
static voe_render_element solid(float x, float y, float w, float h,
				voe_math_float4 colour)
{
	return (voe_render_element){
		.bounds = { x, y, w, h },
		.clip = { x, y, w, h },
		.colour = colour,
		.kind = VOE_RENDER_ELEMENT_SOLID,
	};
}

// The sheet: four texels square, all three channels the same number so that the
// median is that number, and the bottom-right two-by-two the inside of the
// shape. The alpha channel is opaque and is not read, exactly as a real atlas's
// is not.
#define FIELD_SIDE 4
#define FIELD_BYTES (FIELD_SIDE * FIELD_SIDE * 4)

static void field_texels(unsigned char rgba[FIELD_BYTES])
{
	for (int y = 0; y < FIELD_SIDE; y++) {
		for (int x = 0; x < FIELD_SIDE; x++) {
			unsigned char inside = (x >= FIELD_SIDE / 2 &&
						y >= FIELD_SIDE / 2) ? 255 : 0;
			unsigned char *at = rgba + (y * FIELD_SIDE + x) * 4;

			at[0] = inside;
			at[1] = inside;
			at[2] = inside;
			at[3] = 255;
		}
	}
}

// The whole sheet, so that the element shows the corner pattern and says which
// way round both axes go.
static const voe_math_float4 SHEET_WHOLE = { 0.0f, 0.0f, 1.0f, 1.0f };
// A piece of the sheet that is entirely inside the shape — well past the texel
// centres at 0.625 in both axes — so that the element comes out as a full
// rectangle. That is what the clip test and the paint-order tests want: a glyph
// whose coverage is not itself the thing under test.
static const voe_math_float4 SHEET_INSIDE = { 0.7f, 0.7f, 0.25f, 0.25f };

// One glyph element, clipped to itself for the reason solid() is.
static voe_render_element glyph(float x, float y, float w, float h,
				voe_math_float4 colour, uint32_t sheet_texture,
				voe_math_float4 sheet)
{
	return (voe_render_element){
		.bounds = { x, y, w, h },
		.clip = { x, y, w, h },
		.colour = colour,
		.kind = VOE_RENDER_ELEMENT_GLYPH,
		.sheet_texture = sheet_texture,
		.sheet = sheet,
	};
}

static const voe_math_float4 RED = { 1.0f, 0.0f, 0.0f, 1.0f };
static const voe_math_float4 GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
static const voe_math_float4 BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };

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
	};
}

// The surface is the whole target and one millimetre is one pixel.
static voe_math_float4x4 whole_target(void)
{
	return voe_render_element_transform((voe_math_float2){ SIDE, SIDE });
}

// Copies the slot's finished target into `buffer`, by a command buffer of this
// file's own, after waiting for the device to go idle — the shape
// tests/transient.c uses, for the reasons its header gives.
static void read_back(voe_render_device *device,
		      const struct voe_render_frame *frame, VkBuffer buffer)
{
	VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commands = VK_NULL_HANDLE;
	VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	VkBufferImageCopy region = {
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.layerCount = 1,
		},
		.imageExtent = { SIDE, SIDE, 1 },
	};
	VkMemoryBarrier2 visible = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
		.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
	};
	VkDependencyInfo dependency = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &visible,
	};
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};

	voe_render_vk.device_wait_idle(device->device);

	VOE_TEST_CHECK_INT(voe_render_vk.allocate_command_buffers(device->device,
								  &allocate,
								  &commands),
			   VK_SUCCESS);
	if (commands == VK_NULL_HANDLE)
		return;

	voe_render_vk.begin_command_buffer(commands, &begin);
	copy_image_to_buffer(commands, frame->target.colour.image,
			     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1,
			     &region);
	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
	voe_render_vk.end_command_buffer(commands);

	submit_commands.commandBuffer = commands;
	VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1, &submit,
						       VK_NULL_HANDLE),
			   VK_SUCCESS);
	voe_render_vk.device_wait_idle(device->device);
	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
}

// Which of the three primaries a pixel mostly is, or neither: the clear colour
// is a dark blue with no red or green worth counting, so BLUE is the one that
// has to be told from it and is tested on its blue channel being high while the
// other two are near nothing.
#define NEITHER 0
#define IS_RED 1
#define IS_GREEN 2
#define IS_BLUE 3

// BGRA, which is the headless device's format — main() asserts it.
static int colour_of(const unsigned char *pixel)
{
	unsigned char blue = pixel[0];
	unsigned char green = pixel[1];
	unsigned char red = pixel[2];

	if (red > 128 && red > green && red > blue)
		return IS_RED;
	if (green > 128 && green > red && green > blue)
		return IS_GREEN;
	// The clear is a dark blue, so a high blue channel alone does not say
	// this is a blue element; the clear's blue is about 95 of 255 once the
	// target's sRGB format has encoded it.
	if (blue > 200 && red < 64 && green < 64)
		return IS_BLUE;
	return NEITHER;
}

// How many pixels of `colour` inside the rectangle [x0, x1) x [y0, y1), in
// framebuffer coordinates — y0 is the top row, because that is how the bytes
// come back.
static int count_in(const unsigned char *image, int x0, int y0, int x1, int y1,
		    int colour)
{
	int count = 0;

	for (int y = y0; y < y1; y++)
		for (int x = x0; x < x1; x++)
			if (colour_of(image + (y * SIDE + x) * 4) == colour)
				count++;
	return count;
}

struct scene {
	voe_render_device *device;
	// The mesh half, for the last test only.
	voe_render_geometry quad;
	voe_render_shading red;
	voe_render_shading blue;
	// The hand-made field the glyph tests read.
	voe_render_texture sheet;
	struct voe_render_buffer readback;
	// vkMapMemory hands its pointer back through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	void *pixels;
};

static bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, identity_camera(),
					      no_sun(), &drawing));
	VOE_TEST_CHECK(drawing);
	return drawing;
}

// Three quadrants of three colours and the fourth left as the clear, drawn by
// one draw command, and the arrangement is asymmetrical so that a picture the
// wrong way up or the wrong way round cannot pass.
//
// RED IS TOP-LEFT IN ELEMENT SPACE AND HAS TO BE TOP-LEFT IN THE PICTURE, which
// is the whole of the Y-direction claim: element y = 0 is the top.
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

	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));

	// THE CLAIM THIS CARD EXISTS TO PROVE, MEASURED. Three rectangles of
	// three different colours, and the frame holds one draw command.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);

	VOE_TEST_CHECK(voe_render_frame_end(device));
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
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
	// is the bottom-right of the picture because element y runs down.
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(HALF, HALF, HALF, HALF, GREEN)));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	// And the mesh again, into the top-right quadrant. This is the draw the
	// test exists for.
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->quad,
					     shifted(1.0f, scene->blue)));

	// Two mesh draws and one element draw: three commands, and the element
	// draw is the one that held more than one thing.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 3);

	VOE_TEST_CHECK(voe_render_frame_end(device));
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

// THE CARD'S HEADLINE CLAIM: a rectangle and a letter are the same draw command.
// A solid in the top-left quadrant and a glyph in the bottom-right, and the
// frame holds exactly one draw. The picture is checked as well, because a draw
// count of one over a frame that drew nothing would also be one.
static void a_solid_and_a_glyph_are_one_draw(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;

	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, HALF, HALF, RED)));
	// A piece of the sheet that is all interior, so this glyph is a full
	// quadrant of the record's colour and the count is exact.
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(HALF, HALF, HALF, HALF, GREEN,
			      scene->sheet.index, SHEET_INSIDE)));

	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	// ONE. Two kinds, one buffer, one draw.
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	VOE_TEST_CHECK(voe_render_frame_end(device));

	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	VOE_TEST_CHECK_INT(count_in(image, 0, 0, HALF, HALF, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, HALF, HALF, SIDE, SIDE, IS_GREEN),
			   QUADRANT);
	// And neither kind wrote anywhere but its own quadrant.
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_RED), QUADRANT);
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, SIDE, IS_GREEN),
			   QUADRANT);
}

// The interior of the shape is the record's colour and the paper outside it is
// not. One glyph over the whole surface reading the whole sheet, so the picture
// is the sheet's own corner pattern: the bottom-right quarter drawn and the
// other three not.
//
// IT IS ALSO THE ONE TEST THAT SAYS WHICH WAY ROUND THE SHEET RECTANGLE GOES.
// Read mirrored in u the drawn quarter is on the left; read flipped in v it is
// at the top. Both fail here and neither would fail on a stripe.
static void a_glyph_reads_the_sheet(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;
	// Well inside the sheet's inner quarter and well outside it, skipping
	// the band where the linear field crosses the threshold. See the header.
	const int low = 6;
	const int high = 10;
	const int corner = low * low;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, 0, SIDE, SIDE, GREEN, scene->sheet.index,
			      SHEET_WHOLE)));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	// The interior: the record's colour, and every pixel of it.
	VOE_TEST_CHECK_INT(count_in(image, high, high, SIDE, SIDE, IS_GREEN),
			   corner);
	// The paper: three corners of it, none of them written at all. A
	// threshold that let the outside through would fill these.
	VOE_TEST_CHECK_INT(count_in(image, 0, 0, low, low, NEITHER), corner);
	VOE_TEST_CHECK_INT(count_in(image, high, 0, SIDE, low, NEITHER),
			   corner);
	VOE_TEST_CHECK_INT(count_in(image, 0, high, low, SIDE, NEITHER),
			   corner);
}

// A glyph is clipped exactly as a solid is, and the same way round: an element
// over the whole surface, drawing everywhere the sheet is concerned, clipped to
// its top half. This is what a scroll area will be made of.
static void a_glyph_is_clipped_like_a_solid(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	const unsigned char *image;
	voe_render_element element = glyph(0, 0, SIDE, SIDE, GREEN,
					   scene->sheet.index, SHEET_INSIDE);

	element.clip = (voe_math_float4){ 0.0f, 0.0f, SIDE, HALF };

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(device, element));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);
	image = scene->pixels;

	VOE_TEST_CHECK_INT(count_in(image, 0, 0, SIDE, HALF, IS_GREEN),
			   SIDE * HALF);
	VOE_TEST_CHECK_INT(count_in(image, 0, HALF, SIDE, SIDE, NEITHER),
			   SIDE * HALF);
}

// A glyph naming no sheet. VOE_RENDER_NO_TEXTURE is slot 0 and slot 0 is one
// white pixel: the median of white is white, the threshold passes it, and the
// element comes out as a solid rectangle of its own colour.
//
// THIS TEST IS HERE BECAUSE THAT FAILURE LOOKS LIKE SUCCESS. A record that
// forgot its texture index draws a plausible rectangle rather than anything
// wrong, so what the empty id does is written down in voe_render_element and
// asserted here, and changing it means changing this line on purpose.
static void a_glyph_with_no_sheet_draws_a_solid_rectangle(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, 0, SIDE, SIDE, RED, VOE_RENDER_NO_TEXTURE,
			      SHEET_WHOLE)));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED),
			   SIDE * SIDE);
}

// Paint order holds across the two kinds and not only within one: a solid over
// a glyph and a glyph over a solid, and in both the second one submitted is
// what is seen. One order alone would pass on an implementation that sorted by
// kind, which is exactly the thing an interface must not have done to it.
static void paint_order_holds_across_kinds(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, 0, SIDE, SIDE, GREEN, scene->sheet.index,
			      SHEET_INSIDE)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, RED)));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_RED),
			   SIDE * SIDE);

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, solid(0, 0, SIDE, SIDE, RED)));
	VOE_TEST_CHECK(voe_render_frame_submit_element(
		device, glyph(0, 0, SIDE, SIDE, GREEN, scene->sheet.index,
			      SHEET_INSIDE)));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);
	VOE_TEST_CHECK_INT(count_in(scene->pixels, 0, 0, SIDE, SIDE, IS_GREEN),
			   SIDE * SIDE);
}

// Nothing submitted records no draw command, which is not a refusal: an
// interface with nothing in it this frame is not a caller that has gone wrong.
static void an_empty_frame_draws_nothing(struct scene *scene)
{
	voe_render_device *device = scene->device;

	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_draw_elements(device, whole_target()));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 0);
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
		VOE_TEST_CHECK(voe_render_frame_draw_elements(
			device, whole_target()));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 0);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	voe_render_device_destroy(device);
}

// The transform, on its own and without a graphics card: the three corners of
// element space that say which way round it is. It runs whether or not there is
// a Vulkan device, because it is arithmetic.
//
// IT IS CHECKED HERE RATHER THAN INFERRED FROM THE PICTURE because the picture
// is what the whole pipeline did and this is the one line that decides the
// direction. Both together say the direction is right and that nothing else
// undid it.
static void the_transform_puts_the_origin_at_the_top_left(void)
{
	voe_math_float4x4 m = voe_render_element_transform(
		(voe_math_float2){ 100.0f, 50.0f });
	voe_math_float4 origin = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 0.0f, 0.0f, 0.0f, 1.0f });
	voe_math_float4 far_corner = voe_math_float4x4_mul_float4(
		m, (voe_math_float4){ 100.0f, 50.0f, 0.0f, 1.0f });

	// (0,0) millimetres is the left edge and the TOP of the screen, and with
	// the engine's negative viewport height the top of the screen is clip
	// y = +1. That +1 is the Y-down convention and there is no other flip.
	VOE_TEST_CHECK_FLOAT(origin.x, -1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(origin.y, 1.0f, 1e-5f);
	// The far corner is the right edge and the bottom.
	VOE_TEST_CHECK_FLOAT(far_corner.x, 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(far_corner.y, -1.0f, 1e-5f);
	// The near plane, because depth runs backwards here, and w is one
	// because an element surface is not projected.
	VOE_TEST_CHECK_FLOAT(origin.z, 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(origin.w, 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(far_corner.w, 1.0f, 1e-5f);
}

// Eighty bytes, said out loud here and not only in render/src/descriptors.c.
// The static asserts over there are the safety net and they fire at build time;
// this is the claim stated where a person reading the tests can see it, because
// the size is the promise the glyph kind was written to keep — every draw ever
// built against this record changes with it.
//
// It runs whether or not there is a graphics card, because it is arithmetic.
static void the_record_is_still_eighty_bytes(void)
{
	VOE_TEST_CHECK_INT((int)sizeof(voe_render_element), 80);
	// And the two words the glyph kind took are where the shader has them,
	// with two still spare after the first of them.
	VOE_TEST_CHECK_INT((int)offsetof(voe_render_element, kind), 48);
	VOE_TEST_CHECK_INT((int)offsetof(voe_render_element, sheet_texture), 52);
	VOE_TEST_CHECK_INT((int)offsetof(voe_render_element, sheet), 64);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

	the_transform_puts_the_origin_at_the_top_left();
	the_record_is_still_eighty_bytes();

	scene.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						      &error);
	if (scene.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(scene.device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	// Every count below reads bytes as BGRA; see colour_of.
	VOE_TEST_CHECK_INT(scene.device->format.format, VK_FORMAT_B8G8R8A8_SRGB);

	if (!resolve_readback(scene.device)) {
		VOE_TEST_CHECK(false);
		voe_render_device_destroy(scene.device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_render_buffer_build(
		scene.device, &scene.readback, IMAGE_BYTES,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (scene.readback.buffer == VK_NULL_HANDLE) {
		voe_render_device_destroy(scene.device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	// Mapped once for the whole run: every read_back below waits for idle
	// before the pixels are looked at, and the memory is coherent.
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(scene.device->device,
						    scene.readback.memory, 0,
						    VK_WHOLE_SIZE, 0,
						    &scene.pixels),
			   VK_SUCCESS);

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

	// The sheet, uploaded exactly as text/src/font.c uploads its atlas:
	// DATA because the texels are numbers rather than colour, and FIELD
	// because a field is reconstructed by filtering and not read as a grid.
	// Neither is optional and getting either wrong changes where the edge
	// lands.
	{
		unsigned char texels[FIELD_BYTES];

		field_texels(texels);
		VOE_TEST_CHECK(voe_render_texture_create(
			scene.device, VOE_RENDER_TEXTURE_DATA,
			VOE_RENDER_SAMPLING_FIELD, FIELD_SIDE, FIELD_SIDE,
			texels, &scene.sheet, &error));
	}

	if (scene.pixels != NULL) {
		four_colours_in_one_draw(&scene);
		the_clip_rectangle_clips(&scene);
		order_is_paint_order(&scene);
		the_blend_is_premultiplied(&scene);
		overrunning_is_refused_and_the_next_frame_is_fine(&scene);
		a_mesh_after_an_element_draw_is_still_right(&scene);
		a_solid_and_a_glyph_are_one_draw(&scene);
		a_glyph_reads_the_sheet(&scene);
		a_glyph_is_clipped_like_a_solid(&scene);
		a_glyph_with_no_sheet_draws_a_solid_rectangle(&scene);
		paint_order_holds_across_kinds(&scene);
		an_empty_frame_draws_nothing(&scene);
	} else {
		VOE_TEST_CHECK(scene.pixels != NULL);
	}

	voe_render_vk.device_wait_idle(scene.device->device);
	if (scene.pixels != NULL)
		voe_render_vk.unmap_memory(scene.device->device,
					   scene.readback.memory);
	voe_render_buffer_teardown(scene.device, &scene.readback);
	voe_render_device_destroy(scene.device);

	no_element_room_is_a_refusal(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
