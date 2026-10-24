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
// 5. the same with the sun pointing away: red still, emission unshadowed;
// 6. unlit, a base colour texture black on its left half and white on its
//    right, `uv_repeat` 4: the centre row crosses eight bands, four white;
// 7. the same with `uv_repeat` 0, read as 1: two bands, one white;
// 8. case 1 with an occlusion texel of red 0: case 1 again, the sun's direct
//    light undarkened (ADR-0400);
// 9. the sun pointing away and a fill turned up, occlusion red 255: the fill;
// 10. the same with occlusion red 0: darker in every channel, the fill occluded;
// 11. a white base, roughness 1, metallic 0, under the sun: the matte reference;
// 12. roughness 0.1 and a metal-roughness texel green 255 blue 0: case 11
//     again, the shiny factor not read (ADR-0401);
// 13. roughness 1, metallic 1: the metal reference;
// 14. metallic 0 and a texel green 255 blue 255: case 13 again;
// 15. two quads side by side, the right one's u running −x (a mirrored
//     half), flat-mapped under a sun low from +x: the reference per half;
// 16. the same wearing the +u tilt: the left half brighter than 15's, the
//     right darker, the bump turned over with its picture as Blender's is;
// 17, 18. the same along v: the right quad's v running up, a map tilted to
//     +green and a sun low from +y, left brighter than flat, right darker;
// 19. case 2 with the flat map's blue 0, as a BC5 map samples it: the
//     reference, z made from x and y (ADR-0477 point 1).
// Checked once when written: roughness 0.1 with no map reads 255 where case 11
// reads 151, and case 13 reads 80, so 12 and 14 would fail under a multiply.
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
#define CASES 19

// VK_FORMAT_B8G8R8A8_SRGB, which the headless device takes.
#define BLUE 0
#define GREEN 1
#define RED 2

// How far a channel may be from the expected byte: a driver's rounding on the
// way into an sRGB target.
#define TOLERANCE 3

static const voe_render_capacities CAPACITIES = {
	.vertices = 20,
	.indices = 30,
	.geometries = 3,
	.objects = 1,
	.shadings = 14,
	.passes = 1,
};

#define EYE_Z 3.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// Counter-clockwise from +Z; u runs along +x and v down, as glTF's does.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1, 1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { 1, 1, 0 }, { 0, 0, 1 }, { 1, 0 }, { 0 } },
	{ { 1, -1, 0 }, { 0, 0, 1 }, { 1, 1 }, { 0 } },
	{ { -1, -1, 0 }, { 0, 0, 1 }, { 0, 1 }, { 0 } },
};

static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

// Two quads side by side, x −1..0 and 0..1, each wound as QUAD_VERTICES. The
// left is as QUAD_VERTICES; the right's u runs along −x.
static const voe_render_vertex MIRRORED_U_VERTICES[8] = {
	{ { -1, 1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0 }, { 0 } },
	{ { 0, -1, 0 }, { 0, 0, 1 }, { 1, 1 }, { 0 } },
	{ { -1, -1, 0 }, { 0, 0, 1 }, { 0, 1 }, { 0 } },
	{ { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0 }, { 0 } },
	{ { 1, 1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { 1, -1, 0 }, { 0, 0, 1 }, { 0, 1 }, { 0 } },
	{ { 0, -1, 0 }, { 0, 0, 1 }, { 1, 1 }, { 0 } },
};

// The same, the right quad's u along +x again and its v running up.
static const voe_render_vertex MIRRORED_V_VERTICES[8] = {
	{ { -1, 1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0 }, { 0 } },
	{ { 0, -1, 0 }, { 0, 0, 1 }, { 1, 1 }, { 0 } },
	{ { -1, -1, 0 }, { 0, 0, 1 }, { 0, 1 }, { 0 } },
	{ { 0, 1, 0 }, { 0, 0, 1 }, { 0, 1 }, { 0 } },
	{ { 1, 1, 0 }, { 0, 0, 1 }, { 1, 1 }, { 0 } },
	{ { 1, -1, 0 }, { 0, 0, 1 }, { 1, 0 }, { 0 } },
	{ { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
};

static const uint32_t PAIR_INDICES[12] = { 3, 2, 1, 3, 1, 0, 7, 6, 5, 7, 5, 4 };

// The columns read in the pair's left and right quads, on the centre row:
// world x of about ∓0.55 on the plane, well inside each.
#define LEFT_COLUMN (SIDE / 2 - 10)
#define RIGHT_COLUMN (SIDE / 2 + 10)

// Flat, sixty degrees towards +u: (sin 60°, 0, cos 60°) as 0..255, and the
// same towards glTF's +green, which is −v. Flat again with no blue.
static const uint8_t FLAT_TEXEL[4] = { 128, 128, 255, 255 };
static const uint8_t FLAT_NO_BLUE_TEXEL[4] = { 128, 128, 0, 255 };
static const uint8_t TILTED_TEXEL[4] = { 238, 128, 191, 255 };
static const uint8_t TILTED_GREEN_TEXEL[4] = { 128, 238, 191, 255 };

// Occlusion in red: wholly occluded, and not at all.
static const uint8_t OCCLUDED_TEXEL[4] = { 0, 0, 0, 255 };
static const uint8_t OPEN_TEXEL[4] = { 255, 255, 255, 255 };

// Metal-roughness in green and blue: fully rough, not metal; and fully metal.
static const uint8_t ROUGH_TEXEL[4] = { 0, 255, 0, 255 };
static const uint8_t METAL_TEXEL[4] = { 0, 255, 255, 255 };

// Eight texels across, four black then four white, as rgba.
#define HALVES_WIDTH 8
static const uint8_t HALVES[HALVES_WIDTH * 4] = {
	0,   0,   0,   255, 0,   0,   0,   255,
	0,   0,   0,   255, 0,   0,   0,   255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
};

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

static const unsigned char *on_centre_row(const unsigned char *pixels,
					  int image, int column)
{
	return pixels + IMAGE_BYTES * (VkDeviceSize)image +
	       ((size_t)(SIDE / 2) * SIDE + (size_t)column) * 4;
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

// How many white runs the centre row of an image holds. The cleared
// background is near black, so it only ever joins a black band.
static int white_runs(const unsigned char *pixels, int image)
{
	const unsigned char *row = pixels + IMAGE_BYTES * (VkDeviceSize)image +
				   (size_t)(SIDE / 2) * SIDE * 4;
	int runs = 0;
	bool white = false;

	for (int x = 0; x < SIDE; x++) {
		bool now = row[x * 4 + GREEN] > 128;
		if (now && !white)
			runs++;
		white = now;
	}
	return runs;
}

static void check_red(const unsigned char *pixel)
{
	VOE_TEST_CHECK(pixel[RED] >= 255 - TOLERANCE);
	VOE_TEST_CHECK(pixel[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(pixel[BLUE] <= TOLERANCE);
}

// A one-pixel DATA texture of `texel`.
static voe_render_texture data_pixel(voe_render_device *device,
				     const uint8_t texel[4])
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_texture texture = { 0 };

	VOE_TEST_CHECK(voe_render_texture_create(device, VOE_RENDER_TEXTURE_DATA,
						 VOE_RENDER_SAMPLING_SMOOTH, 1,
						 1, texel, &texture, &error));
	return texture;
}

// The fourteen shading records: plain, flat map, tilted map, black and
// glowing, the halves texture repeated four times and with a repeat of 0,
// plain wholly occluded and not occluded at all, white matte, white with a
// rough map, white metal and white with a metal map, plain tilted to green,
// and plain with a flat map of no blue.
static void make_shadings(voe_render_device *device, voe_render_shading *out)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_texture flat = data_pixel(device, FLAT_TEXEL);
	voe_render_texture tilted = data_pixel(device, TILTED_TEXEL);
	voe_render_texture halves = { 0 };
	voe_render_shading_values repeated = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.unlit = 1,
		.uv_repeat = 4.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_shading_values plain = {
		.base_colour = { 0.6f, 0.5f, 0.4f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_shading_values white = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
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
	VOE_TEST_CHECK(voe_render_texture_create(device,
						 VOE_RENDER_TEXTURE_COLOUR,
						 VOE_RENDER_SAMPLING_SMOOTH,
						 HALVES_WIDTH, 1, HALVES, &halves,
						 &error));
	repeated.base_colour_texture = halves.index;
	VOE_TEST_CHECK(voe_render_shading_create(device, repeated, &out[4],
						 &error));
	repeated.uv_repeat = 0.0f;
	VOE_TEST_CHECK(voe_render_shading_create(device, repeated, &out[5],
						 &error));
	plain.normal_texture = VOE_RENDER_NO_TEXTURE;
	plain.occlusion_texture = data_pixel(device, OCCLUDED_TEXEL).index;
	VOE_TEST_CHECK(voe_render_shading_create(device, plain, &out[6],
						 &error));
	plain.occlusion_texture = data_pixel(device, OPEN_TEXEL).index;
	VOE_TEST_CHECK(voe_render_shading_create(device, plain, &out[7],
						 &error));
	VOE_TEST_CHECK(voe_render_shading_create(device, white, &out[8],
						 &error));
	white.roughness = 0.1f;
	white.metallic_roughness_texture = data_pixel(device, ROUGH_TEXEL).index;
	VOE_TEST_CHECK(voe_render_shading_create(device, white, &out[9],
						 &error));
	white.roughness = 1.0f;
	white.metallic = 1.0f;
	white.metallic_roughness_texture = VOE_RENDER_NO_TEXTURE;
	VOE_TEST_CHECK(voe_render_shading_create(device, white, &out[10],
						 &error));
	white.metallic = 0.0f;
	white.metallic_roughness_texture = data_pixel(device, METAL_TEXEL).index;
	VOE_TEST_CHECK(voe_render_shading_create(device, white, &out[11],
						 &error));
	plain.occlusion_texture = VOE_RENDER_NO_TEXTURE;
	plain.normal_texture = data_pixel(device, TILTED_GREEN_TEXEL).index;
	VOE_TEST_CHECK(voe_render_shading_create(device, plain, &out[12],
						 &error));
	plain.normal_texture = data_pixel(device, FLAT_NO_BLUE_TEXEL).index;
	VOE_TEST_CHECK(voe_render_shading_create(device, plain, &out[13],
						 &error));
}

// Cases 15 to 18 into images 14 to 17: each pair flat and then tilted, under
// a sun low from +x for u and from +y for v, both travelling mostly sideways.
static void draw_mirrored(voe_render_device *device,
			  const voe_render_shading *shadings, VkBuffer buffer)
{
	voe_base_error error = VOE_BASE_OK;
	voe_render_geometry along_u = { 0 };
	voe_render_geometry along_v = { 0 };
	voe_render_light low = {
		.direction = { -0.96f, 0.0f, -0.28f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};

	VOE_TEST_CHECK(voe_render_geometry_create(device, MIRRORED_U_VERTICES,
						  8, PAIR_INDICES, 12,
						  &along_u, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(device, MIRRORED_V_VERTICES,
						  8, PAIR_INDICES, 12,
						  &along_v, &error));
	draw_case(device, along_u, shadings[1], low, buffer, IMAGE_BYTES * 14);
	draw_case(device, along_u, shadings[2], low, buffer, IMAGE_BYTES * 15);
	low.direction = (voe_math_float3){ 0.0f, -0.96f, -0.28f };
	draw_case(device, along_v, shadings[1], low, buffer, IMAGE_BYTES * 16);
	draw_case(device, along_v, shadings[12], low, buffer, IMAGE_BYTES * 17);
}

// The tilted pair at `image` against the flat one before it: the unmirrored
// left quad turned towards the sun, the mirrored right one away from it.
static void check_mirrored(const unsigned char *pixels, int image)
{
	check_darker(on_centre_row(pixels, image - 1, LEFT_COLUMN),
		     on_centre_row(pixels, image, LEFT_COLUMN));
	check_darker(on_centre_row(pixels, image, RIGHT_COLUMN),
		     on_centre_row(pixels, image - 1, RIGHT_COLUMN));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_device *device;
	struct voe_render_buffer readback = { 0 };
	voe_render_geometry quad = { 0 };
	voe_render_shading shadings[14] = { 0 };
	// Travelling down −Z into the face: N·L of one.
	voe_render_light sun = {
		.direction = { 0.0f, 0.0f, -1.0f },
		.intensity = 1.0f,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
	voe_render_light away = sun;
	voe_render_light filled;
	void *mapped = NULL;

	away.direction = (voe_math_float3){ 0.0f, 0.0f, 1.0f };
	filled = away;
	filled.fill = (voe_math_float3){ 0.5f, 0.5f, 0.5f };
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
	make_shadings(device, shadings);

	draw_case(device, quad, shadings[0], sun, readback.buffer, 0);
	draw_case(device, quad, shadings[1], sun, readback.buffer, IMAGE_BYTES);
	draw_case(device, quad, shadings[2], sun, readback.buffer,
		  IMAGE_BYTES * 2);
	draw_case(device, quad, shadings[3], sun, readback.buffer,
		  IMAGE_BYTES * 3);
	draw_case(device, quad, shadings[3], away, readback.buffer,
		  IMAGE_BYTES * 4);
	draw_case(device, quad, shadings[4], sun, readback.buffer,
		  IMAGE_BYTES * 5);
	draw_case(device, quad, shadings[5], sun, readback.buffer,
		  IMAGE_BYTES * 6);
	draw_case(device, quad, shadings[6], sun, readback.buffer,
		  IMAGE_BYTES * 7);
	draw_case(device, quad, shadings[7], filled, readback.buffer,
		  IMAGE_BYTES * 8);
	draw_case(device, quad, shadings[6], filled, readback.buffer,
		  IMAGE_BYTES * 9);
	for (int i = 8; i < 12; i++)
		draw_case(device, quad, shadings[i], sun, readback.buffer,
			  IMAGE_BYTES * (VkDeviceSize)(i + 2));
	draw_mirrored(device, shadings, readback.buffer);
	draw_case(device, quad, shadings[13], sun, readback.buffer,
		  IMAGE_BYTES * 18);

	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped != NULL) {
		check_same(centre_of(mapped, 1), centre_of(mapped, 0));
		check_darker(centre_of(mapped, 2), centre_of(mapped, 0));
		check_red(centre_of(mapped, 3));
		check_red(centre_of(mapped, 4));
		VOE_TEST_CHECK_INT(white_runs(mapped, 5), 4);
		VOE_TEST_CHECK_INT(white_runs(mapped, 6), 1);
		check_same(centre_of(mapped, 7), centre_of(mapped, 0));
		check_darker(centre_of(mapped, 9), centre_of(mapped, 8));
		check_same(centre_of(mapped, 11), centre_of(mapped, 10));
		check_same(centre_of(mapped, 13), centre_of(mapped, 12));
		check_mirrored(mapped, 15);
		check_mirrored(mapped, 17);
		check_same(centre_of(mapped, 18), centre_of(mapped, 0));
		voe_render_vk.unmap_memory(device->device, readback.memory);
	}

	voe_render_vk.device_wait_idle(device->device);
	voe_render_buffer_teardown(device, &readback);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
