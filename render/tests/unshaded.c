// A PASS WHOSE LIGHT SAYS `unshaded` DRAWS EVERY SURFACE IN ITS BASE COLOUR.
// That is what a scene with no light looks like (ADR-0238), and the claim is
// nine pictures of one lit cube, read at the centre of each, which is its +Z
// face:
//
// 1. no sun at all (intensity 0) and `unshaded` set: the base colour times the
//    object colour, encoded into the sRGB target, within a small tolerance;
// 2. the same light with `unshaded` clear: black, which is the zeroed light
//    every caller that never sets the flag still gets;
// 3. `unshaded` set with a real sun pointing away from the faces in view: the
//    base colour again, so the flag wins over whatever the light says;
// 4. that sun pointing away, shaded, with fill (0.25, 0.25, 0.25): a quarter of
//    the base colour, the fill lifting a face the sun does not reach (ADR-0273);
// 5. the same with fill zero: black, as before the fill existed;
// 6. the sun pointing straight at the face read, fill (0.25, 0.25, 0.25)
//    against fill zero: the same picture, the fill gone where the sun reaches;
// 7. the same with the sun meeting that face at N·L of 0.5: the same again, a
//    slanted sunlit face untinted by the fill (ADR-0275, ADR-0276).
//
// The material is lit (not `unlit`), fully rough and not metallic, so the only
// thing that can take the unlit exit is the flag under test. The colour is not
// grey, so a pass that dropped a channel shows.
//
// Built the way offscreen.c builds its device, cube and camera, cut to what one
// face at the centre of the picture needs. It includes render's internal header
// for the same reason: reading a target back is not part of render's surface.
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
#define CASES 9

// VK_FORMAT_B8G8R8A8_SRGB, which the headless device takes.
#define BLUE 0
#define GREEN 1
#define RED 2

// How far a channel may be from the expected byte: a driver's rounding on the
// way into an sRGB target.
#define TOLERANCE 3

static const voe_render_capacities CAPACITIES = {
	.vertices = 24,
	.indices = 36,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
};

#define EYE_Y 1.8f
#define EYE_Z 4.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// Counter-clockwise from outside, four vertices a face; offscreen.c's cube
// without its texture coordinates, which a white default texture ignores.
#define H 0.5f
static const voe_render_vertex CUBE_VERTICES[24] = {
	{ { -H, H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, -H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { -H, -H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { -H, H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { -H, -H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { H, -H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { H, H, H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, H, -H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, -H, -H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, -H, H }, { 1, 0, 0 }, { 0, 0 } },
	{ { -H, H, -H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, H, H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, -H, H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, -H, -H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, H, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, H, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, H, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, H, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, -H, H }, { 0, -1, 0 }, { 0, 0 } },
	{ { H, -H, H }, { 0, -1, 0 }, { 0, 0 } },
	{ { H, -H, -H }, { 0, -1, 0 }, { 0, 0 } },
	{ { -H, -H, -H }, { 0, -1, 0 }, { 0, 0 } },
};

static const uint32_t CUBE_INDICES[36] = {
	3, 2, 1, 3, 1, 0,	 7, 6, 5, 7, 5, 4,
	11, 10, 9, 11, 9, 8,	 15, 14, 13, 15, 13, 12,
	19, 18, 17, 19, 17, 16,	 23, 22, 21, 23, 21, 20,
};

// The base colour and the object's tint; the centre should read their product.
static const voe_math_float4 BASE = { 0.6f, 0.3f, 0.1f, 1.0f };
static const voe_math_float4 TINT = { 1.0f, 1.0f, 0.5f, 1.0f };

// offscreen.c's camera: above and behind the origin, looking at it, reverse-Z.
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

// One pass with the given light, then the target copied into the buffer at
// `offset`. Two submits and an idle between them, as in offscreen.c.
static void draw_case(voe_render_device *device, voe_render_geometry cube,
		      voe_render_shading shading, voe_render_light light,
		      VkBuffer buffer, VkDeviceSize offset)
{
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = TINT,
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
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object));
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

static const unsigned char *centre_of(const unsigned char *pixels, int image)
{
	return pixels + IMAGE_BYTES * (VkDeviceSize)image +
	       ((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4;
}

// The base colour times the tint, times `scale`: one for the unlit exit, the
// fill for a face lit by the fill alone.
static void check_base_colour(const unsigned char *pixel, float scale)
{
	VOE_TEST_CHECK(abs(pixel[RED] - srgb_byte(BASE.x * TINT.x * scale)) <=
		       TOLERANCE);
	VOE_TEST_CHECK(abs(pixel[GREEN] - srgb_byte(BASE.y * TINT.y * scale)) <=
		       TOLERANCE);
	VOE_TEST_CHECK(abs(pixel[BLUE] - srgb_byte(BASE.z * TINT.z * scale)) <=
		       TOLERANCE);
}

static void check_black(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] <= TOLERANCE);
}

// A sunlit face drawn with fill and without: lit at all, and the same both
// ways, because the fill is gone wherever the sun reaches.
static void check_fill_absent(const unsigned char *filled,
			      const unsigned char *unfilled)
{
	VOE_TEST_CHECK(unfilled[RED] > TOLERANCE);
	VOE_TEST_CHECK(abs(filled[RED] - unfilled[RED]) <= TOLERANCE);
	VOE_TEST_CHECK(abs(filled[GREEN] - unfilled[GREEN]) <= TOLERANCE);
	VOE_TEST_CHECK(abs(filled[BLUE] - unfilled[BLUE]) <= TOLERANCE);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry cube = { 0 };
	voe_render_shading shading = { 0 };
	voe_render_shading_values values = {
		.base_colour = BASE,
		.metallic = 0.0f,
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_light none = { .unshaded = 1 };
	voe_render_light dark = { .unshaded = 0 };
	// Travelling up and away from the eye, so the +Y and +Z faces in view
	// face away from it: black if the sun were read.
	voe_render_light away = {
		.direction = { 0.0f, 0.70710678f, 0.70710678f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
		.unshaded = 1,
	};
	// The sun above, read this time, so only the fill reaches the faces.
	voe_render_light filled = {
		.direction = { 0.0f, 0.70710678f, 0.70710678f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
		.fill = { 0.25f, 0.25f, 0.25f },
	};
	voe_render_light unfilled = {
		.direction = { 0.0f, 0.70710678f, 0.70710678f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	// Travelling straight into the +Z face read: N·L of one.
	voe_render_light head_on = {
		.direction = { 0.0f, 0.0f, -1.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
		.fill = { 0.25f, 0.25f, 0.25f },
	};
	// Travelling down and into that face at sixty degrees: N·L of a half.
	voe_render_light slanted = {
		.direction = { 0.0f, -0.8660254f, -0.5f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
		.fill = { 0.25f, 0.25f, 0.25f },
	};
	voe_render_light head_on_unfilled = head_on;
	voe_render_light slanted_unfilled = slanted;

	head_on_unfilled.fill = (voe_math_float3){ 0.0f, 0.0f, 0.0f };
	slanted_unfilled.fill = (voe_math_float3){ 0.0f, 0.0f, 0.0f };
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
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (copy_image_to_buffer == NULL || readback.buffer == VK_NULL_HANDLE) {
		voe_render_device_destroy(device);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_render_geometry_create(device, CUBE_VERTICES, 24,
						  CUBE_INDICES, 36, &cube,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, values, &shading,
						 &error));

	draw_case(device, cube, shading, none, readback.buffer, 0);
	draw_case(device, cube, shading, dark, readback.buffer, IMAGE_BYTES);
	draw_case(device, cube, shading, away, readback.buffer,
		  IMAGE_BYTES * 2);
	draw_case(device, cube, shading, filled, readback.buffer,
		  IMAGE_BYTES * 3);
	draw_case(device, cube, shading, unfilled, readback.buffer,
		  IMAGE_BYTES * 4);
	draw_case(device, cube, shading, head_on, readback.buffer,
		  IMAGE_BYTES * 5);
	draw_case(device, cube, shading, head_on_unfilled, readback.buffer,
		  IMAGE_BYTES * 6);
	draw_case(device, cube, shading, slanted, readback.buffer,
		  IMAGE_BYTES * 7);
	draw_case(device, cube, shading, slanted_unfilled, readback.buffer,
		  IMAGE_BYTES * 8);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		check_base_colour(centre_of(mapped, 0), 1.0f);
		check_black(centre_of(mapped, 1));
		check_base_colour(centre_of(mapped, 2), 1.0f);
		check_base_colour(centre_of(mapped, 3), 0.25f);
		check_black(centre_of(mapped, 4));
		check_fill_absent(centre_of(mapped, 5), centre_of(mapped, 6));
		check_fill_absent(centre_of(mapped, 7), centre_of(mapped, 8));
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
