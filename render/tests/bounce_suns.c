// Every sun lighting the probes (ADR-0357 points 1 and 4): the relight's level 1
// summing the first sun and each further sun of the begin's `more`, each by its
// own layer of the sun map. Headless; read through ../src/device_internal.h for
// the window volume's sum images.
//
// A FRAME, as bounce_probes_scene.c's: begin, voe_render_bounce_begin about the
// origin (lowest cell (−12, −6, −12), corner (−24, −12, −24), spacing 2),
// capture passes drawing the scene until one does not open, then one bounce
// shadow pass per sun of the begin (a view down that sun at the volume's
// centre, VOLUME_HALF either side) with the scene drawn into it when it opens,
// relight. No camera pass: what is checked is the volume, not a picture. A
// scene is settled by such frames until one opens no capture pass; a new scene
// is queued whole by a stale sphere.
//
// THE READBACK. Once idle, the sum grid's three images whole (48 × 12 × 24
// RGBA16F each); probe (0, 0, 0), toroidally the cell at world (1, 1, 1), and
// its irradiance the RGB of its six axes summed.
//
// Grey ground (0.5); a sun along (0.6, −0.8, 0) and a blue moon along (−0.6,
// −0.8, 0), each intensity 2, bounce strength 1.
// 1. The sun alone, bouncing once: the reference.
// 2. The same with an empty `more`, after a relight by another: the same bytes.
// 3. A first sun of intensity 0 and the sun as more[0]: within 1% of 1.
// 4. The sun and the moon both bouncing once: more than the sun alone.
// 5. The moon at bounces 0: the sun alone's bytes.
// 6. A grey wall 0.5 × 40 × 100 m across x = 3: from the probe it hides all
//    ground past it, and the moon's shadow covers all ground before it within
//    the probe's 24 m reach. The moon bouncing, its map drawn on sun 1: within
//    1% of the sun alone in that scene.
//
// A card without shaderOutputLayer bounces nothing, and that is said. A machine
// with no usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIDE 16
#define FRAMES_MAX 200
#define VOLUME_HALF 60.0f
#define LIGHT_DISTANCE 60.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 120.0f
// One sum image: 48 × 12 × 24 texels of four halves.
#define IMAGE_BYTES ((VkDeviceSize)2 * VOE_RENDER_BOUNCE_PROBES_TOTAL * 8)
#define READBACK_BYTES (3 * IMAGE_BYTES)

static const voe_render_capacities CAPACITIES = {
	.vertices = 24,
	.indices = 36,
	.geometries = 1,
	.objects = 128,
	.shadings = 1,
	.passes = 8,
};

static const voe_math_float3 SUN_DIRECTION = { 0.6f, -0.8f, 0.0f };
static const voe_math_float3 MOON_DIRECTION = { -0.6f, -0.8f, 0.0f };

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

struct box {
	voe_math_float3 at;
	voe_math_float3 size;
};

#define GROUND { { 0.0f, -0.05f, 0.0f }, { 100.0f, 0.1f, 100.0f } }

static const struct box OPEN_SCENE[] = { GROUND };
static const struct box WALL_SCENE[] = {
	GROUND,
	{ { 3.0f, 20.0f, 0.0f }, { 0.5f, 40.0f, 100.0f } },
};

struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_shading grey;
	const struct box *boxes;
	uint32_t box_count;
	struct voe_render_bounce_frame bounce;
	voe_render_directional_light more[1];
	bool restale;
};

static const voe_math_float4 EVERYTHING = { 0.0f, 0.0f, 0.0f, 1000.0f };

// A view with rows x, y, z and the eye at `eye`.
static voe_render_view view_from(voe_math_float3 eye, voe_math_float3 z)
{
	const voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x = voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_math_float3 rows[3] = { x, voe_math_float3_cross(z, x), z };
	voe_render_view view = { .eye = eye };

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	view.view.m[3][3] = 1.0f;
	return view;
}

// The view down `direction` at the volume's centre, the origin: from
// LIGHT_DISTANCE back along it, orthographic over VOLUME_HALF, reverse-Z.
static voe_render_view sun_view(voe_math_float3 direction)
{
	voe_render_view light = view_from(
		voe_math_float3_scale(direction, -LIGHT_DISTANCE),
		voe_math_float3_scale(direction, -1.0f));
	const float span = LIGHT_FAR - LIGHT_NEAR;

	light.projection.m[0][0] = 1.0f / VOLUME_HALF;
	light.projection.m[1][1] = 1.0f / VOLUME_HALF;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = LIGHT_FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

static void draw_boxes(struct scene *s)
{
	for (uint32_t i = 0; i < s->box_count; i++) {
		const struct box *b = &s->boxes[i];
		voe_render_object object = {
			.world = voe_math_float4x4_mul(
				voe_math_float4x4_from_translation(b->at),
				voe_math_float4x4_from_scale(b->size)),
			.normal = voe_math_float4x4_from_scale((voe_math_float3){
				1.0f / b->size.x, 1.0f / b->size.y,
				1.0f / b->size.z }),
			.shading = s->grey.index,
			.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		};

		VOE_TEST_CHECK(voe_render_frame_draw(s->device, s->cube, object));
	}
}

// Each sun's bounce shadow pass, the scene drawn into each that opens.
static void shadow_passes(struct scene *s)
{
	const struct voe_render_bounce_frame *b = &s->bounce;

	for (uint32_t sun = 0; sun < 1 + b->more.count; sun++) {
		const voe_render_view light = sun_view(
			sun == 0 ? b->sun.direction :
				   b->more.lights[sun - 1].light.direction);
		bool opened = false;

		VOE_TEST_CHECK(voe_render_bounce_shadow_pass_begin(
			s->device, sun, &light, &opened));
		if (!opened)
			continue;
		draw_boxes(s);
		voe_render_pass_end(s->device);
	}
}

// One frame of the header's; the capture passes it opened.
static uint32_t one_frame(struct scene *s)
{
	struct voe_render_bounce_frame bounce = s->bounce;
	bool drawing = false;
	bool opened = true;
	uint32_t passes = 0;

	VOE_TEST_CHECK(voe_render_frame_begin(s->device,
					      (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	if (s->restale && s->device->window_volume.built) {
		bounce.stale = &EVERYTHING;
		bounce.stale_count = 1;
		s->restale = false;
	}
	voe_render_bounce_begin(s->device, VOE_RENDER_TARGET_WINDOW, &bounce);
	while (opened) {
		VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(s->device,
								    &opened));
		if (!opened)
			break;
		draw_boxes(s);
		voe_render_pass_end(s->device);
		passes++;
	}
	shadow_passes(s);
	voe_render_bounce_relight(s->device);
	VOE_TEST_CHECK(voe_render_frame_end(s->device));
	return passes;
}

// `boxes` in place and every probe captured: frames until one opens no
// capture pass, past the first, which may only want the volume.
static void settle(struct scene *s, const struct box *boxes, uint32_t count)
{
	uint32_t frame = 0;

	s->boxes = boxes;
	s->box_count = count;
	s->restale = true;
	for (; frame < FRAMES_MAX; frame++)
		if (one_frame(s) == 0 && frame > 0)
			break;
	printf("settled in %u frames\n", frame);
	VOE_TEST_CHECK(frame < FRAMES_MAX);
}

// The three sum images into `readback`, once the card is idle.
static void read_sum(voe_render_device *device,
		     const struct voe_render_buffer *readback)
{
	PFN_vkCmdCopyImageToBuffer copy = (PFN_vkCmdCopyImageToBuffer)
		voe_render_vk.get_device_proc_addr(device->device,
						  "vkCmdCopyImageToBuffer");
	VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = device->pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commands = VK_NULL_HANDLE;
	const VkCommandBufferBeginInfo begin = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};
	const VkMemoryBarrier2 barriers[2] = {
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
			.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
			.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
			.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
			.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT,
		},
	};
	VkDependencyInfo before = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.memoryBarrierCount = 1,
		.pMemoryBarriers = &barriers[0],
	};
	VkDependencyInfo after = before;
	VkCommandBufferSubmitInfo submit_commands = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
	};
	const VkSubmitInfo2 submit = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &submit_commands,
	};

	after.pMemoryBarriers = &barriers[1];
	VOE_TEST_CHECK(copy != NULL);
	if (copy == NULL)
		return;
	voe_render_vk.device_wait_idle(device->device);
	VOE_TEST_CHECK_INT(voe_render_vk.allocate_command_buffers(device->device,
								  &allocate,
								  &commands),
			   VK_SUCCESS);
	if (commands == VK_NULL_HANDLE)
		return;
	voe_render_vk.begin_command_buffer(commands, &begin);
	voe_render_vk.cmd_pipeline_barrier2(commands, &before);
	for (uint32_t a = 0; a < 3; a++) {
		const VkBufferImageCopy region = {
			.bufferOffset = a * IMAGE_BYTES,
			.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					      .layerCount = 1 },
			.imageExtent = { 2 * VOE_RENDER_BOUNCE_PROBES_XZ,
					 VOE_RENDER_BOUNCE_PROBES_Y,
					 VOE_RENDER_BOUNCE_PROBES_XZ },
		};

		copy(commands, device->window_volume.irradiance[6][a].image,
		     VK_IMAGE_LAYOUT_GENERAL, readback->buffer, 1, &region);
	}
	voe_render_vk.cmd_pipeline_barrier2(commands, &after);
	voe_render_vk.end_command_buffer(commands);
	submit_commands.commandBuffer = commands;
	VOE_TEST_CHECK_INT(voe_render_vk.queue_submit2(device->queue, 1, &submit,
						       VK_NULL_HANDLE),
			   VK_SUCCESS);
	voe_render_vk.device_wait_idle(device->device);
	voe_render_vk.free_command_buffers(device->device, device->pool, 1,
					   &commands);
}

// An IEEE half as a float.
static float half_to_float(uint16_t h)
{
	const int exponent = (h >> 10) & 0x1f;
	const float mantissa = (float)(h & 0x3ff);
	const float value = exponent == 0 ?
				    ldexpf(mantissa, -24) :
				    ldexpf(1024.0f + mantissa, exponent - 25);

	return (h & 0x8000) != 0 ? -value : value;
}

// The sum grid after one frame with the scene's lights as they now are, into
// `halves` (READBACK_BYTES); probe (0, 0, 0)'s irradiance, -1 on a failure.
static float relit(struct scene *s, uint16_t *halves)
{
	struct voe_render_buffer readback = { 0 };
	void *mapped = NULL;
	float sum = 0.0f;

	VOE_TEST_CHECK_INT(one_frame(s), 0);
	memset(halves, 0, READBACK_BYTES);
	VOE_TEST_CHECK(voe_render_buffer_build(
		s->device, &readback, READBACK_BYTES,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	if (readback.buffer == VK_NULL_HANDLE)
		return -1.0f;
	read_sum(s->device, &readback);
	VOE_TEST_CHECK_INT(voe_render_vk.map_memory(s->device->device,
						    readback.memory, 0,
						    VK_WHOLE_SIZE, 0, &mapped),
			   VK_SUCCESS);
	if (mapped == NULL) {
		voe_render_buffer_teardown(s->device, &readback);
		return -1.0f;
	}
	memcpy(halves, mapped, READBACK_BYTES);
	voe_render_vk.unmap_memory(s->device->device, readback.memory);
	voe_render_buffer_teardown(s->device, &readback);
	// Texels (0, 0, 0), E(+a), and (1, 0, 0), E(−a), of each axis image.
	for (uint32_t a = 0; a < 3; a++)
		for (uint32_t i = 0; i < 2 * 4; i++)
			if (i % 4 != 3)
				sum += half_to_float(halves[a * IMAGE_BYTES / 2 + i]);
	return sum;
}

static voe_render_directional_light moon(uint32_t bounces)
{
	return (voe_render_directional_light){
		.light = { .direction = MOON_DIRECTION,
			   .intensity = 2.0f,
			   .colour = { 0.5f, 0.5f, 1.0f } },
		.bounces = bounces,
		.bounce_strength = 1.0f,
	};
}

static bool within(float a, float b, float share)
{
	return fabsf(a - b) <= share * fabsf(b);
}

static void every_sun(struct scene *s, uint16_t *reference, uint16_t *other)
{
	const voe_render_light sun = s->bounce.sun;
	float alone, empty, copied, both, still, walled, hidden;

	settle(s, OPEN_SCENE, 1);
	alone = relit(s, reference);
	s->more[0] = moon(1);
	s->bounce.more = (voe_render_directional_lights){ s->more, 1 };
	both = relit(s, other);
	s->bounce.more.count = 0;
	empty = relit(s, other);
	VOE_TEST_CHECK(memcmp(reference, other, READBACK_BYTES) == 0);

	s->more[0] = (voe_render_directional_light){
		.light = sun, .bounces = 1, .bounce_strength = 1.0f };
	s->bounce.more.count = 1;
	s->bounce.sun.intensity = 0.0f;
	copied = relit(s, other);
	s->bounce.sun = sun;

	s->more[0] = moon(1);
	(void)relit(s, other);
	s->more[0] = moon(0);
	still = relit(s, other);
	VOE_TEST_CHECK(memcmp(reference, other, READBACK_BYTES) == 0);
	printf("probe: sun %.4f, empty more %.4f, dark sun and its copy %.4f, sun and moon %.4f, moon at bounces 0 %.4f\n",
	       (double)alone, (double)empty, (double)copied, (double)both,
	       (double)still);
	VOE_TEST_CHECK(alone > 0.0f);
	VOE_TEST_CHECK(within(copied, alone, 0.01f));
	VOE_TEST_CHECK(both > alone);

	s->bounce.more.count = 0;
	settle(s, WALL_SCENE, 2);
	walled = relit(s, reference);
	s->more[0] = moon(1);
	s->bounce.more.count = 1;
	hidden = relit(s, other);
	VOE_TEST_CHECK(s->device->frames[s->device->slot == 0 ?
						 VOE_RENDER_FRAMES_IN_FLIGHT - 1 :
						 s->device->slot - 1]
			       .bounce_shadow.drawn[1]);
	printf("behind the wall: sun %.4f, sun and a shadowed moon %.4f\n",
	       (double)walled, (double)hidden);
	VOE_TEST_CHECK(walled > 0.0f);
	VOE_TEST_CHECK(within(hidden, walled, 0.01f));
}

static void run(struct scene *s)
{
	static const voe_render_shading_values grey = {
		.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_base_error error = VOE_BASE_OK;
	uint16_t *reference = malloc(READBACK_BYTES);
	uint16_t *other = malloc(READBACK_BYTES);

	VOE_TEST_CHECK(reference != NULL && other != NULL);
	VOE_TEST_CHECK(voe_render_shading_create(s->device, grey, &s->grey,
						 &error));
	VOE_TEST_CHECK(voe_render_geometry_create(s->device, CUBE_VERTICES, 24,
						  CUBE_INDICES, 36, &s->cube,
						  &error));
	s->bounce = (struct voe_render_bounce_frame){
		.cell = { -12, -6, -12 },
		.corner = { -24.0f, -12.0f, -24.0f },
		.spacing = VOE_RENDER_BOUNCE_SPACING,
		.sun = { .direction = SUN_DIRECTION,
			 .intensity = 2.0f,
			 .colour = { 1.0f, 1.0f, 1.0f } },
		.sun_bounces = 1,
		.sun_strength = 1.0f,
	};
	if (reference != NULL && other != NULL)
		every_sun(s, reference, other);
	free(reference);
	free(other);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4 * 1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { 0 };

	scene.device = voe_render_device_new_headless(
		arena, (voe_platform_size){ SIDE, SIDE }, CAPACITIES, &error);
	if (scene.device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				     error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		voe_base_arena_destroy(arena);
		return 0;
	}
	VOE_TEST_CHECK(scene.device != NULL);
	if (scene.device != NULL) {
		if (scene.device->output_layer)
			run(&scene);
		else
			printf("note: no shaderOutputLayer, so nothing bounces\n");
		voe_render_device_destroy(scene.device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
