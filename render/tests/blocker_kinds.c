// A BLOCKER'S KIND GATES LIGHT BY ROOMS AND CROSSINGS (ADR-0350 points 3, 4).
// A grey ground quad seen straight down from 10 m, lit by a low sun from +X and
// a fill so both show. Seven draws:
//
// 0. no blockers: the reference picture;
// 1. a Wall floating 2 m up, x 0 to 20, z −2 to 2: the ground in its shadow
//    along the sun reads the fill alone, not black and below sunlit; the
//    ground under it, whose ray passes beneath, and every row clear of z ±2
//    read the reference;
// 2. under a dark sun, a red point light beside a Wall standing on the ground:
//    behind the Wall black, beside it red;
// 3. an Indoors box over the left half: the left sunlit but darker than the
//    reference, its fill gone; the right byte for byte the reference's;
// 4. the same with the sun's intensity 0: the left black, the right the fill;
// 5. a Room box over the left half with `sun` naming it: the left the
//    reference, the right black, fill included;
// 6. the same box, the three words zero: blocked_light's case 1 picture.
//
// Built the way blocked_light.c builds its device and reads its target back,
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
#define CASES 7

// VK_FORMAT_B8G8R8A8_SRGB, which the headless device takes.
#define BLUE 0
#define GREEN 1
#define RED 2

#define TOLERANCE 3

// 0.3125 m a pixel, column c at x (c − 31.5) × 0.3125. Columns 0 to 30 lie
// wholly left of x = 0, 33 to 63 wholly right of it; 28 and 35 are 1.1 m
// either side, 36 is 1.4 m right, 16 and 48 about 5 m either side. Row 32 is
// on z = 0; rows 0 to 19 are more than 3.5 m from it.
#define LEFT_LAST 30
#define RIGHT_FIRST 33
#define LEFT_PIXEL 28
#define RIGHT_PIXEL 35
#define BEHIND_PIXEL 36
#define SHADOW_PIXEL 16
#define UNDER_PIXEL 48
#define MIDDLE_ROW 32
#define CLEAR_ROWS 20

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

// A grey pixel well clear of black.
static void check_lit(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] > TOLERANCE * 4);
	VOE_TEST_CHECK(pixel[GREEN] > TOLERANCE * 4);
	VOE_TEST_CHECK(pixel[BLUE] > TOLERANCE * 4);
}

// Every pixel of `image` in columns [first, last] of every row byte for byte
// the reference's.
static void check_columns_reference(const unsigned char *pixels, int image,
				    int first, int last)
{
	for (int row = 0; row < SIDE; row++)
		VOE_TEST_CHECK(memcmp(pixel_at(pixels, image, first, row),
				      pixel_at(pixels, 0, first, row),
				      (size_t)(last - first + 1) * 4) == 0);
}

// Every pixel of `image` in columns [first, last] of every row black.
static void check_columns_black(const unsigned char *pixels, int image,
				int first, int last)
{
	for (int row = 0; row < SIDE; row++)
		for (int column = first; column <= last; column++)
			check_black(pixel_at(pixels, image, column, row));
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
	voe_render_light sun_off = sun;
	voe_render_light dark = {
		.direction = { 0.0f, -1.0f, 0.0f },
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	// The sun's ray from x ≈ −5 rises into it near x 15; from x ≈ 5 it is
	// still under 2 m at x 20.
	const voe_render_light_blocker floating[1] = {
		box((voe_math_float3){ 10.0f, 3.0f, 0.0f },
		    (voe_math_float3){ 10.0f, 1.0f, 2.0f }),
	};
	// x 0.5 to 0.8, from under the ground to 2 m up: between the lamp and
	// column 36, not between it and column 28.
	const voe_render_light_blocker standing[1] = {
		box((voe_math_float3){ 0.65f, 0.5f, 0.0f },
		    (voe_math_float3){ 0.15f, 1.5f, 3.0f }),
	};
	const voe_render_light_blocker left[1] = {
		box((voe_math_float3){ -5.5f, 0.0f, 0.0f },
		    (voe_math_float3){ 5.5f, 1.0f, 11.0f }),
	};
	const voe_render_point_light lamp[1] = {
		{ .position = { 0.0f, 0.5f, 0.0f }, .range = 5.0f,
		  .colour = { 3.0f, 0.0f, 0.0f }, .falloff = 1.0f },
	};
	const voe_render_point_lights no_points = { 0 };
	const voe_render_light_blockers none = { 0 };
	const voe_render_light_blockers indoors = { .blockers = left, .count = 1,
						    .indoors = 1 };
	void *mapped = NULL;

	sun_off.intensity = 0.0f;
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

	VOE_TEST_CHECK(voe_render_geometry_create(device, GROUND_VERTICES, 4,
						  GROUND_INDICES, 6, &ground,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));

	draw_case(device, ground, shading, sun, no_points, none,
		  readback.buffer, 0);
	draw_case(device, ground, shading, sun, no_points,
		  (voe_render_light_blockers){ .blockers = floating, .count = 1,
					       .walls = 1 },
		  readback.buffer, IMAGE_BYTES);
	draw_case(device, ground, shading, dark,
		  (voe_render_point_lights){ lamp, 1 },
		  (voe_render_light_blockers){ .blockers = standing, .count = 1,
					       .walls = 1 },
		  readback.buffer, IMAGE_BYTES * 2);
	draw_case(device, ground, shading, sun, no_points, indoors,
		  readback.buffer, IMAGE_BYTES * 3);
	draw_case(device, ground, shading, sun_off, no_points, indoors,
		  readback.buffer, IMAGE_BYTES * 4);
	draw_case(device, ground, shading, sun, no_points,
		  (voe_render_light_blockers){ .blockers = left, .count = 1,
					       .sun = 1 },
		  readback.buffer, IMAGE_BYTES * 5);
	draw_case(device, ground, shading, sun, no_points,
		  (voe_render_light_blockers){ .blockers = left, .count = 1 },
		  readback.buffer, IMAGE_BYTES * 6);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		const unsigned char *shadowed =
			pixel_at(mapped, 1, SHADOW_PIXEL, MIDDLE_ROW);
		const unsigned char *indoors_lit =
			pixel_at(mapped, 3, LEFT_PIXEL, MIDDLE_ROW);

		check_lit(shadowed);
		VOE_TEST_CHECK(shadowed[RED] <
			       pixel_at(mapped, 0, SHADOW_PIXEL, MIDDLE_ROW)[RED]);
		VOE_TEST_CHECK(memcmp(pixel_at(mapped, 1, UNDER_PIXEL, MIDDLE_ROW),
				      pixel_at(mapped, 0, UNDER_PIXEL, MIDDLE_ROW),
				      4) == 0);
		VOE_TEST_CHECK(memcmp(pixel_at(mapped, 1, 0, 0),
				      pixel_at(mapped, 0, 0, 0),
				      IMAGE_BYTES * CLEAR_ROWS / SIDE) == 0);

		check_black(pixel_at(mapped, 2, BEHIND_PIXEL, MIDDLE_ROW));
		check_red(pixel_at(mapped, 2, LEFT_PIXEL, MIDDLE_ROW));

		check_lit(indoors_lit);
		VOE_TEST_CHECK(indoors_lit[RED] <
			       pixel_at(mapped, 0, LEFT_PIXEL, MIDDLE_ROW)[RED]);
		check_columns_reference(mapped, 3, RIGHT_FIRST, SIDE - 1);

		check_black(pixel_at(mapped, 4, LEFT_PIXEL, MIDDLE_ROW));
		check_lit(pixel_at(mapped, 4, RIGHT_PIXEL, MIDDLE_ROW));

		check_columns_reference(mapped, 5, 0, LEFT_LAST);
		check_columns_black(mapped, 5, RIGHT_FIRST, SIDE - 1);

		check_columns_black(mapped, 6, 0, LEFT_LAST);
		check_columns_reference(mapped, 6, RIGHT_FIRST, SIDE - 1);
		check_lit(pixel_at(mapped, 0, LEFT_PIXEL, MIDDLE_ROW));
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
