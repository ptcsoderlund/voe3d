// The point shadow maps (ADR-0325): that a device made with point_shadow_size
// has them and one without does not, that either still draws, that the
// point-shadow pass draws a caster once or not at all, and that a lit surface
// reads them.
//
// WITH 64, READY. The device says voe_render_point_shadows_ready, then draws two
// frames, so each frame slot's set is bound once, of a lit cube in one window
// pass: each frame ends true with one draw command.
//
// THE PASS, ON THE READY DEVICE. One frame with two lights slotted 1 and 2 opens
// the point-shadow pass: a cube 2 m from the first adds one draw command, the
// same cube 100 m off adds none. A camera pass with the same lights follows, and
// then a third pass, past `passes`, is refused.
//
// THE LOOKUP, ON THE READY DEVICE (point 4). A grey ground quad seen straight
// down from 5 m (6.4 pixels a metre), a sun of nothing and no fill, a white lamp
// 1.5 m up at x = −1, range 6, slot 1, strength 1, and a cube 0.5 m from its
// middle to each face standing on the ground at x = 0 — big enough that its
// shadow covers x = +1 (one 0.5 m a side would end at x = 0.875). Each case is
// one frame: the point-shadow pass with the ground, and the cube or not, then
// the camera pass drawing the ground alone, read back:
// - the ground at x = +1 under half as bright with the cube as without; the
//   ground at x = −2 the same either way;
// - the lamp at x = +1: x = −1 darkened instead, x = +2 the same either way;
// - the camera's lamp at slot 0, or at strength 0, over a pass with the cube:
//   every pixel as with no cube.
// The no-cube picture matching those two is also what says the ground does not
// shadow itself: the lookup's one-texel push holds.
//
// WITH 0, NOT READY, AND THE SAME TWO FRAMES. One texel a side stays for the
// binding, so the frames are identical; the lamp slotted and not, with no pass,
// draw the same picture, the ground at x = +1 lit.
//
// A card without shaderOutputLayer makes the first claim false by design; the
// device says so on stderr. Reading a target back needs render's internal
// header, as point_lights.c does.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIDE 64
#define IMAGE_BYTES ((VkDeviceSize)SIDE * SIDE * 4)
#define CASES 6
#define POINT_SHADOW_SIDE 64

// VK_FORMAT_B8G8R8A8_SRGB, which the headless device takes; red is byte 2.
#define RED 2
// A driver's rounding on the way into an sRGB target.
#define TOLERANCE 3

// Columns of the ground at x = +1, −1, −2 and +2 m, on row 32 (z ≈ 0).
#define AT_PLUS_1 38
#define AT_MINUS_1 25
#define AT_MINUS_2 19
#define AT_PLUS_2 44
#define ROW (SIDE / 2)

#define EYE_Y 5.0f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 12,
	.indices = 42,
	.geometries = 2,
	.objects = 4,
	.shadings = 1,
	.passes = 2,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// A cube half a unit a side, inside an identity camera's clip volume.
static const voe_render_vertex VERTICES[8] = {
	{ { -0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.25f, -0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.25f, -0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { 0.25f, 0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	{ { -0.25f, 0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
};

static const uint32_t INDICES[36] = {
	0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
	3, 6, 2, 3, 7, 6, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5,
};

// A 20 m square at y = 0 facing up, counter-clockwise seen from above.
#define H 10.0f
static const voe_render_vertex GROUND_VERTICES[4] = {
	{ { -H, 0, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, 0, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, 0, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, 0, -H }, { 0, 1, 0 }, { 0, 0 } },
};

static const uint32_t GROUND_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

// What one device's cases draw with.
struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_geometry ground;
	voe_render_shading grey;
	struct voe_render_buffer readback;
	PFN_vkCmdCopyImageToBuffer copy_image_to_buffer;
};

// Two frames of the cube in one lit window pass, each ending with one draw.
static void draw_two_frames(voe_render_device *device, voe_render_geometry cube,
			    voe_render_shading grey)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = { .view = voe_math_float4x4_identity(),
			  .projection = voe_math_float4x4_identity() },
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};

	for (int frame = 0; frame < 2; frame++) {
		bool drawing = false;

		VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
		VOE_TEST_CHECK(drawing);
		if (!drawing)
			return;
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object));
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	}
}

// One frame: the point-shadow pass with a near and a far cube, a camera pass,
// and a third pass refused.
static void draw_point_shadow_pass(voe_render_device *device,
				   voe_render_geometry cube,
				   voe_render_shading grey)
{
	voe_platform_size size = { SIDE, SIDE };
	const voe_render_point_light lamps[2] = {
		{ .position = { 2.0f, 0.0f, 0.5f }, .range = 5.0f,
		  .colour = { 1.0f, 1.0f, 1.0f }, .falloff = 1.0f,
		  .shadow = 1, .shadow_strength = 1.0f },
		{ .position = { -2.0f, 0.0f, 0.5f }, .range = 5.0f,
		  .colour = { 1.0f, 1.0f, 1.0f }, .falloff = 1.0f,
		  .shadow = 2, .shadow_strength = 1.0f },
	};
	const voe_render_point_lights points = { lamps, 2 };
	voe_render_pass_camera camera = {
		.view = { .view = voe_math_float4x4_identity(),
			  .projection = voe_math_float4x4_identity() },
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
		.points = points,
	};
	voe_render_object near = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	voe_render_object far = near;
	bool drawing = false;
	uint32_t before;

	far.world = voe_math_float4x4_from_translation(
		(voe_math_float3){ 100.0f, 0.0f, 0.0f });
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_point_shadow_pass_begin(device, &points));
	before = voe_render_frame_draw_count(device);
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, near));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), before + 1);
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, far));
	VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), before + 1);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, near));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(!voe_render_point_shadow_pass_begin(device, &points));
	VOE_TEST_CHECK(voe_render_frame_end(device));
}

// Straight down from (0, EYE_Y, 0), world +X to the right and −Z up the
// picture, a 90° field of view, reverse-Z.
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

// The window's colour image copied into the readback buffer at `offset`, after
// the frame that drew it has finished.
static void read_back(struct scene *scene, const struct voe_render_frame *frame,
		      VkDeviceSize offset)
{
	voe_render_device *device = scene->device;
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
		.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				      .layerCount = 1 },
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
	scene->copy_image_to_buffer(commands, frame->target.colour.image,
				    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				    scene->readback.buffer, 1, &region);
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

// One frame: when `casting` is not NULL the point-shadow pass for it with the
// ground, and the cube when `cube`; then the camera pass of the ground lit by
// `lamp`, read back into image `image`.
static void shadow_case(struct scene *scene,
			const voe_render_point_light *casting, bool cube,
			voe_render_point_light lamp, int image)
{
	voe_render_device *device = scene->device;
	const struct voe_render_frame *frame = voe_render_frame_current(device);
	voe_platform_size size = { SIDE, SIDE };
	voe_render_object ground = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = scene->grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	// The half-unit cube scaled by 2 and moved onto the ground about x = 0.
	voe_render_object block = ground;
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .colour = { 1.0f, 1.0f, 1.0f } },
		.points = { &lamp, 1 },
	};
	bool drawing = false;

	block.world.m[0][0] = 2.0f;
	block.world.m[1][1] = 2.0f;
	block.world.m[2][2] = 2.0f;
	block.world.m[1][3] = 0.5f;
	block.world.m[2][3] = -1.0f;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	if (casting != NULL) {
		const voe_render_point_lights cast = { casting, 1 };

		VOE_TEST_CHECK(voe_render_point_shadow_pass_begin(device, &cast));
		VOE_TEST_CHECK(voe_render_frame_draw(device, scene->ground,
						     ground));
		if (cube)
			VOE_TEST_CHECK(voe_render_frame_draw(device, scene->cube,
							     block));
		voe_render_pass_end(device);
	}
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, scene->ground, ground));
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	voe_render_vk.device_wait_idle(device->device);
	read_back(scene, frame, IMAGE_BYTES * (VkDeviceSize)image);
}

// The red byte of image `image` at `column` on ROW.
static int red_at(const unsigned char *pixels, int image, int column)
{
	return pixels[IMAGE_BYTES * (VkDeviceSize)image +
		      ((size_t)ROW * SIDE + (size_t)column) * 4 + RED];
}

// Whether images `a` and `b` agree in every byte within TOLERANCE.
static bool same_picture(const unsigned char *pixels, int a, int b)
{
	for (VkDeviceSize i = 0; i < IMAGE_BYTES; i++) {
		if (abs(pixels[IMAGE_BYTES * (VkDeviceSize)a + i] -
			pixels[IMAGE_BYTES * (VkDeviceSize)b + i]) > TOLERANCE)
			return false;
	}
	return true;
}

// The lookup's cases on `scene`'s device, ready or not, and their checks.
static void check_lookup(struct scene *scene, bool ready)
{
	const voe_render_point_light left = {
		.position = { -1.0f, 1.5f, 0.0f }, .range = 6.0f,
		.colour = { 3.0f, 3.0f, 3.0f }, .falloff = 1.0f,
		.shadow = 1, .shadow_strength = 1.0f,
	};
	voe_render_point_light right = left;
	voe_render_point_light unslotted = left;
	voe_render_point_light faint = left;
	void *mapped = NULL;
	const unsigned char *p;

	right.position.x = 1.0f;
	unslotted.shadow = 0;
	faint.shadow_strength = 0.0f;
	if (ready) {
		shadow_case(scene, &left, false, left, 0);
		shadow_case(scene, &left, true, left, 1);
		shadow_case(scene, &right, false, right, 2);
		shadow_case(scene, &right, true, right, 3);
		shadow_case(scene, &left, true, unslotted, 4);
		shadow_case(scene, &left, true, faint, 5);
	} else {
		shadow_case(scene, NULL, false, left, 0);
		shadow_case(scene, NULL, false, unslotted, 1);
	}
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(scene->device->device,
						    scene->readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped == NULL)
		return;
	p = mapped;
	VOE_TEST_CHECK(red_at(p, 0, AT_PLUS_1) > 40);
	if (ready) {
		VOE_TEST_CHECK(red_at(p, 1, AT_PLUS_1) * 2 < red_at(p, 0, AT_PLUS_1));
		VOE_TEST_CHECK(abs(red_at(p, 1, AT_MINUS_2) -
				   red_at(p, 0, AT_MINUS_2)) <= TOLERANCE);
		VOE_TEST_CHECK(red_at(p, 2, AT_MINUS_1) > 40);
		VOE_TEST_CHECK(red_at(p, 3, AT_MINUS_1) * 2 < red_at(p, 2, AT_MINUS_1));
		VOE_TEST_CHECK(abs(red_at(p, 3, AT_PLUS_2) -
				   red_at(p, 2, AT_PLUS_2)) <= TOLERANCE);
		VOE_TEST_CHECK(same_picture(p, 4, 0));
		VOE_TEST_CHECK(same_picture(p, 5, 0));
	} else {
		VOE_TEST_CHECK(memcmp(p, p + IMAGE_BYTES, IMAGE_BYTES) == 0);
	}
	voe_render_vk.unmap_memory(scene->device->device, scene->readback.memory);
}

// The ground, the readback buffer and its copy command on `scene`'s device,
// false when any is missing.
static bool build_lookup(struct scene *scene)
{
	voe_base_error error = VOE_BASE_OK;

	scene->copy_image_to_buffer = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(scene->device->device,
						  "vkCmdCopyImageToBuffer");
	VOE_TEST_CHECK(scene->copy_image_to_buffer != NULL);
	VOE_TEST_CHECK(voe_render_geometry_create(scene->device, GROUND_VERTICES,
						  4, GROUND_INDICES, 6,
						  &scene->ground, &error));
	VOE_TEST_CHECK(voe_render_buffer_build(
		scene->device, &scene->readback, IMAGE_BYTES * CASES,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		"test readback"));
	return scene->copy_image_to_buffer != NULL &&
	       scene->readback.buffer != VK_NULL_HANDLE;
}

// A device of `side`, checked ready or not and drawn; false when this machine
// has no Vulkan to test.
static bool check_device(voe_base_arena *arena, uint32_t side, bool ready)
{
	voe_render_capacities room = CAPACITIES;
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

	room.point_shadow_size = side;
	scene.device = voe_render_device_new_headless(arena, size, room, &error);
	if (scene.device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				     error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		return false;
	}
	VOE_TEST_CHECK(scene.device != NULL);
	if (scene.device == NULL)
		return true;

	VOE_TEST_CHECK(voe_render_point_shadows_ready(scene.device) == ready);
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, GREY, &scene.grey,
						 &error));
	VOE_TEST_CHECK(voe_render_geometry_create(scene.device, VERTICES, 8,
						  INDICES, 36, &scene.cube,
						  &error));
	draw_two_frames(scene.device, scene.cube, scene.grey);
	if (ready)
		draw_point_shadow_pass(scene.device, scene.cube, scene.grey);
	if (build_lookup(&scene))
		check_lookup(&scene, ready);
	voe_render_vk.device_wait_idle(scene.device->device);
	voe_render_buffer_teardown(scene.device, &scene.readback);
	voe_render_device_destroy(scene.device);
	return true;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	if (check_device(arena, POINT_SHADOW_SIDE, true))
		check_device(arena, 0, false);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
