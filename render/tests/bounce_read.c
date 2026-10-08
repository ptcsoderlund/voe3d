// Lit surfaces read the probe volume (ADR-0326 point 7), read through
// ../src/device_internal.h. Headless.
//
// THE SCENE: a grey ground (a 40 m flat box, base 0.5) under a sun at N·L 0.8,
// no cascades, fill 0.1, seen from above; the window's volume begun about the
// origin, lowest cell (−12, −6, −12), corner (−24, −12, −24).
//
// NOTHING CAPTURED IS NO BOUNCE. The first frame's begin wants the volume and
// the next frame's top builds it, cleared: every probe invalid. A frame begun
// onto that built volume names it in its camera pass's block (entry 0 of
// binding 6) and reads the same pixels as a frame with no begin, which names
// none: with no valid probe the read's weights sum to nought and E is nought,
// so the fill alone lifts the ground and the gain is gone.
//
// A card without shaderOutputLayer builds no volume and names none; the two
// pictures still match, and that is said. A machine with no usable Vulkan skips
// and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SIDE 64
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define FILL 0.1f

static const voe_render_capacities CAPACITIES = {
	.vertices = 24,
	.indices = 36,
	.geometries = 1,
	.objects = 2,
	.shadings = 1,
	.passes = 2,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

static const voe_render_light SUN = {
	.direction = { -0.6f, -0.8f, 0.0f },
	.intensity = 1.0f,
	.colour = { 1.0f, 1.0f, 1.0f },
	.fill = { FILL, FILL, FILL },
};

static const struct voe_render_bounce_frame BOUNCE = {
	.cell = { -12, -6, -12 },
	.corner = { -24.0f, -12.0f, -24.0f },
	.sun = SUN,
	.sun_bounces = 1,
	.sun_strength = 1.0f,
	.spacing = VOE_RENDER_BOUNCE_SPACING,
};

// Counter-clockwise from outside, four vertices a face, a unit cube.
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

struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_pass_camera camera;
};

// The camera at (0, 6, 8) looking at the origin, reverse-Z perspective.
static voe_render_view scene_camera(void)
{
	const voe_math_float3 eye = { 0.0f, 6.0f, 8.0f };
	voe_math_float3 z = voe_math_float3_normalize(eye);
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x =
		voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_math_float3 rows[3] = { x, voe_math_float3_cross(z, x), z };
	voe_render_view view = { .eye = eye };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The ground: 40 m square, its top at y = 0.
static voe_render_object ground(voe_render_shading shading)
{
	const voe_math_float3 scale = { 40.0f, 0.1f, 40.0f };

	return (voe_render_object){
		.world = voe_math_float4x4_mul(
			voe_math_float4x4_from_translation(
				(voe_math_float3){ 0.0f, -0.05f, 0.0f }),
			voe_math_float4x4_from_scale(scale)),
		.normal = voe_math_float4x4_from_scale((voe_math_float3){
			1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z }),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// One frame, begun onto the window when `begin`, its camera pass drawing the
// ground; `*named` is the pass's bounce record's grid. The picture, or a
// zeroed one when `read` is false.
static voe_render_picture draw_frame(struct scene *scene, bool begin, bool read,
				     uint32_t *named, voe_base_arena *arena)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const struct voe_render_frame *frame;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device,
					      (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	if (begin)
		voe_render_bounce_begin(scene->device, VOE_RENDER_TARGET_WINDOW,
					&BOUNCE);
	VOE_TEST_CHECK(voe_render_pass_begin(
		scene->device, VOE_RENDER_TARGET_WINDOW, &scene->camera));
	VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->cube,
					     ground(scene->grey)));
	voe_render_pass_end(scene->device);
	frame = voe_render_frame_current(scene->device);
	*named = ((const struct voe_render_frame_block *)frame->uniforms_mapped)
			 ->bounce.grid;
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	if (read)
		VOE_TEST_CHECK(voe_render_target_read(scene->device,
						      VOE_RENDER_TARGET_WINDOW,
						      arena, &picture, &error));
	return picture;
}

static void nothing_captured_is_no_bounce(struct scene *scene,
					  voe_base_arena *arena)
{
	const bool layered = scene->device->output_layer;
	const size_t bytes = (size_t)SIDE * SIDE * 4;
	uint32_t named = 0;
	voe_render_picture begun;
	voe_render_picture plain;

	if (!layered)
		printf("note: no shaderOutputLayer, so no volume is built\n");
	draw_frame(scene, true, false, &named, arena);
	VOE_TEST_CHECK(named == VOE_RENDER_NO_BOUNCE);
	begun = draw_frame(scene, true, true, &named, arena);
	VOE_TEST_CHECK(scene->device->window_volume[0].built == layered);
	VOE_TEST_CHECK(named == (layered ? 0u : VOE_RENDER_NO_BOUNCE));
	plain = draw_frame(scene, false, true, &named, arena);
	VOE_TEST_CHECK(named == VOE_RENDER_NO_BOUNCE);

	VOE_TEST_CHECK(begun.pixels != NULL && plain.pixels != NULL);
	if (begun.pixels == NULL || plain.pixels == NULL)
		return;
	{
		const uint8_t *middle =
			&begun.pixels[((SIDE / 2) * SIDE + SIDE / 2) * 4];

		printf("middle: %d %d %d\n", middle[0], middle[1], middle[2]);
		VOE_TEST_CHECK(middle[0] > 0 && middle[0] == middle[1]);
	}
	VOE_TEST_CHECK(memcmp(begun.pixels, plain.pixels, bytes) == 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
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
		VOE_TEST_CHECK(voe_render_shading_create(scene.device, GREY,
							 &scene.grey, &error));
		VOE_TEST_CHECK(voe_render_geometry_create(
			scene.device, CUBE_VERTICES, 24, CUBE_INDICES, 36,
			&scene.cube, &error));
		scene.camera = (voe_render_pass_camera){
			.view = scene_camera(),
			.light = SUN,
		};
		nothing_captured_is_no_bounce(&scene, arena);
		voe_render_device_destroy(scene.device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
