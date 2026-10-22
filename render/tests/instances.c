// COPIES OF A MESH IN ONE DRAW (ADR-0442 point 6). Three instance records place
// a small unlit white quad at three places along the picture's middle row, and
// one voe_render_frame_draw_instanced draws them all:
//
// 1. each place reads white and a spot between two of them the clear colour, so
//    every copy was placed by its own rows and not by the record's `world`,
//    which would have covered the whole picture;
// 2. the frame held one draw command and three quads' triangles, six;
// 3. on a device of `instances` 3, with the three written, a second write of
//    one is refused, `first` untouched, and the frame still draws and ends.
//
// The camera is dither.c's: eye at the origin looking down −Z, 90°, so at
// z = −1 the picture spans −1..1 on both axes. A copy is the unit quad scaled
// by 0.2 at x −0.6, 0 and 0.6, about 13 pixels wide of 64.
//
// It runs headless and reads the window target back, as unshaded.c does, and
// includes render's internal header for the same reason. A machine with no
// usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>

#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)
#define COPIES 3

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define SCALE 0.2f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
	.instances = COPIES,
};

// A unit quad facing +Z, wound counter-clockwise from the camera.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
	{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0 } },
	{ { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0 } },
	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0 } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

// Where each copy stands along x, and the pixel columns of their centres and of
// the spots between them.
static const float PLACES[COPIES] = { -0.6f, 0.0f, 0.6f };
static const int CENTRES[COPIES] = { 13, 32, 51 };
static const int BETWEEN[2] = { 22, 42 };

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// Eye at the origin looking down −Z, reverse-Z with a 90° field, as dither.c's.
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

// Copy i: the quad scaled by SCALE at (PLACES[i], 0, −1), drawn whole.
static voe_render_instance copy_at(int i)
{
	voe_render_instance copy = { 0 };

	copy.rows[0] = (voe_math_float4){ SCALE, 0.0f, 0.0f, PLACES[i] };
	copy.rows[1] = (voe_math_float4){ 0.0f, SCALE, 0.0f, 0.0f };
	copy.rows[2] = (voe_math_float4){ 0.0f, 0.0f, SCALE, -1.0f };
	return copy;
}

// The frame: three copies written, a fourth refused, one instanced draw, then
// the picture read back into `buffer`.
static void draw_frame(voe_render_device *device, voe_render_geometry quad,
		       voe_render_object object, VkBuffer buffer)
{
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = { .view = the_camera() };
	voe_render_instance copies[COPIES];
	uint32_t first = 99;
	uint32_t refused = 77;
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
	bool drawing = false;

	for (int i = 0; i < COPIES; i++)
		copies[i] = copy_at(i);

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_frame_instances(device, copies, COPIES, &first));
	VOE_TEST_CHECK_INT(first, 0);
	VOE_TEST_CHECK(!voe_render_frame_instances(device, copies, 1, &refused));
	VOE_TEST_CHECK_INT(refused, 77);
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw_instanced(device, quad, object, first,
						       COPIES));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	VOE_TEST_CHECK_INT(voe_render_frame_triangle_count(device), 2 * COPIES);
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

// The pixel at column `x` of the middle row.
static const uint8_t *middle_at(const uint8_t *image, int x)
{
	return image + ((size_t)(SIDE / 2) * SIDE + (size_t)x) * 4;
}

static bool white(const uint8_t *p)
{
	return p[0] >= 253 && p[1] >= 253 && p[2] >= 253;
}

// The clear colour: every channel under 64, near black.
static bool clear(const uint8_t *p)
{
	return p[0] < 64 && p[1] < 64 && p[2] < 64;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad = { 0 };
	voe_render_shading shading = { 0 };
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
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
		device, &readback, IMAGE_BYTES, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		"test readback"));
	VOE_TEST_CHECK(voe_render_geometry_create(device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &quad,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));

	if (copy_image_to_buffer != NULL && readback.buffer != VK_NULL_HANDLE) {
		// The record's own world fills the picture at z = −1: were it
		// read instead of the rows, the spots between would be white.
		voe_render_object object = {
			.world = voe_math_float4x4_identity(),
			.normal = voe_math_float4x4_identity(),
			.shading = shading.index,
			.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		};

		object.world.m[0][0] = 2.0f;
		object.world.m[1][1] = 2.0f;
		object.world.m[2][3] = -1.0f;
		draw_frame(device, quad, object, readback.buffer);

		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
							    readback.memory, 0,
							    VK_WHOLE_SIZE, 0,
							    &mapped),
				   VK_SUCCESS);
		if (mapped != NULL) {
			for (int i = 0; i < COPIES; i++)
				VOE_TEST_CHECK(white(middle_at(mapped, CENTRES[i])));
			for (int i = 0; i < 2; i++)
				VOE_TEST_CHECK(clear(middle_at(mapped, BETWEEN[i])));
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
