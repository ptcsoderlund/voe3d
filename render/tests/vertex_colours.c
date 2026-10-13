// A VERTEX'S COLOUR TINTS ITS SURFACE (ADR-0421 point 1). One quad facing +Z,
// seen head-on from +Z, read at the centre of four pictures:
//
// 1. an unlit white record on a quad whose vertices carry red: red;
// 2. the same record on a quad whose colours are zeroed: white, because a
//    zeroed complement is opaque white;
// 3. a lit white record, roughness 1, under a sun from +Z, on a quad whose
//    vertices carry half grey;
// 4. the same on the zeroed quad: case 3 reads about half its brightness in
//    linear light, within 0.4 to 0.6 (the dielectric's specular is not tinted).
//
// Built the way surface_maps.c builds its device, camera and readback,
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

// How far a channel may be from the expected byte: a driver's rounding on the
// way into an sRGB target.
#define TOLERANCE 3

static const voe_render_capacities CAPACITIES = {
	.vertices = 12,
	.indices = 18,
	.geometries = 3,
	.objects = 1,
	.shadings = 2,
	.passes = 1,
};

#define EYE_Z 3.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// The complement of red, of half grey and of white, rgba.
static const uint16_t RED_COMPLEMENT[4] = { 0, 65535, 65535, 0 };
static const uint16_t GREY_COMPLEMENT[4] = { 32768, 32768, 32768, 0 };
static const uint16_t WHITE_COMPLEMENT[4] = { 0, 0, 0, 0 };

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

// The quad counter-clockwise from +Z, every vertex carrying `complement`.
static voe_render_geometry make_quad(voe_render_device *device,
				     const uint16_t complement[4])
{
	static const float corners[4][2] = { { -1, 1 }, { 1, 1 }, { 1, -1 },
					     { -1, -1 } };
	voe_render_vertex vertices[4];
	voe_render_geometry quad = { 0 };
	voe_base_error error = VOE_BASE_OK;

	for (int i = 0; i < 4; i++) {
		vertices[i] = (voe_render_vertex){
			.position = { corners[i][0], corners[i][1], 0 },
			.normal = { 0, 0, 1 },
			.uv = { (corners[i][0] + 1) * 0.5f,
				(1 - corners[i][1]) * 0.5f },
		};
		for (int c = 0; c < 4; c++)
			vertices[i].colour_complement[c] = complement[c];
	}
	VOE_TEST_CHECK(voe_render_geometry_create(device, vertices, 4,
						  QUAD_INDICES, 6, &quad,
						  &error));
	return quad;
}

// On the +Z axis looking down −Z, reverse-Z, as surface_maps.c's camera.
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

// One pass drawing `quad` with the given shading and light, then the target
// copied into the buffer at `offset`. Two submits and an idle between them.
static void draw_case(voe_render_device *device, voe_render_geometry quad,
		      voe_render_shading shading, voe_render_light light,
		      VkBuffer buffer, VkDeviceSize offset)
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
		.view = the_camera(device->resolution),
		.light = light,
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
	VOE_TEST_CHECK(voe_render_frame_draw(device, quad, object));
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

static const unsigned char *centre_of(const unsigned char *pixels, int image)
{
	return pixels + IMAGE_BYTES * (VkDeviceSize)image +
	       ((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4;
}

static void check_red(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] >= 255 - TOLERANCE);
	VOE_TEST_CHECK(pixel[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] <= TOLERANCE);
}

static void check_white(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] >= 255 - TOLERANCE);
	VOE_TEST_CHECK(pixel[GREEN] >= 255 - TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] >= 255 - TOLERANCE);
}

// An sRGB byte as linear light.
static float linear_of(unsigned char byte)
{
	float c = (float)byte / 255.0f;

	return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f);
}

// Each channel of `grey` about half of `white`'s, in linear light.
static void check_half(const unsigned char *grey, const unsigned char *white)
{
	const int channels[3] = { RED, GREEN, BLUE };

	for (int i = 0; i < 3; i++) {
		float ratio = linear_of(grey[channels[i]]) /
			      linear_of(white[channels[i]]);

		VOE_TEST_CHECK(white[channels[i]] > 32);
		VOE_TEST_CHECK(ratio > 0.4f && ratio < 0.6f);
	}
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry red_quad, grey_quad, white_quad;
	voe_render_shading unlit = { 0 };
	voe_render_shading lit = { 0 };
	voe_render_shading_values white = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	// Travelling down −Z into the face: N·L of one.
	voe_render_light sun = {
		.direction = { 0.0f, 0.0f, -1.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
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

	red_quad = make_quad(device, RED_COMPLEMENT);
	grey_quad = make_quad(device, GREY_COMPLEMENT);
	white_quad = make_quad(device, WHITE_COMPLEMENT);
	VOE_TEST_CHECK(voe_render_shading_create(device, white, &unlit, &error));
	white.unlit = 0;
	VOE_TEST_CHECK(voe_render_shading_create(device, white, &lit, &error));

	draw_case(device, red_quad, unlit, sun, readback.buffer, 0);
	draw_case(device, white_quad, unlit, sun, readback.buffer, IMAGE_BYTES);
	draw_case(device, grey_quad, lit, sun, readback.buffer, IMAGE_BYTES * 2);
	draw_case(device, white_quad, lit, sun, readback.buffer,
		  IMAGE_BYTES * 3);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		check_red(centre_of(mapped, 0));
		check_white(centre_of(mapped, 1));
		check_half(centre_of(mapped, 2), centre_of(mapped, 3));
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
