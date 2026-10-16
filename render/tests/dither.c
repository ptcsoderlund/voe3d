// A DRAW KEEPS A SHARE OF ITS PIXELS (ADR-0432 point 5). An unlit quad filling
// the 64×64 picture, its object record's `dither` set, drawn four ways:
//
// 1. red at dither 0: every pixel red, as a zeroed record has always drawn;
// 2. red at 0.5: red pixels within 1% of half the picture, the rest the clear
//    colour;
// 3. red at 0.5, then green at the same place at −0.5 in the same pass: no
//    pixel the clear colour, red and green each within 1% of half, which is
//    the two levels of a model covering each pixel exactly once;
// 4. red at 0.25: red within 1% of a quarter.
//
// A drawn pixel is its colour on every channel (unlit, exposure ×1 and Clip);
// an undrawn one is the clear colour, near black. The camera and quad are
// cutout_far.c's: eye at the origin looking down −Z, 90°, the quad at z = −1
// larger than the picture.
//
// It runs headless and reads the window target back, as cutout_far.c does, and
// includes render's internal header for the same reason. A machine with no
// usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <stdlib.h>

#define SIDE 64
#define PIXELS (SIDE * SIDE)
#define IMAGE_BYTES ((VkDeviceSize)PIXELS * 4)
#define CASES 4

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// 1% of the picture: how far a share's count may be from what it asked.
#define TOLERANCE (PIXELS / 100)

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 2,
	.shadings = 2,
	.passes = 1,
};

// A unit quad facing +Z, wound counter-clockwise from the camera.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
	{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0 } },
	{ { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0 } },
	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0 } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// Eye at the origin looking down −Z, reverse-Z with a 90° field, as mips.c's.
static voe_render_view the_camera(void)
{
	voe_render_view view = { 0 };
	float span = FAR_PLANE - NEAR_PLANE;

	view.view = voe_math_float4x4_identity();
	view.projection.m[0][0] = 1.0f;
	view.projection.m[1][1] = 1.0f;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The quad at z = −1, half-side 2, wider than the picture's 2 units, wearing
// `shading` and keeping `dither` of its pixels.
static voe_render_object filling(voe_render_shading shading, float dither)
{
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.dither = dither,
	};

	object.world.m[0][0] = 2.0f;
	object.world.m[1][1] = 2.0f;
	object.world.m[2][3] = -1.0f;
	return object;
}

// `count` objects drawn in order in one camera pass, the picture read back into
// `buffer` at `offset`.
static void draw_case(voe_render_device *device, voe_render_geometry quad,
		      const voe_render_object *objects, uint32_t count,
		      VkBuffer buffer, VkDeviceSize offset)
{
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = { .view = the_camera() };
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
	for (uint32_t i = 0; i < count; i++)
		VOE_TEST_CHECK(voe_render_frame_draw(device, quad, objects[i]));
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

// The channel order of VK_FORMAT_B8G8R8A8_SRGB, which the headless device takes.
enum { BLUE, GREEN, RED };

// What one picture holds: pixels red, green, and the clear colour (every
// channel under 64, near black).
typedef struct {
	uint32_t red;
	uint32_t green;
	uint32_t clear;
} counts;

static counts count(const uint8_t *image)
{
	counts c = { 0 };

	for (uint32_t i = 0; i < PIXELS; i++) {
		const uint8_t *p = image + (size_t)i * 4;

		if (p[RED] >= 253 && p[GREEN] <= 2 && p[BLUE] <= 2)
			c.red++;
		else if (p[GREEN] >= 253 && p[RED] <= 2 && p[BLUE] <= 2)
			c.green++;
		else if (p[0] < 64 && p[1] < 64 && p[2] < 64)
			c.clear++;
	}
	return c;
}

static bool near_to(uint32_t got, uint32_t want)
{
	return abs((int)got - (int)want) <= TOLERANCE;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad = { 0 };
	voe_render_shading red = { 0 };
	voe_render_shading green = { 0 };
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
		.unlit = 1,
	};
	void *mapped = NULL;

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
	VOE_TEST_CHECK(voe_render_geometry_create(device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &quad,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &red, &error));
	values.base_colour = (voe_math_float4){ 0.0f, 1.0f, 0.0f, 1.0f };
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &green,
						 &error));

	if (copy_image_to_buffer != NULL && readback.buffer != VK_NULL_HANDLE) {
		voe_render_object whole = filling(red, 0.0f);
		voe_render_object half = filling(red, 0.5f);
		voe_render_object both[2] = { filling(red, 0.5f),
					      filling(green, -0.5f) };
		voe_render_object quarter = filling(red, 0.25f);

		draw_case(device, quad, &whole, 1, readback.buffer, 0);
		draw_case(device, quad, &half, 1, readback.buffer, IMAGE_BYTES);
		draw_case(device, quad, both, 2, readback.buffer,
			  IMAGE_BYTES * 2);
		draw_case(device, quad, &quarter, 1, readback.buffer,
			  IMAGE_BYTES * 3);

		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
							    readback.memory, 0,
							    VK_WHOLE_SIZE, 0,
							    &mapped),
				   VK_SUCCESS);
		VOE_TEST_CHECK_INT(device->format.format,
				   VK_FORMAT_B8G8R8A8_SRGB);
		if (mapped != NULL) {
			const uint8_t *pixels = mapped;
			counts c[CASES];

			for (uint32_t i = 0; i < CASES; i++) {
				c[i] = count(pixels + IMAGE_BYTES * i);
				printf("case %u: red %u green %u clear %u of %u\n",
				       i + 1, c[i].red, c[i].green, c[i].clear,
				       PIXELS);
			}
			VOE_TEST_CHECK_INT(c[0].red, PIXELS);
			VOE_TEST_CHECK(near_to(c[1].red, PIXELS / 2));
			VOE_TEST_CHECK_INT(c[1].red + c[1].clear, PIXELS);
			VOE_TEST_CHECK_INT(c[2].clear, 0);
			VOE_TEST_CHECK(near_to(c[2].red, PIXELS / 2));
			VOE_TEST_CHECK(near_to(c[2].green, PIXELS / 2));
			VOE_TEST_CHECK(near_to(c[3].red, PIXELS / 4));
			voe_render_vk.unmap_memory(device->device,
						   readback.memory);
		}
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
