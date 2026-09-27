// The scene the element tests draw into: a headless device SIDE pixels square,
// a buffer the finished target is read back into, the record builders, the
// colour counts and a frame opened and closed around one pass. Included by
// tests/elements.c and tests/glyphs.c, each of which opens it with its own
// capacities in main() and closes it before it returns.
//
// IT IS A HEADER AND NOT A .c, because every tests/*.c is built as a program of
// its own: a shared .c would be a test with no main(). So every function here is
// `static inline` and every constant `[[maybe_unused]]`, and a program that
// leaves one unused still builds under -Werror.
//
// It includes render's internal header by relative path, as tests/pools.c and
// tests/transient.c do: reading a target back is not something the engine does
// and must not become part of its surface so that a test can see it.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO: open_scene() prints the
// skip and returns false, and the program returns its result at once.
#pragma once

#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float2.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 16
#define IMAGE_BYTES (SIDE * SIDE * 4)
#define HALF (SIDE / 2)
#define QUADRANT (HALF * HALF)

// The one function this test needs that render's own code never calls, resolved
// by hand for the reason tests/offscreen.c gives: reading an image back is a
// test's business and not surface to add to the table.
[[maybe_unused]]
static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static inline bool resolve_readback(voe_render_device *device)
{
	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
	return copy_image_to_buffer != NULL;
}

// The identity camera, so that a mesh's vertex is already in clip space and the
// quad below is exactly one quadrant. The element pipeline never reads binding 0
// at all; it is the mesh draws in the last test that read this.
static inline voe_render_view identity_camera(void)
{
	return (voe_render_view){
		.view = voe_math_float4x4_identity(),
		.projection = voe_math_float4x4_identity(),
	};
}

// Any sun: the records the mesh wears are unlit and the element pipeline has no
// lighting in it.
static inline voe_render_light no_sun(void)
{
	return (voe_render_light){
		.direction = { 0.0f, -1.0f, 0.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
}

// One rectangle, clipped to itself — which is what an element that is not meant
// to be clipped says, because a zeroed clip rect clips everything away.
static inline voe_render_element solid(float x, float y, float w, float h,
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
//
// THE SHEET IS A HAND-MADE FIELD AND NOT A FONT, because this folder does not
// depend on `text` and must not learn to. Four texels square, its bottom-right
// quarter the inside of the shape and the rest the outside, uploaded as DATA
// with FIELD sampling exactly as an atlas is. That is enough to say everything
// a letter would: where the interior is, where the paper is, and — because the
// pattern is in a corner rather than a stripe — which way round both axes of
// the sheet rectangle go. A sheet read mirrored in u or flipped in v puts the
// drawn quarter somewhere else and every count below fails.
#define FIELD_SIDE 4
#define FIELD_BYTES (FIELD_SIDE * FIELD_SIDE * 4)

static inline void field_texels(unsigned char rgba[FIELD_BYTES])
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
[[maybe_unused]]
static const voe_math_float4 SHEET_WHOLE = { 0.0f, 0.0f, 1.0f, 1.0f };
// A piece of the sheet that is entirely inside the shape — well past the texel
// centres at 0.625 in both axes — so that the element comes out as a full
// rectangle. That is what the clip test and the paint-order tests want: a glyph
// whose coverage is not itself the thing under test.
[[maybe_unused]]
static const voe_math_float4 SHEET_INSIDE = { 0.7f, 0.7f, 0.25f, 0.25f };

// One glyph element, clipped to itself for the reason solid() is.
static inline voe_render_element glyph(float x, float y, float w, float h,
				       voe_math_float4 colour,
				       uint32_t sheet_texture,
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

[[maybe_unused]]
static const voe_math_float4 RED = { 1.0f, 0.0f, 0.0f, 1.0f };
[[maybe_unused]]
static const voe_math_float4 GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
[[maybe_unused]]
static const voe_math_float4 BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
// The surface is the whole target and one millimetre is one pixel.
//
// ONE MILLIMETRE IS ONE PIXEL HERE, on purpose: the surface is handed the
// target's size in millimetres, so every count below is exact rather than a
// threshold and a rectangle's edges land on pixel boundaries.
static inline voe_math_float4x4 whole_target(void)
{
	return voe_render_element_transform((voe_math_float2){ SIDE, SIDE });
}

// Everything submitted to this frame, over the whole target — which is what
// every test here means except the two about ranges.
//
// IT IS A HELPER AND NOT A RANGE SPELLED OUT AT EACH CALL, on purpose. A dozen
// calls each carrying `0, voe_render_frame_elements_submitted(device)` would say
// the same thing a dozen times and would bury the two tests where the range is
// the claim. What a test means by this call is "all of it", so that is what it
// says, and a range appears in this file only where it is being tested.
[[nodiscard]] static inline bool draw_everything(voe_render_device *device)
{
	return voe_render_frame_draw_elements(
		device, whole_target(), 0,
		voe_render_frame_elements_submitted(device));
}

// Copies the slot's finished target into `buffer`, by a command buffer of this
// file's own, after waiting for the device to go idle — the shape
// tests/transient.c uses, for the reasons its header gives.
static inline void read_back(voe_render_device *device,
			     const struct voe_render_frame *frame,
			     VkBuffer buffer)
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
static inline int colour_of(const unsigned char *pixel)
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
static inline int count_in(const unsigned char *image, int x0, int y0,
			   int x1, int y1, int colour)
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
	// The mesh half, for elements.c's mesh claim only.
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

// A frame and one pass onto the window, which is where every draw here goes.
// With a camera, because the mesh draws in one of the tests need it.
static inline bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = { .view = identity_camera(),
					  .light = no_sun() };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return false;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	return true;
}

// The pass open_frame opened, and then the frame.
static inline bool close_frame(voe_render_device *device)
{
	voe_render_pass_end(device);
	return voe_render_frame_end(device);
}

// Opens the headless device with `capacities`, resolves the readback and maps
// the buffer it copies into. False means main() returns voe_test_result() at
// once: a machine with no usable Vulkan has printed its skip, anything else has
// failed a check, and nothing is left open but the arena. True with no pixels
// is a failed map, checked here and left for main() to see.
static inline bool open_scene(struct scene *scene, voe_base_arena *arena,
			      voe_render_capacities capacities)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;

	scene->device = voe_render_device_new_headless(arena, size, capacities,
						       &error);
	if (scene->device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			return false;
		}
		VOE_TEST_CHECK(scene->device != NULL);
		return false;
	}

	// Every count reads bytes as BGRA; see colour_of.
	VOE_TEST_CHECK_INT(scene->device->format.format,
			   VK_FORMAT_B8G8R8A8_SRGB);

	if (!resolve_readback(scene->device)) {
		VOE_TEST_CHECK(false);
		voe_render_device_destroy(scene->device);
		return false;
	}

	VOE_TEST_CHECK(voe_render_buffer_build(
		scene->device, &scene->readback, IMAGE_BYTES,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (scene->readback.buffer == VK_NULL_HANDLE) {
		voe_render_device_destroy(scene->device);
		return false;
	}
	// Mapped once for the whole run: every read_back waits for idle before
	// the pixels are looked at, and the memory is coherent.
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(scene->device->device,
						    scene->readback.memory, 0,
						    VK_WHOLE_SIZE, 0,
						    &scene->pixels),
			   VK_SUCCESS);
	return true;
}

// Everything open_scene opened, in the order it has to go.
static inline void close_scene(struct scene *scene)
{
	voe_render_vk.device_wait_idle(scene->device->device);
	if (scene->pixels != NULL)
		voe_render_vk.unmap_memory(scene->device->device,
					   scene->readback.memory);
	voe_render_buffer_teardown(scene->device, &scene->readback);
	voe_render_device_destroy(scene->device);
}

