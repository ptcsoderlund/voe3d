// A frame as a sequence of passes onto the window, read back out of the picture.
// Four claims, each the one a wrong implementation of the clear rule, the
// per-pass camera or the capacity would get wrong without failing anything else.
//
// A SECOND PASS LOADS WHAT THE FIRST LEFT, COLOUR AND DEPTH BOTH. The first pass
// draws a near red quad over the left half and a far red quad over the right;
// the second draws one green quad over the whole target, between the two in
// depth. The left half has to stay red — the depth the first pass wrote hides
// the green, which only happens if depth was loaded rather than cleared — and
// the right half has to turn green, drawn over what the first pass left. A
// second pass that cleared colour loses the red; one that cleared depth paints
// the left half green; one that did not draw leaves the right half red.
//
// AND THE SECOND PASS READS ITS OWN CAMERA. The green quad spans x from -2 to 0
// and the second pass's view moves it one unit right, onto the whole target.
// Drawn with the first pass's identity camera instead — a dynamic offset that
// did not move — it covers only the left half and the right half stays red.
//
// A FRAME THAT OPENS NO PASS STILL PRESENTS THE CLEAR COLOUR. The same slot is
// drawn into with a red half first, then comes round again with no pass at all;
// every pixel of its target has to be the clear colour that frame one left in
// its right half, not the red that was there before.
//
// THE PASS CAPACITY IS A RETURNED REFUSAL AND IT IS PER FRAME. A device with room
// for one pass refuses the second in a frame, still ends that frame, and opens
// the first pass of the next one.
//
// A PASS WITH NO CAMERA DRAWS ELEMENTS. A solid red element over the left half,
// in a pass opened with NULL, lands exactly on the left half.
//
// IDENTITY CAMERA, UNLIT RECORDS, AS tests/transient.c HAS THEM. A vertex is
// already in clip space, so a quad from x = -1 to x = 0 is exactly the left
// half, and its z is its depth: larger is nearer, under the GREATER test this
// engine runs.
//
// It includes render's internal header by relative path, as the other headless
// tests do: reading a target back is not something the engine does.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define SIDE 16
#define IMAGE_BYTES (SIDE * SIDE * 4)

// The element surface is this many millimetres square, stretched over the
// target; half of it is exactly half the target.
#define SURFACE_MM 160.0f

// Three quads, two passes. The depths are the three the header names.
#define NEAR_DEPTH 0.75f
#define MIDDLE_DEPTH 0.5f
#define FAR_DEPTH 0.25f

static const voe_render_capacities CAPACITIES = {
	.vertices = 12,
	.indices = 18,
	.geometries = 3,
	.objects = 4,
	.shadings = 2,
	.elements = 1,
	.passes = 2,
};

static const voe_render_shading_values RED = {
	.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.unlit = 1,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};
static const voe_render_shading_values GREEN = {
	.base_colour = { 0.0f, 1.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.unlit = 1,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// Resolved by hand for the reason tests/offscreen.c gives.
static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static bool resolve_readback(voe_render_device *device)
{
	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
	return copy_image_to_buffer != NULL;
}

// A quad from x0 to x1 across the whole height at depth z. Corner for corner the
// shape tests/transient.c uses, which is a front face.
static void quad(float x0, float x1, float z, voe_render_vertex vertices[4],
		 uint32_t indices[6])
{
	vertices[0] = (voe_render_vertex){ { x0, 1.0f, z },
					   { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 0.0f } };
	vertices[1] = (voe_render_vertex){ { x1, 1.0f, z },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 0.0f } };
	vertices[2] = (voe_render_vertex){ { x1, -1.0f, z },
					   { 0.0f, 0.0f, 1.0f },
					   { 1.0f, 1.0f } };
	vertices[3] = (voe_render_vertex){ { x0, -1.0f, z },
					   { 0.0f, 0.0f, 1.0f },
					   { 0.0f, 1.0f } };
	indices[0] = 3;
	indices[1] = 2;
	indices[2] = 1;
	indices[3] = 3;
	indices[4] = 1;
	indices[5] = 0;
}

// The identity camera, moved `shift` along x. Row-major, so the translation is
// the last column of the first row.
static voe_render_pass_camera camera_shifted(float shift)
{
	voe_render_pass_camera camera = {
		.view = {
			.view = voe_math_float4x4_identity(),
			.projection = voe_math_float4x4_identity(),
		},
		// Any light: the records are unlit and never read it.
		.light = {
			.direction = { 0.0f, -1.0f, 0.0f },
			.intensity = 1.0f,
			.colour = { 1.0f, 1.0f, 1.0f },
		},
	};

	camera.view.view.m[0][3] = shift;
	return camera;
}

static voe_render_object wearing(voe_render_shading shading)
{
	return (voe_render_object){
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// Copies the slot's finished target into `buffer` — the shape tests/transient.c
// uses, for the reasons tests/offscreen.c gives.
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

#define NEITHER 0
#define IS_RED 1
#define IS_GREEN 2

// BGRA, which is the headless device's format — main() asserts it. The clear
// colour is near black and counts as neither.
static int colour_of(const unsigned char *pixel)
{
	unsigned char blue = pixel[0];
	unsigned char green = pixel[1];
	unsigned char red = pixel[2];

	if (red > 128 && red > green && red > blue)
		return IS_RED;
	if (green > 128 && green > red && green > blue)
		return IS_GREEN;
	return NEITHER;
}

static int count_in_columns(const unsigned char *image, int x0, int x1,
			    int colour)
{
	int count = 0;

	for (int y = 0; y < SIDE; y++)
		for (int x = x0; x < x1; x++)
			if (colour_of(image + (y * SIDE + x) * 4) == colour)
				count++;
	return count;
}

// The halves are the quads' exact footprints, so the counts are exact.
#define HALF (SIDE * SIDE / 2)

static void expect_halves(const unsigned char *image, int left, int right)
{
	VOE_TEST_CHECK_INT(count_in_columns(image, 0, SIDE / 2, IS_RED),
			   left == IS_RED ? HALF : 0);
	VOE_TEST_CHECK_INT(count_in_columns(image, 0, SIDE / 2, IS_GREEN),
			   left == IS_GREEN ? HALF : 0);
	VOE_TEST_CHECK_INT(count_in_columns(image, SIDE / 2, SIDE, IS_RED),
			   right == IS_RED ? HALF : 0);
	VOE_TEST_CHECK_INT(count_in_columns(image, SIDE / 2, SIDE, IS_GREEN),
			   right == IS_GREEN ? HALF : 0);
}

struct scene {
	voe_render_device *device;
	voe_render_shading red;
	voe_render_shading green;
	voe_render_geometry near_left;
	voe_render_geometry far_right;
	// x from -2 to 0, which only the second pass's camera puts over the
	// whole target.
	voe_render_geometry middle_shifted;
	struct voe_render_buffer readback;
	// vkMapMemory hands its pointer back through a void **, which is
	// Vulkan's signature and not one this engine gets to choose.
	void *pixels;
};

static bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	return drawing;
}

static void a_second_pass_loads_colour_and_depth(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_render_pass_camera first = camera_shifted(0.0f);
	voe_render_pass_camera second = camera_shifted(1.0f);

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;

	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &first));
	VOE_TEST_CHECK(voe_render_pass_is_open(device));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->near_left,
					     wearing(scene->red)));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->far_right,
					     wearing(scene->red)));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(!voe_render_pass_is_open(device));

	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &second));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->middle_shifted,
					     wearing(scene->green)));
	voe_render_pass_end(device);

	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);
	expect_halves(scene->pixels, IS_RED, IS_GREEN);
}

static void a_frame_with_no_pass_presents_the_clear(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_render_pass_camera camera = camera_shifted(0.0f);
	unsigned char clear[4];

	// Frame one: red over the left half, and the clear colour in the right
	// half to compare against afterwards.
	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->near_left,
					     wearing(scene->red)));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);
	expect_halves(scene->pixels, IS_RED, NEITHER);
	memcpy(clear, (unsigned char *)scene->pixels + (SIDE - 1) * 4, 4);

	// Round the other slots, with no pass, until the first one comes back.
	for (uint32_t i = 1; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		if (!open_frame(device))
			return;
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	VOE_TEST_CHECK(voe_render_frame_current(device) == frame);

	// The same slot, no pass: every pixel is the clear colour.
	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);

	{
		int cleared = 0;

		for (int i = 0; i < SIDE * SIDE; i++)
			if (memcmp((unsigned char *)scene->pixels + i * 4,
				   clear, 4) == 0)
				cleared++;
		VOE_TEST_CHECK_INT(cleared, SIDE * SIDE);
	}
}

static void a_pass_with_no_camera_draws_elements(struct scene *scene)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame;
	voe_math_float2 surface = { SURFACE_MM, SURFACE_MM };
	voe_render_element left = {
		.bounds = { 0.0f, 0.0f, SURFACE_MM / 2.0f, SURFACE_MM },
		.clip = { 0.0f, 0.0f, SURFACE_MM, SURFACE_MM },
		.colour = { 1.0f, 0.0f, 0.0f, 1.0f },
		.kind = VOE_RENDER_ELEMENT_SOLID,
	};
	uint32_t first;

	frame = voe_render_frame_current(device);
	if (!open_frame(device))
		return;

	first = voe_render_frame_elements_submitted(device);
	VOE_TEST_CHECK(voe_render_frame_submit_element(device, left));

	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     NULL));
	VOE_TEST_CHECK(voe_render_frame_draw_elements(
		device, voe_render_element_transform(surface), first, 1));
	voe_render_pass_end(device);

	VOE_TEST_CHECK(voe_render_frame_end(device));
	read_back(device, frame, scene->readback.buffer);
	expect_halves(scene->pixels, IS_RED, NEITHER);
}

static void the_pass_capacity_is_per_frame(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_capacities room = CAPACITIES;
	voe_render_device *device;
	voe_base_error error = VOE_BASE_OK;

	room.passes = 1;
	device = voe_render_device_new_headless(arena, size, room, &error);
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return;

	if (open_frame(device)) {
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, NULL));
		voe_render_pass_end(device);
		VOE_TEST_CHECK(!voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, NULL));
		VOE_TEST_CHECK(!voe_render_pass_is_open(device));
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	if (open_frame(device)) {
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, NULL));
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}

	voe_render_device_destroy(device);
}

static bool upload(struct scene *scene, float x0, float x1, float z,
		   voe_render_geometry *out)
{
	voe_render_vertex vertices[4];
	uint32_t indices[6];
	voe_base_error error = VOE_BASE_OK;

	quad(x0, x1, z, vertices, indices);
	return voe_render_geometry_create(scene->device, vertices, 4, indices,
					  6, out, &error);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

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
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(scene.device->device,
						    scene.readback.memory, 0,
						    VK_WHOLE_SIZE, 0,
						    &scene.pixels),
			   VK_SUCCESS);

	VOE_TEST_CHECK(voe_render_shading_create(scene.device, RED, &scene.red,
						 &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, GREEN,
						 &scene.green, &error));
	VOE_TEST_CHECK(upload(&scene, -1.0f, 0.0f, NEAR_DEPTH, &scene.near_left));
	VOE_TEST_CHECK(upload(&scene, 0.0f, 1.0f, FAR_DEPTH, &scene.far_right));
	VOE_TEST_CHECK(upload(&scene, -2.0f, 0.0f, MIDDLE_DEPTH,
			      &scene.middle_shifted));

	if (scene.pixels != NULL) {
		a_second_pass_loads_colour_and_depth(&scene);
		a_frame_with_no_pass_presents_the_clear(&scene);
		a_pass_with_no_camera_draws_elements(&scene);
	} else {
		VOE_TEST_CHECK(scene.pixels != NULL);
	}

	voe_render_vk.device_wait_idle(scene.device->device);
	if (scene.pixels != NULL)
		voe_render_vk.unmap_memory(scene.device->device,
					   scene.readback.memory);
	voe_render_buffer_teardown(scene.device, &scene.readback);
	voe_render_device_destroy(scene.device);

	the_pass_capacity_is_per_frame(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
