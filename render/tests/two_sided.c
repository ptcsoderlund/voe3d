// A DRAW CAN SHOW BOTH SIDES (ADR-0421 point 2). One lit white quad turned
// half a turn about Y, so it faces −Z and the camera on +Z sees its back, read
// back in four frames:
//
// 1. drawn one-sided under a sun from the camera's side: the centre is the
//    clear colour, the back face culled;
// 2. after voe_render_frame_two_sided(true), the same: drawn and lit, its
//    normal turned round towards the camera;
// 3. the same with the sun behind the quad: darker than 2, which is what tells
//    a turned normal from one left as it was;
// 4. two passes in one frame, the first two-sided drawing the quad on the left
//    and the second, with no call, on the right: the left lit, the right the
//    clear colour, because every pass opens culling back faces.
//
// Built the way vertex_colours.c builds its device, camera and readback,
// including render's internal header for the same reason: reading a target
// back is not part of render's surface.
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
#define CASES 4

// VK_FORMAT_B8G8R8A8_SRGB, which the headless device takes.
#define BLUE 0
#define GREEN 1
#define RED 2

// How far a channel may be from the clear colour's byte: a driver's rounding.
#define TOLERANCE 3
// How bright a lit channel is at the least, and by how much the sun from the
// camera's side beats the sun behind.
#define LIT 32
#define MARGIN 16

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 2,
	.shadings = 1,
	.passes = 2,
};

#define EYE_Z 3.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// The columns the left and right quads of case 4 are read at: x = ∓1 at the
// quad's depth, three metres out under a 60° field.
#define LEFT_COLUMN 14
#define RIGHT_COLUMN 50

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

// One pass of a frame: where the quad is along x, whether the pass is
// two-sided, and its sun.
struct shown {
	float x;
	bool two_sided;
	voe_render_light light;
};

// The quad counter-clockwise from +Z, normal +Z, zeroed colours.
static voe_render_geometry make_quad(voe_render_device *device)
{
	static const float corners[4][2] = { { -1, 1 }, { 1, 1 }, { 1, -1 },
					     { -1, -1 } };
	voe_render_vertex vertices[4];
	voe_render_geometry quad = { 0 };
	voe_base_error error = VOE_BASE_OK;

	for (int i = 0; i < 4; i++)
		vertices[i] = (voe_render_vertex){
			.position = { corners[i][0], corners[i][1], 0 },
			.normal = { 0, 0, 1 },
			.uv = { (corners[i][0] + 1) * 0.5f,
				(1 - corners[i][1]) * 0.5f },
		};
	VOE_TEST_CHECK(voe_render_geometry_create(device, vertices, 4,
						  QUAD_INDICES, 6, &quad,
						  &error));
	return quad;
}

// On the +Z axis looking down −Z, reverse-Z, as vertex_colours.c's camera.
static voe_render_view the_camera(VkExtent2D extent)
{
	voe_render_view view = { .eye = { 0.0f, 0.0f, EYE_Z } };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float aspect = (float)extent.width / (float)extent.height;
	float span = FAR_PLANE - NEAR_PLANE;

	view.view.m[0][0] = 1.0f;
	view.view.m[1][1] = 1.0f;
	view.view.m[2][2] = 1.0f;
	view.view.m[2][3] = -EYE_Z;
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = focal / aspect;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The quad's record: half a turn about Y, so it faces −Z, moved `x` along X.
// The rotation is its own inverse transpose, so it is the normal matrix too.
static voe_render_object turned_away(voe_render_shading shading, float x)
{
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};

	object.world.m[0][0] = -1.0f;
	object.world.m[2][2] = -1.0f;
	object.world.m[0][3] = x;
	object.normal.m[0][0] = -1.0f;
	object.normal.m[2][2] = -1.0f;
	return object;
}

// The window's picture copied into `buffer` at `offset`, after the frame.
static void read_back(voe_render_device *device,
		      const struct voe_render_frame *frame, VkBuffer buffer,
		      VkDeviceSize offset)
{
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

// One frame of `count` passes onto the window, each drawing the quad as
// `passes` says, then the picture copied into the buffer at `offset`.
static void draw_frame(voe_render_device *device, voe_render_geometry quad,
		       voe_render_shading shading, const struct shown *passes,
		       uint32_t count, VkBuffer buffer, VkDeviceSize offset)
{
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	for (uint32_t p = 0; p < count; p++) {
		voe_render_pass_camera camera = {
			.view = the_camera(device->resolution),
			.light = passes[p].light,
		};

		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		if (passes[p].two_sided)
			voe_render_frame_two_sided(device, true);
		VOE_TEST_CHECK(voe_render_frame_draw(
			device, quad, turned_away(shading, passes[p].x)));
		voe_render_pass_end(device);
	}
	VOE_TEST_CHECK(voe_render_frame_end(device));
	voe_render_vk.device_wait_idle(device->device);
	read_back(device, frame, buffer, offset);
}

static const unsigned char *pixel_of(const unsigned char *pixels, int image,
				     int column)
{
	return pixels + IMAGE_BYTES * (VkDeviceSize)image +
	       ((size_t)(SIDE / 2) * SIDE + (size_t)column) * 4;
}

// `pixel` within TOLERANCE of image `image`'s top-left corner, the clear colour.
static void check_clear(const unsigned char *pixels, int image,
			const unsigned char *pixel)
{
	const unsigned char *corner = pixels + IMAGE_BYTES * (VkDeviceSize)image;

	for (int c = 0; c < 3; c++)
		VOE_TEST_CHECK(abs((int)pixel[c] - (int)corner[c]) <= TOLERANCE);
}

static void check_lit(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] > LIT);
	VOE_TEST_CHECK(pixel[GREEN] > LIT);
	VOE_TEST_CHECK(pixel[BLUE] > LIT);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad;
	voe_render_shading lit = { 0 };
	voe_render_shading_values white = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	// Travelling down −Z from the camera's side, and up +Z from behind.
	const voe_render_light front = {
		.direction = { 0.0f, 0.0f, -1.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	const voe_render_light behind = {
		.direction = { 0.0f, 0.0f, 1.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	const struct shown one_sided[1] = { { 0.0f, false, front } };
	const struct shown two_sided[1] = { { 0.0f, true, front } };
	const struct shown sun_behind[1] = { { 0.0f, true, behind } };
	const struct shown following[2] = { { -1.0f, true, front },
					     { 1.0f, false, front } };
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

	quad = make_quad(device);
	VOE_TEST_CHECK(voe_render_shading_create(device, white, &lit, &error));

	draw_frame(device, quad, lit, one_sided, 1, readback.buffer, 0);
	draw_frame(device, quad, lit, two_sided, 1, readback.buffer,
		   IMAGE_BYTES);
	draw_frame(device, quad, lit, sun_behind, 1, readback.buffer,
		   IMAGE_BYTES * 2);
	draw_frame(device, quad, lit, following, 2, readback.buffer,
		   IMAGE_BYTES * 3);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		const unsigned char *pixels = mapped;
		const unsigned char *shown = pixel_of(pixels, 1, SIDE / 2);
		const unsigned char *dark = pixel_of(pixels, 2, SIDE / 2);

		check_clear(pixels, 0, pixel_of(pixels, 0, SIDE / 2));
		check_lit(shown);
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK((int)shown[c] > (int)dark[c] + MARGIN);
		check_lit(pixel_of(pixels, 3, LEFT_COLUMN));
		check_clear(pixels, 3, pixel_of(pixels, 3, RIGHT_COLUMN));
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
