// A SURFACE READS ITS NORMAL AND EMISSION MAPS (ADR-0278 points 3 and 4). One
// quad facing +Z, seen head-on from +Z, read at the centre of five pictures:
//
// 1. lit by a sun from +Z with no normal map: the reference;
// 2. the same with a flat normal map (128, 128, 255): the reference again,
//    within the tolerance, so the tangent frame adds nothing where it should not;
// 3. with a map tilted sixty degrees towards +u: darker in every channel, N·L
//    a half where the reference had one;
// 4. a black, fully metallic base with emissive (1, 0, 0) and no emissive
//    texture, under the same sun: red, because the lit terms of a black metal
//    are nought and the emission is added whole;
// 5. the same with the sun pointing away: red still, emission unshadowed.
//
// The maps are one-pixel DATA textures, so the channel bytes arrive as written.
// Built the way unshaded.c builds its device, camera and readback, including
// render's internal header for the same reason: reading a target back is not
// part of render's surface.
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
#define CASES 5

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
	.shadings = 4,
	.passes = 1,
};

#define EYE_Z 3.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// Counter-clockwise from +Z; u runs along +x and v down, as glTF's does.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1, 1, 0 }, { 0, 0, 1 }, { 0, 0 } },
	{ { 1, 1, 0 }, { 0, 0, 1 }, { 1, 0 } },
	{ { 1, -1, 0 }, { 0, 0, 1 }, { 1, 1 } },
	{ { -1, -1, 0 }, { 0, 0, 1 }, { 0, 1 } },
};

static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

// Flat, and sixty degrees towards +u: (sin 60°, 0, cos 60°) as 0..255.
static const uint8_t FLAT_TEXEL[4] = { 128, 128, 255, 255 };
static const uint8_t TILTED_TEXEL[4] = { 238, 128, 191, 255 };

// On the +Z axis looking down −Z, reverse-Z, as unshaded.c's camera.
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

// One pass with the given shading and light, then the target copied into the
// buffer at `offset`. Two submits and an idle between them, as in unshaded.c.
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

static void check_same(const unsigned char *a, const unsigned char *b)
{
	VOE_TEST_CHECK(abs(a[RED] - b[RED]) <= TOLERANCE);
	VOE_TEST_CHECK(abs(a[GREEN] - b[GREEN]) <= TOLERANCE);
	VOE_TEST_CHECK(abs(a[BLUE] - b[BLUE]) <= TOLERANCE);
}

static void check_darker(const unsigned char *dark, const unsigned char *light)
{
	VOE_TEST_CHECK(dark[RED] + TOLERANCE < light[RED]);
	VOE_TEST_CHECK(dark[GREEN] + TOLERANCE < light[GREEN]);
	VOE_TEST_CHECK(dark[BLUE] + TOLERANCE < light[BLUE]);
}

static void check_red(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] >= 255 - TOLERANCE);
	VOE_TEST_CHECK(pixel[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] <= TOLERANCE);
}

// The four shading records: plain, flat map, tilted map, black and glowing.
static void make_shadings(voe_render_device *device, voe_render_shading *out)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_texture flat = { 0 };
	voe_render_texture tilted = { 0 };
	voe_render_shading_values plain = {
		.base_colour = { 0.6f, 0.5f, 0.4f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_shading_values glowing = {
		.base_colour = { 0.0f, 0.0f, 0.0f, 1.0f },
		.metallic = 1.0f,
		.roughness = 1.0f,
		.emissive = { 1.0f, 0.0f, 0.0f },
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};

	VOE_TEST_CHECK(voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
						 VOE_RENDER_SAMPLING_SMOOTH, 1,
						 1, FLAT_TEXEL, &flat, &error));
	VOE_TEST_CHECK(voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
						 VOE_RENDER_SAMPLING_SMOOTH, 1,
						 1, TILTED_TEXEL, &tilted,
						 &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, plain, &out[0],
						 &error));
	plain.normal_texture = flat.index;
	VOE_TEST_CHECK(voe_render_shading_create(device, plain, &out[1],
						 &error));
	plain.normal_texture = tilted.index;
	VOE_TEST_CHECK(voe_render_shading_create(device, plain, &out[2],
						 &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, glowing, &out[3],
						 &error));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad = { 0 };
	voe_render_shading shadings[4] = { 0 };
	// Travelling down −Z into the face: N·L of one.
	voe_render_light sun = {
		.direction = { 0.0f, 0.0f, -1.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	voe_render_light away = sun;
	void *mapped = NULL;

	away.direction = (voe_math_float3){ 0.0f, 0.0f, 1.0f };
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

	VOE_TEST_CHECK(voe_render_geometry_create(device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &quad,
						  &error));
	make_shadings(device, shadings);

	draw_case(device, quad, shadings[0], sun, readback.buffer, 0);
	draw_case(device, quad, shadings[1], sun, readback.buffer, IMAGE_BYTES);
	draw_case(device, quad, shadings[2], sun, readback.buffer,
		  IMAGE_BYTES * 2);
	draw_case(device, quad, shadings[3], sun, readback.buffer,
		  IMAGE_BYTES * 3);
	draw_case(device, quad, shadings[3], away, readback.buffer,
		  IMAGE_BYTES * 4);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		check_same(centre_of(mapped, 1), centre_of(mapped, 0));
		check_darker(centre_of(mapped, 2), centre_of(mapped, 0));
		check_red(centre_of(mapped, 3));
		check_red(centre_of(mapped, 4));
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
