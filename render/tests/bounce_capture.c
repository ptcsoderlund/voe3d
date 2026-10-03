// The bounce capture pass (ADR-0326 point 3), read through
// ../src/device_internal.h. Headless.
//
// A VOLUME BUILT. A begin onto the window wants it; the next frame's top builds
// it and that frame's begin places it, every probe queued.
//
// FOUR OPEN, THE FIFTH DOES NOT. In that frame the first capture pass opens: a
// red cube a metre a side centred at (2.5, 1, 1), under 3 m from the eye, adds
// one draw; the same cube 100 m off adds none. Three more open, and a fifth says
// it did not (VOE_RENDER_BOUNCE_CAPTURE_PASSES).
//
// PASSES SPENT. The next frame opens `passes` passes with no camera; a capture
// pass is then false, and says nothing opened.
//
// THE PICTURE. The nearest probe, ties to the lower index, is probe 0, world cell
// (0, 0, 0), centre (1, 1, 1): its atlas strip is x 0..47, y 0..7. The cube is
// 1 to 2 m along its +X, so the middle of face 0's tile is red in the albedo atlas,
// read back after the frame, and face 1's (−X) is still nought.
//
// A card without shaderOutputLayer opens nothing: checked instead, and said. A
// machine with no usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 16
// Probe 0's strip of the albedo atlas: six faces across, one down, RGBA8.
#define STRIP_WIDTH (6u * VOE_RENDER_BOUNCE_FACE)
#define STRIP_BYTES ((VkDeviceSize)STRIP_WIDTH * VOE_RENDER_BOUNCE_FACE * 4)

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 8,
	.shadings = 1,
	.passes = 5,
};

static const voe_render_shading_values RED = {
	.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// A cube a metre a side about the origin.
static const voe_render_vertex VERTICES[8] = {
	{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
};

static const uint32_t INDICES[36] = {
	0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
	3, 6, 2, 3, 7, 6, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5,
};

static const struct voe_render_bounce_frame BOUNCE = {
	.cell = { -12, -6, -12 },
	.corner = { -24.0f, -12.0f, -24.0f },
	.sun = { .direction = { 0.0f, -1.0f, 0.0f }, .intensity = 1.0f,
		 .colour = { 1.0f, 1.0f, 1.0f } },
	.sun_bounces = 1,
	.sun_strength = 1.0f,
	.spacing = VOE_RENDER_BOUNCE_SPACING,
};

// A frame begun and the window's bounce begun; false when it is not drawing.
static bool open_frame(voe_render_device *device)
{
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing)
		voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &BOUNCE);
	return drawing;
}

static voe_render_object cube_at(voe_render_shading red, float x)
{
	return (voe_render_object){
		.world = voe_math_float4x4_from_translation(
			(voe_math_float3){ x, 1.0f, 1.0f }),
		.normal = voe_math_float4x4_identity(),
		.shading = red.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// The frame of four capture passes, the first with the near and far cubes, and
// a fifth that does not open.
static void four_passes(voe_render_device *device, voe_render_geometry cube,
			voe_render_shading red)
{
	bool opened = false;
	uint32_t before;

	if (!open_frame(device))
		return;
	VOE_TEST_CHECK(device->window_volume.built);
	VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(device, &opened));
	VOE_TEST_CHECK(opened);
	if (opened) {
		before = voe_render_frame_draw_count(device);
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube,
						     cube_at(red, 2.5f)));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), before + 1);
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube,
						     cube_at(red, 100.0f)));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), before + 1);
		voe_render_pass_end(device);
	}
	for (int i = 1; i < VOE_RENDER_BOUNCE_CAPTURE_PASSES; i++) {
		opened = false;
		VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(device, &opened));
		VOE_TEST_CHECK(opened);
		if (opened)
			voe_render_pass_end(device);
	}
	opened = true;
	VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(device, &opened));
	VOE_TEST_CHECK(!opened);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// A frame whose passes are spent before a capture pass is asked for.
static void passes_spent(voe_render_device *device)
{
	bool opened = true;

	if (!open_frame(device))
		return;
	for (uint32_t i = 0; i < CAPACITIES.passes; i++) {
		VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
						     NULL));
		voe_render_pass_end(device);
	}
	VOE_TEST_CHECK(!voe_render_bounce_capture_pass_begin(device, &opened));
	VOE_TEST_CHECK(!opened);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// Probe 0's strip of the window volume's albedo atlas into `readback`, once
// the card is idle.
static void read_strip(voe_render_device *device,
		       const struct voe_render_buffer *readback)
{
	PFN_vkCmdCopyImageToBuffer copy = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
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
		.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				      .layerCount = 1 },
		.imageExtent = { STRIP_WIDTH, VOE_RENDER_BOUNCE_FACE, 1 },
	};
	VkMemoryBarrier2 barriers[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
			.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
		},
	};
	VkDependencyInfo before = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &barriers[0],
	};
	VkDependencyInfo after = before;
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};

	after.pMemoryBarriers = &barriers[1];
	VOE_TEST_CHECK(copy != NULL);
	if (copy == NULL)
		return;
	voe_render_vk.device_wait_idle(device->device);
	VOE_TEST_CHECK_INT(voe_render_vk.allocate_command_buffers(device->device,
								  &allocate,
								  &commands),
			   VK_SUCCESS);
	if (commands == VK_NULL_HANDLE)
		return;
	voe_render_vk.begin_command_buffer(commands, &begin);
	voe_render_vk.cmd_pipeline_barrier2(commands, &before);
	copy(commands, device->window_volume.albedo.image,
	     VK_IMAGE_LAYOUT_GENERAL, readback->buffer, 1, &region);
	voe_render_vk.cmd_pipeline_barrier2(commands, &after);
	voe_render_vk.end_command_buffer(commands);
	submit_commands.commandBuffer = commands;
	VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1, &submit,
						       VK_NULL_HANDLE),
			   VK_SUCCESS);
	voe_render_vk.device_wait_idle(device->device);
	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
}

// The texel at (x, y) of the strip, channel `c`.
static int texel(const unsigned char *strip, uint32_t x, uint32_t y, int c)
{
	return strip[((size_t)y * STRIP_WIDTH + x) * 4 + (size_t)c];
}

static void check_picture(voe_render_device *device)
{
	struct voe_render_buffer readback = { 0 };
	const uint32_t middle = VOE_RENDER_BOUNCE_FACE / 2;
	void *mapped = NULL;

	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, STRIP_BYTES, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (readback.buffer == VK_NULL_HANDLE)
		return;
	read_strip(device, &readback);
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		const unsigned char *strip = mapped;

		// Face 0 (+X) faces the cube; face 1 (−X) faces nothing.
		VOE_TEST_CHECK(texel(strip, middle, middle, 0) > 200);
		VOE_TEST_CHECK(texel(strip, middle, middle, 1) < 20);
		VOE_TEST_CHECK_INT(texel(strip, VOE_RENDER_BOUNCE_FACE + middle,
					 middle, 0),
				   0);
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}
	voe_render_buffer_teardown(device, &readback);
}

static void capture(voe_render_device *device)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry cube;
	voe_render_shading red;

	VOE_TEST_CHECK(voe_render_shading_create(device, RED, &red, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(device, VERTICES, 8, INDICES,
						  36, &cube, &error));
	if (open_frame(device))
		VOE_TEST_CHECK(voe_render_frame_end(device));
	four_passes(device, cube, red);
	check_picture(device);
	passes_spent(device);
}

static void nothing_without_output_layer(voe_render_device *device)
{
	bool opened = true;

	printf("note: no shaderOutputLayer, so nothing is captured\n");
	for (int frame = 0; frame < 2; frame++) {
		if (!open_frame(device))
			return;
		VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(device, &opened));
		VOE_TEST_CHECK(!opened);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = voe_render_device_new_headless(
		arena, (voe_platform_size){ SIDE, SIDE }, CAPACITIES, &error);

	if (device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
			       error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		voe_base_arena_destroy(arena);
		return 0;
	}
	VOE_TEST_CHECK(device != NULL);
	if (device != NULL) {
		if (device->output_layer)
			capture(device);
		else
			nothing_without_output_layer(device);
		voe_render_device_destroy(device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
