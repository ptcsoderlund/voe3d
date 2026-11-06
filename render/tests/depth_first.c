// A PASS DRAWS WHOLE, OR LAYS DEPTH THEN SHADES (ADR-0508 point 2). Unlit quads
// facing the camera, each frame read back, the depth step checked six ways:
//
// 1. laid_then_shaded_is_the_whole_picture: cutout_far.c's one-texel alpha
//    stripes (cutoff 0.5) on a red quad at z = −2 and a green one at z = −1
//    shifted a pixel, two-sided: drawn whole, and both laid then both shaded,
//    the same bytes, red and green each on at least an eighth of the picture;
// 2. a_dissolving_pair_covers_each_pixel_once: dither.c's red at 0.5 then green
//    at −0.5, laid then shaded: every pixel red or green, the same bytes as whole;
// 3. a_dither_below_the_floor_lays_nothing: red at dither 0.01, laid then shaded:
//    every pixel the clear colour;
// 4. lay_alone_draws_no_colour: a red quad laid alone leaves the clear colour;
//    a small one laid, then a green one drawn whole behind it, leaves the clear
//    colour where it was laid and green round it;
// 5. instanced_copies_lay_then_shade: instances.c's three white copies laid then
//    shaded, the same bytes as whole, two draws and one draw's six triangles;
// 6. each_pass_opens_whole: a pass ending in lay, then a green quad drawn in the
//    next pass with no step chosen: every pixel green.
//
// The camera is cutout_far.c's: eye at the origin looking down −Z, 90°, so at
// z = −1 the picture spans −1..1 and a pixel is 1/32. A stripe quad of half-side
// 8 at z = −1, or 16 at z = −2, shows two pixels a texel, level 0.
//
// A frame is a list of acts — draws, step and side choices, a second pass — so
// each case is data. It runs headless and reads the window target back, as
// cutout_far.c does, and includes render's internal header for the same
// reason. A machine with no usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

#define SIDE 64
#define PIXELS (SIDE * SIDE)
#define IMAGE_BYTES ((VkDeviceSize)PIXELS * 4)
#define TEXELS 256
#define COPIES 3

#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define SCALE 0.2f
// One pixel at z = −1, and half a stripe texel there.
#define PIXEL (1.0f / 32.0f)

// The readback's image slots, one per frame drawn.
enum shot { STRIPES_WHOLE, STRIPES_LAID, PAIR_WHOLE, PAIR_LAID, FLOOR_LAID,
	    LAID_ALONE, LAID_IN_FRONT, COPIES_WHOLE, COPIES_LAID, SECOND_PASS,
	    SHOTS };

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 4,
	.shadings = 2,
	.passes = 2,
	.instances = COPIES,
};

// A unit quad facing +Z, wound counter-clockwise from the camera, (0,0) at its
// top-left.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
	{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0 } },
	{ { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0 } },
	{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0 } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

static const voe_math_float4 RED = { 1.0f, 0.0f, 0.0f, 1.0f };
static const voe_math_float4 GREEN = { 0.0f, 1.0f, 0.0f, 1.0f };
static const voe_math_float4 WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };

// Where each copy stands along x at z = −1, as instances.c's.
static const float PLACES[COPIES] = { -0.6f, 0.0f, 0.6f };

static PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;

// What a frame records, in order, inside its first pass.
enum act_kind { ACT_DRAW, ACT_COPIES, ACT_STEP, ACT_TWO_SIDED, ACT_NEXT_PASS };

struct act {
	enum act_kind kind;
	voe_render_object object;
	voe_render_depth_step step;
};

// The device and what every case draws with, and the mapped readback.
struct scene {
	voe_render_device *device;
	voe_render_geometry quad;
	voe_render_shading stripes;
	voe_render_shading plain;
	VkBuffer buffer;
	const uint8_t *pixels;
};

// The stripes: every texel white, alpha 255 on even columns and 0 on odd.
static uint8_t stripe_pixels[TEXELS * TEXELS * 4];

static const uint8_t *stripes(void)
{
	for (uint32_t y = 0; y < TEXELS; y++) {
		for (uint32_t x = 0; x < TEXELS; x++) {
			uint8_t *p = stripe_pixels + ((size_t)y * TEXELS + x) * 4;

			memset(p, 255, 3);
			p[3] = (x & 1) ? 0 : 255;
		}
	}
	return stripe_pixels;
}

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

// The quad of half-side `half` facing the camera at (x, 0, z), wearing
// `shading` in `colour` and keeping `dither` of its pixels.
static voe_render_object quad_at(voe_render_shading shading, float half,
				 float x, float z, voe_math_float4 colour,
				 float dither)
{
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = colour,
		.dither = dither,
	};

	object.world.m[0][0] = half;
	object.world.m[1][1] = half;
	object.world.m[0][3] = x;
	object.world.m[2][3] = z;
	return object;
}

static struct act draw(voe_render_object object)
{
	return (struct act){ .kind = ACT_DRAW, .object = object };
}

static struct act copies(voe_render_object object)
{
	return (struct act){ .kind = ACT_COPIES, .object = object };
}

static struct act step(voe_render_depth_step depth_step)
{
	return (struct act){ .kind = ACT_STEP, .step = depth_step };
}

// Copy i: the quad scaled by SCALE at (PLACES[i], 0, −1).
static voe_render_instance copy_at(int i)
{
	voe_render_instance copy = { 0 };

	copy.rows[0] = (voe_math_float4){ SCALE, 0.0f, 0.0f, PLACES[i] };
	copy.rows[1] = (voe_math_float4){ 0.0f, SCALE, 0.0f, 0.0f };
	copy.rows[2] = (voe_math_float4){ 0.0f, 0.0f, SCALE, -1.0f };
	return copy;
}

// One act recorded into the open pass.
static void record(const struct scene *scene, const struct act *act,
		   uint32_t first)
{
	voe_render_pass_camera camera = { .view = the_camera() };

	switch (act->kind) {
	case ACT_DRAW:
		VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->quad,
						     act->object));
		break;
	case ACT_COPIES:
		VOE_TEST_CHECK(voe_render_frame_draw_instanced(scene->device,
							       scene->quad,
							       act->object,
							       first, COPIES));
		break;
	case ACT_STEP:
		voe_render_frame_depth_step(scene->device, act->step);
		break;
	case ACT_TWO_SIDED:
		voe_render_frame_two_sided(scene->device, true);
		break;
	case ACT_NEXT_PASS:
		voe_render_pass_end(scene->device);
		VOE_TEST_CHECK(voe_render_pass_begin(scene->device,
						     VOE_RENDER_TARGET_WINDOW,
						     &camera));
		break;
	}
}

// One frame of `count` acts in a camera pass, the three copies written first,
// the picture read back into image slot `shot`; returns that picture.
static const uint8_t *draw_case(const struct scene *scene,
				const struct act *acts, uint32_t count,
				enum shot shot)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = { .view = the_camera() };
	voe_render_instance placed[COPIES];
	uint32_t first = 0;
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
		.bufferOffset = IMAGE_BYTES * shot,
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
		placed[i] = copy_at(i);

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return scene->pixels + IMAGE_BYTES * shot;
	VOE_TEST_CHECK(voe_render_frame_instances(device, placed, COPIES, &first));
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	for (uint32_t i = 0; i < count; i++)
		record(scene, &acts[i], first);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	voe_render_vk.device_wait_idle(device->device);

	VOE_TEST_CHECK_INT(voe_render_vk.allocate_command_buffers(device->device,
								  &allocate,
								  &commands),
			   VK_SUCCESS);
	if (commands == VK_NULL_HANDLE)
		return scene->pixels + IMAGE_BYTES * shot;
	voe_render_vk.begin_command_buffer(commands, &begin);
	copy_image_to_buffer(commands, frame->target.colour.image,
			     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, scene->buffer,
			     1, &region);
	voe_render_vk.cmd_pipeline_barrier2(commands, &dependency);
	voe_render_vk.end_command_buffer(commands);
	submit_commands.commandBuffer = commands;
	VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1, &submit,
						       VK_NULL_HANDLE),
			   VK_SUCCESS);
	voe_render_vk.device_wait_idle(device->device);
	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
	return scene->pixels + IMAGE_BYTES * shot;
}

// What a pixel is read as: red, green, or the clear colour, every channel under
// 64, near black.
enum hue { RED_PIXEL, GREEN_PIXEL, CLEAR_PIXEL };

// The format is B8G8R8A8: blue, green, red.
static bool is(const uint8_t *p, enum hue hue)
{
	switch (hue) {
	case RED_PIXEL:
		return p[2] >= 200 && p[1] < 64 && p[0] < 64;
	case GREEN_PIXEL:
		return p[1] >= 200 && p[2] < 64 && p[0] < 64;
	case CLEAR_PIXEL:
		return p[0] < 64 && p[1] < 64 && p[2] < 64;
	}
	return false;
}

// How many pixels of `image` in rows and columns `first` .. `last` − 1 are `hue`.
static uint32_t counted(const uint8_t *image, uint32_t first, uint32_t last,
			enum hue hue)
{
	uint32_t count = 0;

	for (uint32_t y = first; y < last; y++)
		for (uint32_t x = first; x < last; x++)
			if (is(image + ((size_t)y * SIDE + x) * 4, hue))
				count++;
	return count;
}

static bool same(const uint8_t *a, const uint8_t *b)
{
	return memcmp(a, b, IMAGE_BYTES) == 0;
}

static void laid_then_shaded_is_the_whole_picture(const struct scene *scene)
{
	const voe_render_object far =
		quad_at(scene->stripes, 16.0f, 0.0f, -2.0f, RED, 0.0f);
	const voe_render_object near =
		quad_at(scene->stripes, 8.0f, PIXEL, -1.0f, GREEN, 0.0f);
	const struct act whole[] = { { .kind = ACT_TWO_SIDED }, draw(far),
				     draw(near) };
	const struct act laid[] = {
		{ .kind = ACT_TWO_SIDED }, step(VOE_RENDER_DEPTH_STEP_LAY),
		draw(far), draw(near), step(VOE_RENDER_DEPTH_STEP_SHADE),
		draw(far), draw(near), step(VOE_RENDER_DEPTH_STEP_WHOLE),
	};
	const uint8_t *a = draw_case(scene, whole, 3, STRIPES_WHOLE);
	const uint8_t *b = draw_case(scene, laid, 8, STRIPES_LAID);

	printf("stripes: red %u, green %u of %u\n", counted(b, 0, SIDE, RED_PIXEL),
	       counted(b, 0, SIDE, GREEN_PIXEL), PIXELS);
	VOE_TEST_CHECK(same(a, b));
	VOE_TEST_CHECK(counted(b, 0, SIDE, RED_PIXEL) >= PIXELS / 8);
	VOE_TEST_CHECK(counted(b, 0, SIDE, GREEN_PIXEL) >= PIXELS / 8);
}

static void a_dissolving_pair_covers_each_pixel_once(const struct scene *scene)
{
	const voe_render_object out =
		quad_at(scene->plain, 2.0f, 0.0f, -1.0f, RED, 0.5f);
	const voe_render_object in =
		quad_at(scene->plain, 2.0f, 0.0f, -1.0f, GREEN, -0.5f);
	const struct act whole[] = { draw(out), draw(in) };
	const struct act laid[] = {
		step(VOE_RENDER_DEPTH_STEP_LAY), draw(out), draw(in),
		step(VOE_RENDER_DEPTH_STEP_SHADE), draw(out), draw(in),
	};
	const uint8_t *a = draw_case(scene, whole, 2, PAIR_WHOLE);
	const uint8_t *b = draw_case(scene, laid, 6, PAIR_LAID);
	const uint32_t reds = counted(b, 0, SIDE, RED_PIXEL);
	const uint32_t greens = counted(b, 0, SIDE, GREEN_PIXEL);

	printf("pair: red %u, green %u of %u\n", reds, greens, PIXELS);
	VOE_TEST_CHECK(same(a, b));
	VOE_TEST_CHECK_INT(reds + greens, PIXELS);
	VOE_TEST_CHECK(reds > 0 && greens > 0);
}

static void a_dither_below_the_floor_lays_nothing(const struct scene *scene)
{
	const voe_render_object faint =
		quad_at(scene->plain, 2.0f, 0.0f, -1.0f, RED, 0.01f);
	const struct act laid[] = { step(VOE_RENDER_DEPTH_STEP_LAY), draw(faint),
				    step(VOE_RENDER_DEPTH_STEP_SHADE),
				    draw(faint) };
	const uint8_t *b = draw_case(scene, laid, 4, FLOOR_LAID);

	VOE_TEST_CHECK_INT(counted(b, 0, SIDE, CLEAR_PIXEL), PIXELS);
}

static void lay_alone_draws_no_colour(const struct scene *scene)
{
	const voe_render_object filling =
		quad_at(scene->plain, 2.0f, 0.0f, -1.0f, RED, 0.0f);
	// The middle quarter, pixels 16 .. 47, and a green quad behind filling.
	const voe_render_object small =
		quad_at(scene->plain, 0.5f, 0.0f, -1.0f, RED, 0.0f);
	const voe_render_object behind =
		quad_at(scene->plain, 4.0f, 0.0f, -2.0f, GREEN, 0.0f);
	const struct act alone[] = { step(VOE_RENDER_DEPTH_STEP_LAY),
				     draw(filling) };
	const struct act in_front[] = { step(VOE_RENDER_DEPTH_STEP_LAY),
					draw(small),
					step(VOE_RENDER_DEPTH_STEP_WHOLE),
					draw(behind) };
	const uint8_t *a = draw_case(scene, alone, 2, LAID_ALONE);
	const uint8_t *b = draw_case(scene, in_front, 4, LAID_IN_FRONT);

	VOE_TEST_CHECK_INT(counted(a, 0, SIDE, CLEAR_PIXEL), PIXELS);
	VOE_TEST_CHECK_INT(counted(b, 16, 48, CLEAR_PIXEL), 32 * 32);
	VOE_TEST_CHECK_INT(counted(b, 0, SIDE, GREEN_PIXEL), PIXELS - 32 * 32);
}

static void instanced_copies_lay_then_shade(const struct scene *scene)
{
	const voe_render_object white =
		quad_at(scene->plain, 2.0f, 0.0f, -1.0f, WHITE, 0.0f);
	const struct act whole[] = { copies(white) };
	const struct act laid[] = { step(VOE_RENDER_DEPTH_STEP_LAY),
				    copies(white),
				    step(VOE_RENDER_DEPTH_STEP_SHADE),
				    copies(white) };
	const uint8_t *a = draw_case(scene, whole, 1, COPIES_WHOLE);
	const uint8_t *b;

	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(scene->device), 1);
	b = draw_case(scene, laid, 4, COPIES_LAID);
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(scene->device), 2);
	VOE_TEST_CHECK_INT(voe_render_frame_triangle_count(scene->device),
			   2 * COPIES);
	VOE_TEST_CHECK(same(a, b));
	VOE_TEST_CHECK(counted(b, 0, SIDE, CLEAR_PIXEL) < PIXELS);
}

static void each_pass_opens_whole(const struct scene *scene)
{
	const voe_render_object far =
		quad_at(scene->plain, 4.0f, 0.0f, -2.0f, RED, 0.0f);
	const voe_render_object filling =
		quad_at(scene->plain, 2.0f, 0.0f, -1.0f, GREEN, 0.0f);
	const struct act acts[] = { step(VOE_RENDER_DEPTH_STEP_LAY), draw(far),
				    { .kind = ACT_NEXT_PASS }, draw(filling) };
	const uint8_t *b = draw_case(scene, acts, 4, SECOND_PASS);

	VOE_TEST_CHECK_INT(counted(b, 0, SIDE, GREEN_PIXEL), PIXELS);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	struct scene scene = { 0 };
	struct voe_render_buffer readback = { 0 };
	voe_render_texture texture = { 0 };
	voe_render_shading_values values = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
		.unlit = 1,
	};
	void *mapped = NULL;

	scene.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						      &error);
	if (scene.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED) {
			printf("skip: %s\n", voe_base_error_string(error));
			voe_base_arena_destroy(arena);
			return voe_test_result();
		}
		VOE_TEST_CHECK(scene.device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(scene.device->device,
						  "vkCmdCopyImageToBuffer");
	VOE_TEST_CHECK(copy_image_to_buffer != NULL);
	VOE_TEST_CHECK(voe_render_buffer_build(
		scene.device, &readback, IMAGE_BYTES * SHOTS,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		"test readback"));
	VOE_TEST_CHECK(voe_render_geometry_create(scene.device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &scene.quad,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, values,
						 &scene.plain, &error));
	VOE_TEST_CHECK(voe_render_texture_create(
		scene.device, VOE_RENDER_TEXTURE_COLOUR,
		VOE_RENDER_SAMPLING_SMOOTH, TEXELS, TEXELS, stripes(), &texture,
		&error));
	values.alpha_mode = VOE_RENDER_ALPHA_CUTOUT;
	values.alpha_cutoff = 0.5f;
	values.base_colour_texture = texture.index;
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, values,
						 &scene.stripes, &error));

	if (copy_image_to_buffer != NULL && readback.buffer != VK_NULL_HANDLE) {
		VOE_TEST_CHECK_INT(voe_render_vk.map_memory(scene.device->device,
							    readback.memory, 0,
							    VK_WHOLE_SIZE, 0,
							    &mapped),
				   VK_SUCCESS);
	}
	if (mapped != NULL) {
		scene.buffer = readback.buffer;
		scene.pixels = mapped;
		laid_then_shaded_is_the_whole_picture(&scene);
		a_dissolving_pair_covers_each_pixel_once(&scene);
		a_dither_below_the_floor_lays_nothing(&scene);
		lay_alone_draws_no_colour(&scene);
		instanced_copies_lay_then_shade(&scene);
		each_pass_opens_whole(&scene);
		voe_render_vk.unmap_memory(scene.device->device,
					   readback.memory);
	}

	voe_render_vk.device_wait_idle(scene.device->device);
	voe_render_buffer_teardown(scene.device, &readback);
	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
