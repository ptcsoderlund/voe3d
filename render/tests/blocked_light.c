// A PASS'S LIGHT BLOCKERS KEEP OUTSIDE LIGHT OUT OF THEIR BOXES (ADR-0347).
// A grey ground quad seen straight down from 10 m, lit by a low sun and a fill
// so both show; a box over its left half, x −11 to 0, y −1 to 1. Eight draws:
//
// 0. no blockers: the reference picture;
// 1. the box: the left black, the right byte for byte the reference's;
// 2. the box's array with a count of 0, and 3. one box 500 m off: the
//    reference exactly;
// 4. under a dark sun, a red point light outside the box, range over both
//    halves: the right red, the left black;
// 5. the same light inside the box: the left red, the right black;
// 6. an unshaded pass with the box: the base colour, as 7. without it.
//
// Built the way point_lights.c builds its device and reads its target back,
// with render's internal header for the same reason.
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

#define TOLERANCE 3

// Columns 0 to 30 lie wholly left of x = 0, 33 to 63 wholly right of it; 28
// and 35 are 1.1 m either side.
#define LEFT_LAST 30
#define RIGHT_FIRST 33
#define LEFT_PIXEL 28
#define RIGHT_PIXEL 35

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

// An unturned box of centre `c` and half sizes `h`: rows (a_i / h_i,
// −a_i·c / h_i) and a sphere of radius |h|.
static voe_render_light_blocker box(voe_math_float3 c, voe_math_float3 h)
{
	return (voe_render_light_blocker){
		.rows = {
			{ 1.0f / h.x, 0.0f, 0.0f, -c.x / h.x },
			{ 0.0f, 1.0f / h.y, 0.0f, -c.y / h.y },
			{ 0.0f, 0.0f, 1.0f / h.z, -c.z / h.z },
		},
		.sphere = { c.x, c.y, c.z, sqrtf(voe_math_float3_dot(h, h)) },
	};
}

// One pass with the given light, points and blockers, then the target copied
// into the buffer at `offset`. Two submits and an idle between them.
static void draw_case(voe_render_device *device, voe_render_geometry ground,
		      voe_render_shading shading, voe_render_light light,
		      voe_render_point_lights points,
		      voe_render_light_blockers blockers, VkBuffer buffer,
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
		.blockers = blockers,
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

static const unsigned char *pixel_at(const unsigned char *pixels, int image,
				     int column, int row)
{
	return pixels + IMAGE_BYTES * (VkDeviceSize)image +
	       ((size_t)row * SIDE + (size_t)column) * 4;
}

static void check_black(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] <= TOLERANCE);
}

static void check_red(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] > 40);
	VOE_TEST_CHECK(pixel[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] <= TOLERANCE);
}

// Every pixel of `image` left of x = 0 black, and every one right of it
// byte for byte the reference's, which is lit there.
static void check_left_blocked(const unsigned char *pixels, int image)
{
	for (int row = 0; row < SIDE; row++) {
		for (int column = 0; column <= LEFT_LAST; column++)
			check_black(pixel_at(pixels, image, column, row));
		VOE_TEST_CHECK(memcmp(pixel_at(pixels, image, RIGHT_FIRST, row),
				      pixel_at(pixels, 0, RIGHT_FIRST, row),
				      (SIDE - RIGHT_FIRST) * 4) == 0);
	}
	VOE_TEST_CHECK(pixel_at(pixels, 0, RIGHT_PIXEL, SIDE / 2)[RED] >
		       TOLERANCE * 4);
	VOE_TEST_CHECK(pixel_at(pixels, 0, LEFT_PIXEL, SIDE / 2)[RED] >
		       TOLERANCE * 4);
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
	// Low from +X, N·L 0.1, so the fill still shows under it.
	voe_render_light sun = {
		.direction = { -0.99498744f, -0.1f, 0.0f },
		.intensity = 3.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
		.fill = { 0.2f, 0.2f, 0.2f },
	};
	voe_render_light dark = {
		.direction = { 0.0f, -1.0f, 0.0f },
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	voe_render_light unshaded = dark;
	const voe_render_light_blocker left[1] = {
		box((voe_math_float3){ -5.5f, 0.0f, 0.0f },
		    (voe_math_float3){ 5.5f, 1.0f, 11.0f }),
	};
	const voe_render_light_blocker far[1] = {
		box((voe_math_float3){ 0.0f, 0.0f, 500.0f },
		    (voe_math_float3){ 1.0f, 1.0f, 1.0f }),
	};
	const voe_render_point_light outside[1] = {
		{ .position = { 2.0f, 0.5f, 0.0f }, .range = 5.0f,
		  .colour = { 3.0f, 0.0f, 0.0f }, .falloff = 1.0f },
	};
	const voe_render_point_light inside[1] = {
		{ .position = { -2.0f, 0.5f, 0.0f }, .range = 5.0f,
		  .colour = { 3.0f, 0.0f, 0.0f }, .falloff = 1.0f },
	};
	const voe_render_point_lights no_points = { 0 };
	const voe_render_light_blockers none = { 0 };
	const voe_render_light_blockers boxed = { .blockers = left, .count = 1 };
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

	draw_case(device, ground, shading, sun, no_points, none,
		  readback.buffer, 0);
	draw_case(device, ground, shading, sun, no_points, boxed,
		  readback.buffer, IMAGE_BYTES);
	draw_case(device, ground, shading, sun, no_points,
		  (voe_render_light_blockers){ .blockers = left, .count = 0 }, readback.buffer,
		  IMAGE_BYTES * 2);
	draw_case(device, ground, shading, sun, no_points,
		  (voe_render_light_blockers){ .blockers = far, .count = 1 }, readback.buffer,
		  IMAGE_BYTES * 3);
	draw_case(device, ground, shading, dark,
		  (voe_render_point_lights){ outside, 1 }, boxed,
		  readback.buffer, IMAGE_BYTES * 4);
	draw_case(device, ground, shading, dark,
		  (voe_render_point_lights){ inside, 1 }, boxed,
		  readback.buffer, IMAGE_BYTES * 5);
	draw_case(device, ground, shading, unshaded, no_points, boxed,
		  readback.buffer, IMAGE_BYTES * 6);
	draw_case(device, ground, shading, unshaded, no_points, none,
		  readback.buffer, IMAGE_BYTES * 7);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		check_left_blocked(mapped, 1);
		VOE_TEST_CHECK(memcmp(pixel_at(mapped, 2, 0, 0),
				      pixel_at(mapped, 0, 0, 0), IMAGE_BYTES) == 0);
		VOE_TEST_CHECK(memcmp(pixel_at(mapped, 3, 0, 0),
				      pixel_at(mapped, 0, 0, 0), IMAGE_BYTES) == 0);
		check_red(pixel_at(mapped, 4, RIGHT_PIXEL, SIDE / 2));
		check_black(pixel_at(mapped, 4, LEFT_PIXEL, SIDE / 2));
		check_red(pixel_at(mapped, 5, LEFT_PIXEL, SIDE / 2));
		check_black(pixel_at(mapped, 5, RIGHT_PIXEL, SIDE / 2));
		VOE_TEST_CHECK(memcmp(pixel_at(mapped, 6, 0, 0),
				      pixel_at(mapped, 7, 0, 0), IMAGE_BYTES) == 0);
		VOE_TEST_CHECK(pixel_at(mapped, 6, LEFT_PIXEL, SIDE / 2)[RED] >
			       100);
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
