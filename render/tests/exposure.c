// A PASS EXPOSES AND TONE MAPS ITS MESH DRAWS BY ITS EXPOSURE RECORD (ADR-0405
// points 7 and 8, 0406 point 4). One quad facing the eye, read at the centre of
// ten pictures:
//
// 1. an unlit quad of 0.5 under a zeroed exposure record: the sRGB byte of 0.5,
//    the old picture, the zeroed record being exposure 0 and Clip;
// 2. exposure 1 at Clip: 255; 3. exposure −1: the sRGB byte of 0.25;
// 4. Soft at exposure 0: the sRGB byte of PBR Neutral(0.5), worked out here,
//    within 2/255;
// 5. a lit quad under a head-on sun of 8 at Clip: 255;
// 6. the same at Soft: below 255, but above 7. the same under a sun of 1 at Soft;
// 8. an unlit quad drawn after the pass's depth clear at exposure −2: as
//    authored, the sRGB byte of 0.5;
// 9. a blended unlit quad of 0.5 at alpha 0.5 at exposure 1: its straight colour
//    doubled to 1, then blended over the clear colour at half;
// 10. the unlit quad under a zeroed sky with exposure 1: still 255, so the
//    picture works with no sky.
//
// Built as unshaded.c builds its device, camera and readback; it includes
// render's internal header because reading a target back is not render's
// surface.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. The skip is a pass and it
// prints its reason.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)
#define CASES 10

// VK_FORMAT_B8G8R8A8_SRGB, which the headless device takes.
#define BLUE 0
#define GREEN 1
#define RED 2

// A driver's rounding on the way into an sRGB target, and the card's 2/255 for
// the curves and the blend.
#define TOLERANCE 1
#define CURVE_TOLERANCE 2

// pass.c's clear colour, linear, which the blended quad is drawn over.
#define CLEAR_RED 0.00857f
#define CLEAR_BLUE 0.01033f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 3,
	.passes = 1,
};

#define EYE_Y 1.8f
#define EYE_Z 4.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// Two metres square at z = 0, facing +Z, counter-clockwise from the eye.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1, 1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { 1, 1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { 1, -1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { -1, -1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
};

static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

// One picture: the record drawn, the light and exposure it is drawn under, and
// whether it goes through the blended pipeline or after a depth clear. Every
// case's sky is zeroed.
struct exposure_case {
	voe_render_shading shading;
	voe_render_light light;
	voe_render_exposure exposure;
	bool blended;
	bool after_clear;
};

// unshaded.c's camera: above and behind the origin, looking at it, reverse-Z.
static voe_render_view the_camera(VkExtent2D extent)
{
	voe_math_float3 eye = { 0.0f, EYE_Y, EYE_Z };
	voe_math_float3 z = voe_math_float3_normalize(eye);
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x = voe_math_float3_normalize(
		voe_math_float3_cross(up, z));
	voe_math_float3 y = voe_math_float3_cross(z, x);
	voe_render_view view = { .eye = eye };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float aspect = (float)extent.width / (float)extent.height;
	float span = FAR_PLANE - NEAR_PLANE;
	voe_math_float3 rows[3] = { x, y, z };

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = focal / aspect;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The frame of one case: one pass, one quad.
static void draw_frame(voe_render_device *device, voe_render_geometry quad,
		       const struct exposure_case *with)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = with->shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	voe_render_pass_camera camera = {
		.view = the_camera(device->resolution),
		.light = with->light,
		.sky = { 0 },
		.exposure = with->exposure,
	};
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	if (with->after_clear)
		voe_render_frame_clear_depth(device);
	if (with->blended)
		VOE_TEST_CHECK(voe_render_frame_draw_blended(device, quad,
							     object));
	else
		VOE_TEST_CHECK(voe_render_frame_draw(device, quad, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	voe_render_vk.device_wait_idle(device->device);
}

// One case drawn, then the target copied into the buffer at `offset`. Two
// submits and an idle between them, as in unshaded.c.
static void draw_case(voe_render_device *device, voe_render_geometry quad,
		      const struct exposure_case *with, VkBuffer buffer,
		      VkDeviceSize offset)
{
	const struct voe_render_frame *frame = voe_render_frame_current(device);
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
		.bufferOffset = offset,
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

	draw_frame(device, quad, with);
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

// A linear value as the byte an sRGB target stores for it.
static int srgb_byte(float linear)
{
	float encoded = linear <= 0.0031308f
				? linear * 12.92f
				: 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

// Khronos PBR Neutral of a grey, tone.slangh's curve written again here.
static float pbr_neutral_grey(float value)
{
	const float start = 0.8f - 0.04f;
	const float d = 1.0f - start;
	float offset = value < 0.08f ? value - 6.25f * value * value : 0.04f;
	float peak = value - offset;

	if (peak < start)
		return peak;
	return 1.0f - d * d / (peak + d - start);
}

static const unsigned char *centre_of(const unsigned char *pixels, int image)
{
	return pixels + IMAGE_BYTES * (VkDeviceSize)image +
	       ((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4;
}

// Every channel within `tolerance` of the sRGB byte of `red`, `red` and `blue`.
static void check_colour(const unsigned char *pixel, float red, float blue,
			 int tolerance)
{
	VOE_TEST_CHECK(abs(pixel[RED] - srgb_byte(red)) <= tolerance);
	VOE_TEST_CHECK(abs(pixel[GREEN] - srgb_byte(red)) <= tolerance);
	VOE_TEST_CHECK(abs(pixel[BLUE] - srgb_byte(blue)) <= tolerance);
}

static void check_pictures(const unsigned char *mapped)
{
	check_colour(centre_of(mapped, 0), 0.5f, 0.5f, TOLERANCE);
	check_colour(centre_of(mapped, 1), 1.0f, 1.0f, 0);
	check_colour(centre_of(mapped, 2), 0.25f, 0.25f, TOLERANCE);
	check_colour(centre_of(mapped, 3), pbr_neutral_grey(0.5f),
		     pbr_neutral_grey(0.5f), CURVE_TOLERANCE);
	check_colour(centre_of(mapped, 4), 1.0f, 1.0f, 0);
	VOE_TEST_CHECK(centre_of(mapped, 5)[RED] < 255);
	VOE_TEST_CHECK(centre_of(mapped, 5)[RED] > centre_of(mapped, 6)[RED]);
	check_colour(centre_of(mapped, 7), 0.5f, 0.5f, TOLERANCE);
	check_colour(centre_of(mapped, 8), 0.5f + 0.5f * CLEAR_RED,
		     0.5f + 0.5f * CLEAR_BLUE, CURVE_TOLERANCE);
	check_colour(centre_of(mapped, 9), 1.0f, 1.0f, 0);
}

// The ten cases, in the order the header lists them.
static void draw_cases(voe_render_device *device, voe_render_geometry quad,
		       const voe_render_shading shadings[3], VkBuffer buffer)
{
	const voe_render_light dark = { 0 };
	const voe_render_light sun = {
		.direction = { 0.0f, 0.0f, -1.0f },
		.intensity = 8.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	voe_render_light weak = sun;

	weak.intensity = 1.0f;
	const struct exposure_case cases[CASES] = {
		{ shadings[0], dark, { 0 }, false, false },
		{ shadings[0], dark, { .exposure = 1.0f }, false, false },
		{ shadings[0], dark, { .exposure = -1.0f }, false, false },
		{ shadings[0], dark, { .tone = VOE_RENDER_TONE_SOFT }, false,
		  false },
		{ shadings[1], sun, { .tone = VOE_RENDER_TONE_CLIP }, false,
		  false },
		{ shadings[1], sun, { .tone = VOE_RENDER_TONE_SOFT }, false,
		  false },
		{ shadings[1], weak, { .tone = VOE_RENDER_TONE_SOFT }, false,
		  false },
		{ shadings[0], dark, { .exposure = -2.0f }, false, true },
		{ shadings[2], dark, { .exposure = 1.0f }, true, false },
		{ shadings[0], dark,
		  { .exposure = 1.0f, .tone = VOE_RENDER_TONE_CLIP }, false,
		  false },
	};

	for (int i = 0; i < CASES; i++)
		draw_case(device, quad, &cases[i], buffer,
			  IMAGE_BYTES * (VkDeviceSize)i);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad = { 0 };
	voe_render_shading shadings[3] = { 0 };
	voe_render_shading_values unlit = {
		.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_shading_values lit = unlit;
	voe_render_shading_values blended = unlit;
	void *mapped = NULL;

	lit.unlit = 0;
	blended.base_colour.w = 0.5f;
	blended.alpha_mode = VOE_RENDER_ALPHA_BLENDED;
	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK_INT(device->format.format, VK_FORMAT_B8G8R8A8_SRGB);
	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
	VOE_TEST_CHECK(copy_image_to_buffer != NULL);
	VOE_TEST_CHECK(voe_render_buffer_build(
		device, &readback, IMAGE_BYTES * CASES,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		"test readback"));
	if (copy_image_to_buffer == NULL || readback.buffer == VK_NULL_HANDLE) {
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_render_geometry_create(device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &quad,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, unlit, &shadings[0],
						 &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, lit, &shadings[1],
						 &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, blended, &shadings[2],
						 &error));

	draw_cases(device, quad, shadings, readback.buffer);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		check_pictures(mapped);
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
