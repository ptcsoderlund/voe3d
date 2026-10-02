// Captured probes settle (ADR-0326 point 5), read through
// ../src/device_internal.h. Headless.
//
// THE SCENE. The window's volume placed with probe 0, world cell (0, 0, 0), at
// (1, 1, 1) about the eye, and a cube 1.5 m a side centred 1.5 m along +X from
// it, at (2.5, 1, 1): its −X face is 0.75 m from probe 0, and probe 1, at
// (3, 1, 1), stands inside it.
//
// FRAMES UNTIL NONE OPENS. Each frame begins the bounce, opens capture passes,
// drawing the cube into each, until one does not open, then relights; the first
// frame only wants the volume. Every frame that captured dispatched a settle.
//
// WHAT SETTLED. Read back once idle: probe 0 has validity 1, and the moments at
// the middle of its +X face have a mean near 0.75 m (1.5 m less half the cube)
// and a mean² near its square; probe 1, whose every texel sees a back face, has
// validity 0.
//
// A SETTLED FRAME. One more frame of begin, a capture pass that does not open,
// and relight records no dispatch.
//
// A card without shaderOutputLayer settles nothing: relight is called and said.
// A machine with no usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SIDE 16
#define FACE VOE_RENDER_BOUNCE_FACE
// The readback: probe 0's +X face of the moments atlas (RG16F) at 0, the first
// two validity texels (R16F) after it.
#define MOMENTS_BYTES ((VkDeviceSize)FACE * FACE * 4)
#define READBACK_BYTES (MOMENTS_BYTES + 4)
// More frames than capturing every probe 64 a frame takes.
#define FRAMES_MAX 200

static const voe_render_capacities CAPACITIES = {
	.vertices = 24,
	.indices = 36,
	.geometries = 1,
	.objects = 8,
	.shadings = 1,
	.passes = 5,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// A cube a metre a side about the origin, its normals outward per face.
static const voe_render_vertex VERTICES[24] = {
	{ { -0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 0, 0 } },
	{ { 0.5f, -0.5f, -0.5f }, { 0, 0, -1 }, { 0, 0 } },
	{ { 0.5f, 0.5f, -0.5f }, { 0, 0, -1 }, { 0, 0 } },
	{ { -0.5f, 0.5f, -0.5f }, { 0, 0, -1 }, { 0, 0 } },
	{ { -0.5f, -0.5f, 0.5f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 0.5f, -0.5f, 0.5f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 0.5f, 0.5f, 0.5f }, { 0, 0, 1 }, { 0, 0 } },
	{ { -0.5f, 0.5f, 0.5f }, { 0, 0, 1 }, { 0, 0 } },
	{ { -0.5f, -0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0 } },
	{ { -0.5f, 0.5f, -0.5f }, { -1, 0, 0 }, { 0, 0 } },
	{ { -0.5f, 0.5f, 0.5f }, { -1, 0, 0 }, { 0, 0 } },
	{ { -0.5f, -0.5f, 0.5f }, { -1, 0, 0 }, { 0, 0 } },
	{ { 0.5f, -0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0 } },
	{ { 0.5f, 0.5f, -0.5f }, { 1, 0, 0 }, { 0, 0 } },
	{ { 0.5f, 0.5f, 0.5f }, { 1, 0, 0 }, { 0, 0 } },
	{ { 0.5f, -0.5f, 0.5f }, { 1, 0, 0 }, { 0, 0 } },
	{ { -0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 0, 0 } },
	{ { 0.5f, -0.5f, -0.5f }, { 0, -1, 0 }, { 0, 0 } },
	{ { 0.5f, -0.5f, 0.5f }, { 0, -1, 0 }, { 0, 0 } },
	{ { -0.5f, -0.5f, 0.5f }, { 0, -1, 0 }, { 0, 0 } },
	{ { -0.5f, 0.5f, -0.5f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 0.5f, 0.5f, -0.5f }, { 0, 1, 0 }, { 0, 0 } },
	{ { 0.5f, 0.5f, 0.5f }, { 0, 1, 0 }, { 0, 0 } },
	{ { -0.5f, 0.5f, 0.5f }, { 0, 1, 0 }, { 0, 0 } },
};

// Nothing is culled in a capture pass, so the winding does not matter.
static const uint32_t INDICES[36] = {
	0,  1,  2,  0,  2,  3,  4,  5,  6,  4,  6,  7,  8,  9,  10, 8,  10, 11,
	12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
};

static const struct voe_render_bounce_frame BOUNCE = {
	.cell = { -12, -6, -12 },
	.corner = { -24.0f, -12.0f, -24.0f },
	.sun = { .direction = { 0.0f, -1.0f, 0.0f }, .intensity = 1.0f,
		 .colour = { 1.0f, 1.0f, 1.0f } },
	.sun_bounces = 1,
	.sun_strength = 1.0f,
};

static voe_render_object cube_object(voe_render_shading grey)
{
	return (voe_render_object){
		.world = voe_math_float4x4_mul(
			voe_math_float4x4_from_translation(
				(voe_math_float3){ 2.5f, 1.0f, 1.0f }),
			voe_math_float4x4_from_scale(
				(voe_math_float3){ 1.5f, 1.5f, 1.5f })),
		.normal = voe_math_float4x4_identity(),
		.shading = grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// One frame: begin, capture passes with the cube until one does not open,
// relight. Returns how many capture passes opened.
static uint32_t one_frame(voe_render_device *device, voe_render_geometry cube,
			  voe_render_shading grey)
{
	bool drawing = false;
	bool opened = true;
	uint32_t passes = 0;

	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &BOUNCE);
	while (opened) {
		VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(device, &opened));
		if (!opened)
			break;
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube, cube_object(grey)));
		voe_render_pass_end(device);
		passes++;
	}
	voe_render_bounce_relight(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	return passes;
}

// The readback buffer filled from the window volume, once the card is idle.
static void read_volume(voe_render_device *device,
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
	const VkBufferImageCopy moments = {
		.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				      .layerCount = 1 },
		.imageExtent = { FACE, FACE, 1 },
	};
	const VkBufferImageCopy validity = {
		.bufferOffset = MOMENTS_BYTES,
		.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				      .layerCount = 1 },
		.imageExtent = { 2, 1, 1 },
	};
	const VkMemoryBarrier2 barriers[2] = {
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
	copy(commands, device->window_volume.moments.image,
	     VK_IMAGE_LAYOUT_GENERAL, readback->buffer, 1, &moments);
	copy(commands, device->window_volume.validity.image,
	     VK_IMAGE_LAYOUT_GENERAL, readback->buffer, 1, &validity);
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

// An IEEE half as a float.
static float half_to_float(uint16_t h)
{
	const int exponent = (h >> 10) & 0x1f;
	const float mantissa = (float)(h & 0x3ff);
	const float value = exponent == 0 ?
				    ldexpf(mantissa, -24) :
				    ldexpf(1024.0f + mantissa, exponent - 25);

	return (h & 0x8000) != 0 ? -value : value;
}

static void check_settled(voe_render_device *device)
{
	struct voe_render_buffer readback = { 0 };
	void *mapped = NULL;

	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, READBACK_BYTES, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (readback.buffer == VK_NULL_HANDLE)
		return;
	read_volume(device, &readback);
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		uint16_t halves[READBACK_BYTES / 2];
		// The texel just off the face's middle, (3, 3).
		const size_t middle = 2 * (3 * FACE + 3);
		float mean;
		float square;

		memcpy(halves, mapped, sizeof(halves));
		mean = half_to_float(halves[middle]);
		square = half_to_float(halves[middle + 1]);
		printf("probe 0 +X: mean %.3f m, mean² %.3f; validity %.1f, %.1f\n",
		       (double)mean, (double)square,
		       (double)half_to_float(halves[MOMENTS_BYTES / 2]),
		       (double)half_to_float(halves[MOMENTS_BYTES / 2 + 1]));
		VOE_TEST_CHECK(fabsf(mean - 0.75f) < 0.1f);
		VOE_TEST_CHECK(fabsf(square - mean * mean) < 0.05f);
		VOE_TEST_CHECK(half_to_float(halves[MOMENTS_BYTES / 2]) == 1.0f);
		VOE_TEST_CHECK(half_to_float(halves[MOMENTS_BYTES / 2 + 1]) == 0.0f);
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}
	voe_render_buffer_teardown(device, &readback);
}

static void settle(voe_render_device *device)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry cube;
	voe_render_shading grey;
	uint32_t frames = 0;
	uint32_t before;

	VOE_TEST_CHECK(voe_render_shading_create(device, GREY, &grey, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(device, VERTICES, 24, INDICES,
						  36, &cube, &error));
	// The first frame only wants the volume.
	VOE_TEST_CHECK_INT(one_frame(device, cube, grey), 0);
	VOE_TEST_CHECK(device->window_volume.wanted);
	for (; frames < FRAMES_MAX; frames++) {
		before = device->relight_dispatches;
		if (one_frame(device, cube, grey) == 0)
			break;
		VOE_TEST_CHECK(device->relight_dispatches > before);
	}
	printf("captured every probe in %u frames\n", frames);
	VOE_TEST_CHECK(frames > 0 && frames < FRAMES_MAX);
	check_settled(device);

	before = device->relight_dispatches;
	VOE_TEST_CHECK_INT(one_frame(device, cube, grey), 0);
	VOE_TEST_CHECK_INT(device->relight_dispatches, before);
}

static void nothing_without_output_layer(voe_render_device *device)
{
	bool drawing = false;

	printf("note: no shaderOutputLayer, so nothing settles\n");
	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	if (!drawing)
		return;
	voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &BOUNCE);
	voe_render_bounce_relight(device);
	VOE_TEST_CHECK_INT(device->relight_dispatches, 0);
	VOE_TEST_CHECK(voe_render_frame_end(device));
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
			settle(device);
		else
			nothing_without_output_layer(device);
		voe_render_device_destroy(device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
