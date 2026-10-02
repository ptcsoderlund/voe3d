// A PASS'S POINT LIGHTS LIGHT ITS SURFACES, AND ONLY WHERE THEY REACH (ADR-0320).
// A grey ground quad seen straight down from 10 m, a sun of intensity 0 and no
// fill, so whatever is not black is a point light's. Six cases:
//
// 1. one red light 1 m above the quad's middle, range 3: the pixel under it red
//    above 0.2 with green and blue near nought, and a pixel 5 m off black;
// 2. the same scene with `points` zeroed: black under where the light was;
// 3. an unshaded pass with the light: the unshaded picture, the base colour,
//    because the unshaded exit adds no point light;
// 4. two lights, 4 m either side of the middle: both pools red, and the middle,
//    4 m from each and past their range, black;
// 5. the first light at falloff 0.25, 1 and 4 (ADR-0322): a pixel 2.3 m along
//    the ground, about 0.85 of the reach, brighter at 0.25 than at 1 and at 1
//    than at 4, and the pixel under the light no darker at 0.25 than at 1;
// 6. the first light with `shadow` 3 and strength 1 (ADR-0325): the ground lit
//    exactly as case 1, because this device has no point shadows.
//
// Every light is falloff 1 but case 5's others, so cases 1 to 4 keep the
// numbers they had before falloff.
//
// The material is lit, fully rough and not metallic. Built the way unshaded.c
// builds its device and reads its target back, and it includes render's
// internal header for the same reason: reading a target back is not part of
// render's surface.
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
#include <string.h>

#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)
#define CASES 8

// VK_FORMAT_B8G8R8A8_SRGB, which the headless device takes.
#define BLUE 0
#define GREEN 1
#define RED 2

// How far a channel may be from the expected byte: a driver's rounding on the
// way into an sRGB target.
#define TOLERANCE 3

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
};

#define EYE_Y 10.0f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define GREY 0.5f

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// A 20 m square at y = 0 facing up, counter-clockwise seen from above.
#define H 10.0f
static const voe_render_vertex GROUND_VERTICES[4] = {
	{ { -H, 0, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, 0, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, 0, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, 0, -H }, { 0, 1, 0 }, { 0, 0 } },
};

static const uint32_t GROUND_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

// Straight down from (0, EYE_Y, 0), world +X to the right and −Z up the
// picture, a 90° field of view, reverse-Z: 32 pixels are 10 m on the ground.
static voe_render_view the_camera(void)
{
	voe_math_float3 eye = { 0.0f, EYE_Y, 0.0f };
	voe_math_float3 rows[3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 1.0f, 0.0f },
	};
	voe_render_view view = { .eye = eye };
	float span = FAR_PLANE - NEAR_PLANE;

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = 1.0f;
	view.projection.m[1][1] = 1.0f;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// One pass with the given light and points, then the target copied into the
// buffer at `offset`. Two submits and an idle between them, as in unshaded.c.
static void draw_case(voe_render_device *device, voe_render_geometry ground,
		      voe_render_shading shading, voe_render_light light,
		      voe_render_point_lights points, VkBuffer buffer,
		      VkDeviceSize offset)
{
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = light,
		.points = points,
	};
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
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, ground, object));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
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

// A linear value as the byte an sRGB target stores for it.
static int srgb_byte(float linear)
{
	float encoded = linear <= 0.0031308f
				? linear * 12.92f
				: 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

static const unsigned char *pixel_at(const unsigned char *pixels, int image,
				     int column, int row)
{
	return pixels + IMAGE_BYTES * (VkDeviceSize)image +
	       ((size_t)row * SIDE + (size_t)column) * 4;
}

static void check_red(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] > srgb_byte(0.2f));
	VOE_TEST_CHECK(pixel[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] <= TOLERANCE);
}

static void check_black(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] <= TOLERANCE);
}

static void check_grey(const unsigned char *pixel)
{
	VOE_TEST_CHECK(abs(pixel[RED] - srgb_byte(GREY)) <= TOLERANCE);
	VOE_TEST_CHECK(abs(pixel[GREEN] - srgb_byte(GREY)) <= TOLERANCE);
	VOE_TEST_CHECK(abs(pixel[BLUE] - srgb_byte(GREY)) <= TOLERANCE);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry ground = { 0 };
	voe_render_shading shading = { 0 };
	voe_render_shading_values values = {
		.base_colour = { GREY, GREY, GREY, 1.0f },
		.metallic = 0.0f,
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	// A sun of nothing and no fill: black wherever no point light reaches.
	voe_render_light dark = {
		.direction = { 0.0f, -1.0f, 0.0f },
		.intensity = 0.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	voe_render_light unshaded = dark;
	const voe_render_point_light middle[1] = {
		{ .position = { 0.0f, 1.0f, 0.0f }, .range = 3.0f,
		  .colour = { 3.0f, 0.0f, 0.0f }, .falloff = 1.0f },
	};
	const voe_render_point_light sides[2] = {
		{ .position = { 4.0f, 1.0f, 0.0f }, .range = 3.0f,
		  .colour = { 3.0f, 0.0f, 0.0f }, .falloff = 1.0f },
		{ .position = { -4.0f, 1.0f, 0.0f }, .range = 3.0f,
		  .colour = { 3.0f, 0.0f, 0.0f }, .falloff = 1.0f },
	};
	const float falloffs[3] = { 0.25f, 1.0f, 4.0f };
	voe_render_point_light bent[3];
	voe_render_point_light slotted = middle[0];
	const voe_render_point_lights one = { middle, 1 };
	const voe_render_point_lights none = { 0 };
	const voe_render_point_lights two = { sides, 2 };
	void *mapped = NULL;

	unshaded.unshaded = 1;
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
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (copy_image_to_buffer == NULL || readback.buffer == VK_NULL_HANDLE) {
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_render_geometry_create(device, GROUND_VERTICES, 4,
						  GROUND_INDICES, 6, &ground,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));

	draw_case(device, ground, shading, dark, one, readback.buffer, 0);
	draw_case(device, ground, shading, dark, none, readback.buffer,
		  IMAGE_BYTES);
	draw_case(device, ground, shading, unshaded, one, readback.buffer,
		  IMAGE_BYTES * 2);
	draw_case(device, ground, shading, dark, two, readback.buffer,
		  IMAGE_BYTES * 3);
	for (int f = 0; f < 3; f++) {
		bent[f] = middle[0];
		bent[f].falloff = falloffs[f];
		draw_case(device, ground, shading, dark,
			  (voe_render_point_lights){ &bent[f], 1 },
			  readback.buffer, IMAGE_BYTES * (VkDeviceSize)(4 + f));
	}
	slotted.shadow = 3;
	slotted.shadow_strength = 1.0f;
	draw_case(device, ground, shading, dark,
		  (voe_render_point_lights){ &slotted, 1 }, readback.buffer,
		  IMAGE_BYTES * 7);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		// 16 pixels to the right of the middle is 5 m along +X.
		check_red(pixel_at(mapped, 0, SIDE / 2, SIDE / 2));
		check_black(pixel_at(mapped, 0, SIDE / 2 + 16, SIDE / 2));
		check_black(pixel_at(mapped, 1, SIDE / 2, SIDE / 2));
		check_grey(pixel_at(mapped, 2, SIDE / 2, SIDE / 2));
		// Columns 44 and 19 are 3.9 m either side, under each light.
		check_red(pixel_at(mapped, 3, 44, SIDE / 2));
		check_red(pixel_at(mapped, 3, 19, SIDE / 2));
		check_black(pixel_at(mapped, 3, SIDE / 2, SIDE / 2));
		// Column 39 is 2.3 m along +X, 2.55 m from the light.
		VOE_TEST_CHECK(pixel_at(mapped, 4, SIDE / 2 + 7, SIDE / 2)[RED] >
			       pixel_at(mapped, 5, SIDE / 2 + 7, SIDE / 2)[RED]);
		VOE_TEST_CHECK(pixel_at(mapped, 5, SIDE / 2 + 7, SIDE / 2)[RED] >
			       pixel_at(mapped, 6, SIDE / 2 + 7, SIDE / 2)[RED]);
		VOE_TEST_CHECK(pixel_at(mapped, 4, SIDE / 2, SIDE / 2)[RED] >=
			       pixel_at(mapped, 5, SIDE / 2, SIDE / 2)[RED]);
		VOE_TEST_CHECK(memcmp(pixel_at(mapped, 7, 0, 0),
				      pixel_at(mapped, 0, 0, 0), IMAGE_BYTES) == 0);
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
