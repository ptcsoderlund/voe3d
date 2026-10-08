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
// READINESS (ADR-0389 point 4), the validity's G, before the frames above go
// on: probe 0's is 0 read back after the first frame that captured it,
// i / VOE_RENDER_BOUNCE_FADE after i more, and 1 after VOE_RENDER_BOUNCE_FADE.
//
// A FADING FRAME. The frame after the first that captures nothing, its probes
// still fading in: the bounce shadow pass does not open, and the relight
// records one dispatch, the settle.
//
// A SETTLED FRAME. After VOE_RENDER_BOUNCE_FADE frames for the last pictures
// to fade in, one more frame of begin, a capture pass that does not open, and
// relight records no dispatch.
//
// THE BREAKDOWN. Read VOE_RENDER_FRAMES_IN_FLIGHT frames after one that
// relit, it holds `bounce relight`; after the settled frame, it does not. A card
// that writes no timestamps skips this with a line.
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
// two validity texels (RG16F: validity, readiness) after it.
#define MOMENTS_BYTES ((VkDeviceSize)FACE * FACE * 4)
#define READBACK_BYTES (MOMENTS_BYTES + 8)
// Halves into the readback: probe 0's validity and readiness, probe 1's validity.
#define VALIDITY_0 (MOMENTS_BYTES / 2)
#define READY_0 (VALIDITY_0 + 1)
#define VALIDITY_1 (VALIDITY_0 + 2)
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
	.spacing = VOE_RENDER_BOUNCE_SPACING,
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
	copy(commands, device->window_volume[0].moments.image,
	     VK_IMAGE_LAYOUT_GENERAL, readback->buffer, 1, &moments);
	copy(commands, device->window_volume[0].validity.image,
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

// The readback's halves into `halves`, once the card is idle; false, checked,
// when it could not be read.
static bool read_halves(voe_render_device *device,
			uint16_t halves[READBACK_BYTES / 2])
{
	struct voe_render_buffer readback = { 0 };
	void *mapped = NULL;

	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, READBACK_BYTES, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		"test readback"));
	if (readback.buffer == VK_NULL_HANDLE)
		return false;
	read_volume(device, &readback);
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		memcpy(halves, mapped, READBACK_BYTES);
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}
	voe_render_buffer_teardown(device, &readback);
	return mapped != NULL;
}

static void check_settled(voe_render_device *device)
{
	uint16_t halves[READBACK_BYTES / 2];
	// The texel just off the face's middle, (3, 3).
	const size_t middle = 2 * (3 * FACE + 3);
	float mean;
	float square;

	if (!read_halves(device, halves))
		return;
	mean = half_to_float(halves[middle]);
	square = half_to_float(halves[middle + 1]);
	printf("probe 0 +X: mean %.3f m, mean² %.3f; validity %.1f, %.1f\n",
	       (double)mean, (double)square,
	       (double)half_to_float(halves[VALIDITY_0]),
	       (double)half_to_float(halves[VALIDITY_1]));
	VOE_TEST_CHECK(fabsf(mean - 0.75f) < 0.1f);
	VOE_TEST_CHECK(fabsf(square - mean * mean) < 0.05f);
	VOE_TEST_CHECK(half_to_float(halves[VALIDITY_0]) == 1.0f);
	VOE_TEST_CHECK(half_to_float(halves[VALIDITY_1]) == 0.0f);
}

// Probe 0's readiness read back, or −1 when it could not be read.
static float readiness_of_probe_0(voe_render_device *device)
{
	uint16_t halves[READBACK_BYTES / 2];

	return read_halves(device, halves) ? half_to_float(halves[READY_0]) :
					     -1.0f;
}

// The first frame that captures, then VOE_RENDER_BOUNCE_FADE more, probe 0's
// readiness read back after each.
static void a_new_picture_reads_ready_after_sixteen_frames(
	voe_render_device *device, voe_render_geometry cube,
	voe_render_shading grey)
{
	bool rising = true;

	VOE_TEST_CHECK(one_frame(device, cube, grey) > 0);
	VOE_TEST_CHECK(readiness_of_probe_0(device) == 0.0f);
	for (uint32_t i = 1; i <= VOE_RENDER_BOUNCE_FADE; i++) {
		const float ready = (one_frame(device, cube, grey),
				     readiness_of_probe_0(device));

		rising = rising &&
			 ready == (float)i / (float)VOE_RENDER_BOUNCE_FADE;
	}
	VOE_TEST_CHECK(rising);
	VOE_TEST_CHECK(readiness_of_probe_0(device) == 1.0f);
}

// A frame with only fading probes: no capture, no bounce shadow pass, and the
// settle the relight's one dispatch.
static void a_fading_frame_records_only_the_settle(voe_render_device *device)
{
	const voe_render_view light = {
		.view = voe_math_float4x4_identity(),
		.projection = voe_math_float4x4_identity(),
	};
	const uint32_t before = device->relight_dispatches;
	bool drawing = false;
	bool opened = true;

	VOE_TEST_CHECK(voe_render_bounce_probes_fading(&device->window_volume[0].probes));
	VOE_TEST_CHECK(voe_render_frame_begin(device, (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	if (!drawing)
		return;
	voe_render_bounce_begin(device, VOE_RENDER_TARGET_WINDOW, &BOUNCE);
	VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(device, &opened));
	VOE_TEST_CHECK(!opened);
	opened = true;
	VOE_TEST_CHECK(voe_render_bounce_shadow_pass_begin(device, 0, &light,
							   &opened));
	VOE_TEST_CHECK(!opened);
	voe_render_bounce_relight(device);
	VOE_TEST_CHECK_INT(device->relight_dispatches, before + 1);
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// Whether the newest breakdown names the relight.
static bool breakdown_relit(const voe_render_device *device)
{
	voe_render_pass_time times[8];
	const uint32_t count = voe_render_frame_pass_times(device, times, 8);

	for (uint32_t i = 0; i < count; i++)
		if (strcmp(times[i].name, "bounce relight") == 0)
			return true;
	return false;
}

static void settle(voe_render_device *device)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry cube;
	voe_render_shading grey;
	uint32_t frames = 0;
	uint32_t passes;
	uint32_t before;

	VOE_TEST_CHECK(voe_render_shading_create(device, GREY, &grey, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(device, VERTICES, 24, INDICES,
						  36, &cube, &error));
	// The first frame only wants the volume.
	VOE_TEST_CHECK_INT(one_frame(device, cube, grey), 0);
	VOE_TEST_CHECK(device->window_volume[0].wanted);
	a_new_picture_reads_ready_after_sixteen_frames(device, cube, grey);
	for (; frames < FRAMES_MAX; frames++) {
		before = device->relight_dispatches;
		passes = one_frame(device, cube, grey);
		// This frame's begin read the frame VOE_RENDER_FRAMES_IN_FLIGHT
		// back, a capturing one once this loop has run that many.
		if (frames == VOE_RENDER_FRAMES_IN_FLIGHT) {
			if (device->timestamps)
				VOE_TEST_CHECK(breakdown_relit(device));
			else
				printf("note: no timestamps, so no breakdown to read\n");
		}
		if (passes == 0)
			break;
		VOE_TEST_CHECK(device->relight_dispatches > before);
	}
	printf("captured every probe in %u frames\n", frames);
	VOE_TEST_CHECK(frames > 0 && frames < FRAMES_MAX);
	a_fading_frame_records_only_the_settle(device);
	check_settled(device);
	// The last pictures fade in, relighting, before the grid is still.
	for (uint32_t i = 0; i < VOE_RENDER_BOUNCE_FADE; i++)
		VOE_TEST_CHECK_INT(one_frame(device, cube, grey), 0);

	before = device->relight_dispatches;
	VOE_TEST_CHECK_INT(one_frame(device, cube, grey), 0);
	VOE_TEST_CHECK_INT(device->relight_dispatches, before);
	// The settled frame's breakdown, read that many settled frames on.
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		VOE_TEST_CHECK_INT(one_frame(device, cube, grey), 0);
	VOE_TEST_CHECK_INT(device->relight_dispatches, before);
	VOE_TEST_CHECK(!breakdown_relit(device));
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
