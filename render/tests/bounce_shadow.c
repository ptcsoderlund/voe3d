// The bounce shadow pass (ADR-0329 point 2), read through
// ../src/device_internal.h. Headless.
//
// A VOLUME BUILT. The first frame's begin onto the window wants it; the next
// frame's top builds it and that frame's begin places it, every probe queued.
//
// THE FRAME THAT CAPTURES. Its capture passes open, a cube a metre a side 2 m
// from the eye drawn into each; then the bounce shadow pass opens under a sun
// that bounces once, the cube adds one draw, and the relight runs. A copy of
// that slot's map to a host buffer holds a texel other than the clear.
//
// WHEN IT DOES NOT OPEN. A following frame with nothing changed: no relight is
// needed. A sun of bounces 0 with a lamp of bounces 1: the lights changed, but
// the sun does not bounce. Then, the sun bouncing again, a frame whose `passes`
// are spent: false, and nothing opened.
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
#define TEXELS VOE_RENDER_BOUNCE_SHADOW_TEXELS
#define MAP_BYTES ((VkDeviceSize)TEXELS * TEXELS * sizeof(float))
// The light's orthographic box: HALF metres either side of the eye, looking
// down from HEIGHT, depth from NEAR to FAR below it.
#define HALF 4.0f
#define HEIGHT 10.0f
#define NEAR 1.0f
#define FAR 20.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 8,
	.shadings = 1,
	.passes = VOE_RENDER_BOUNCE_CAPTURE_PASSES + 1,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
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

static const voe_render_point_light LAMP = {
	.position = { 0.0f, 1.0f, 0.0f },
	.range = 5.0f,
	.colour = { 1.0f, 1.0f, 1.0f },
	.falloff = 1.0f,
	.bounces = 1,
	.bounce_strength = 1.0f,
};

// The bounce of every frame: the sun straight down, bouncing `sun_bounces`
// times, and the lamp when `lamp`.
static struct voe_render_bounce_frame bounce(uint32_t sun_bounces, bool lamp)
{
	return (struct voe_render_bounce_frame){
		.cell = { -12, -6, -12 },
		.corner = { -24.0f, -12.0f, -24.0f },
		.sun = { .direction = { 0.0f, -1.0f, 0.0f }, .intensity = 1.0f,
			 .colour = { 1.0f, 1.0f, 1.0f } },
		.sun_bounces = sun_bounces,
		.sun_strength = 1.0f,
		.points = { .lights = lamp ? &LAMP : NULL, .count = lamp ? 1u : 0u },
	};
}

// The sun's view: from HEIGHT above the eye straight down, orthographic and
// reverse-Z, depth one at NEAR and nought at FAR.
static voe_render_view sun_view(void)
{
	voe_render_view light = { .eye = { 0.0f, HEIGHT, 0.0f } };
	const float span = FAR - NEAR;

	light.view.m[0][0] = 1.0f;
	light.view.m[1][2] = -1.0f;
	light.view.m[2][1] = 1.0f;
	light.view.m[2][3] = -HEIGHT;
	light.view.m[3][3] = 1.0f;
	light.projection.m[0][0] = 1.0f / HALF;
	light.projection.m[1][1] = 1.0f / HALF;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

// A frame begun and the window's bounce begun; false when it is not drawing.
static bool open_frame(voe_render_device *device,
		       const struct voe_render_bounce_frame *frame)
{
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing)
		voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, frame);
	return drawing;
}

// The cube 2 m from the eye along +X.
static voe_render_object cube_object(voe_render_shading grey)
{
	return (voe_render_object){
		.world = voe_math_float4x4_from_translation(
			(voe_math_float3){ 2.0f, 0.0f, 0.0f }),
		.normal = voe_math_float4x4_identity(),
		.shading = grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// A frame whose bounce shadow pass is asked for with nothing else drawn, and
// says it did not open; relit after.
static void not_opened(voe_render_device *device,
		       const struct voe_render_bounce_frame *frame)
{
	const voe_render_view light = sun_view();
	bool opened = true;

	if (!open_frame(device, frame))
		return;
	VOE_TEST_CHECK(voe_render_bounce_shadow_pass_begin(device, &light, &opened));
	VOE_TEST_CHECK(!opened);
	voe_render_bounce_relight(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// The frame that captures: the capture passes with the cube, then the bounce
// shadow pass with it, one draw, and the relight. Returns the frame's slot.
static uint32_t captures(voe_render_device *device, voe_render_geometry cube,
			 voe_render_shading grey)
{
	const struct voe_render_bounce_frame frame = bounce(1, false);
	const voe_render_view light = sun_view();
	const uint32_t slot = device->slot;
	bool opened = true;
	uint32_t before;

	if (!open_frame(device, &frame))
		return slot;
	VOE_TEST_CHECK(device->window_volume.built);
	for (int i = 0; i < VOE_RENDER_BOUNCE_CAPTURE_PASSES; i++) {
		VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(device, &opened));
		VOE_TEST_CHECK(opened);
		if (!opened)
			break;
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube, cube_object(grey)));
		voe_render_pass_end(device);
	}
	opened = false;
	VOE_TEST_CHECK(voe_render_bounce_shadow_pass_begin(device, &light, &opened));
	VOE_TEST_CHECK(opened);
	if (opened) {
		before = voe_render_frame_draw_count(device);
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube, cube_object(grey)));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), before + 1);
		voe_render_pass_end(device);
	}
	VOE_TEST_CHECK(device->frames[slot].bounce_shadow.drawn);
	voe_render_bounce_relight(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	return slot;
}

// Slot `slot`'s map into `readback`, once the card is idle, and back where it
// rests.
static void read_map(voe_render_device *device, uint32_t slot,
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
		.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				      .layerCount = 1 },
		.imageExtent = { TEXELS, TEXELS, 1 },
	};
	VkImageMemoryBarrier2 moves[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = device->frames[slot].bounce_shadow.map.image,
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
					      .levelCount = 1,
					      .layerCount = 1 },
		},
	};
	VkMemoryBarrier2 host = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
		.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
	};
	VkDependencyInfo before = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &moves[0],
	};
	VkDependencyInfo after = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &host,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &moves[1],
	};
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};

	moves[1] = moves[0];
	moves[1].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
	moves[1].srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
	moves[1].dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	moves[1].dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
	moves[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	moves[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
	copy(commands, moves[0].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	     readback->buffer, 1, &region);
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

// Whether slot `slot`'s map holds a texel other than the clear.
static void check_map(voe_render_device *device, uint32_t slot)
{
	struct voe_render_buffer readback = { 0 };
	void *mapped = NULL;
	bool drawn = false;

	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, MAP_BYTES, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (readback.buffer == VK_NULL_HANDLE)
		return;
	read_map(device, slot, &readback);
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		const float *texels = mapped;

		for (size_t i = 0; !drawn && i < (size_t)TEXELS * TEXELS; i++)
			drawn = texels[i] != VOE_RENDER_DEPTH_CLEAR;
		VOE_TEST_CHECK(drawn);
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}
	voe_render_buffer_teardown(device, &readback);
}

// A frame whose passes are spent before the bounce shadow pass is asked for.
static void passes_spent(voe_render_device *device)
{
	const struct voe_render_bounce_frame frame = bounce(1, false);
	const voe_render_view light = sun_view();
	bool opened = true;

	if (!open_frame(device, &frame))
		return;
	for (uint32_t i = 0; i < CAPACITIES.passes; i++) {
		VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
						     NULL));
		voe_render_pass_end(device);
	}
	VOE_TEST_CHECK(!voe_render_bounce_shadow_pass_begin(device, &light, &opened));
	VOE_TEST_CHECK(!opened);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

static void sun_map(voe_render_device *device)
{
	const struct voe_render_bounce_frame sun = bounce(1, false);
	const struct voe_render_bounce_frame lamp = bounce(0, true);
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry cube;
	voe_render_shading grey;

	VOE_TEST_CHECK(voe_render_shading_create(device, GREY, &grey, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(device, VERTICES, 8, INDICES,
						  36, &cube, &error));
	if (open_frame(device, &sun))
		VOE_TEST_CHECK(voe_render_frame_end(device));
	check_map(device, captures(device, cube, grey));
	not_opened(device, &sun);
	not_opened(device, &lamp);
	passes_spent(device);
}

static void nothing_without_output_layer(voe_render_device *device)
{
	const struct voe_render_bounce_frame sun = bounce(1, false);

	printf("note: no shaderOutputLayer, so no bounce sun map is drawn\n");
	for (int frame = 0; frame < 2; frame++)
		not_opened(device, &sun);
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
			sun_map(device);
		else
			nothing_without_output_layer(device);
		voe_render_device_destroy(device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
