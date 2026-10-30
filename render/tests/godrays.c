// THE FOG IS IN SHADOW WHERE THE SUN IS BLOCKED (ADR-0493 point 6): beams are
// the first light in the fog, shadowed by its cascades and by the clouds, as
// strong as `godrays` says. A target of the test's own seen from the origin
// looking along −Z, no sky drawn, exposure 0 and Clip; a fog volume 40 m wide,
// 10 m tall and 20 m deep centred 30 m ahead, density 0.05, patchiness 0. One
// white light with a fill casts into cascade 0 through an orthographic view
// along it; an opaque slab drawn into that cascade alone, never into the
// picture, shades the volume's left half (x < 0). Patches are means over 8 × 8
// pixels, one left of the middle and one right, so the march's jitter does not
// decide a case.
//
// 1. the light straight above, godrays 1: the left patch darker than the
//    right by more than 4/255;
// 2. godrays 0: the two within 2/255, and each more than 8/255 off no fog;
// 3. godrays 0.5: the difference between those of 1 and 2;
// 4. the light low from behind the eye, the slab behind it shading the left
//    half again, godrays 1: the left darker by more than 4/255;
// 5. no slab, the light above, godrays 1, Volumetric clouds at coverage 1,
//    bottom 500 and thickness 500 read for their shadow and never drawn:
//    strength 1 darker than no clouds by more than 4/255 in both patches,
//    strength 0 within 1/255 of no clouds.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. The skip is a pass and it
// prints its reason.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SIDE 64
#define SHADOW_SIDE 256
#define PATCH 8
// The patches' first row and first columns: either side of the middle.
#define PATCH_ROW 28
#define LEFT_COLUMN 16
#define RIGHT_COLUMN 40
// Bytes out of 255.
#define BEAMS 4.0f
#define EVEN 2.0f
#define VISIBLE 8.0f
#define SAME 1.0f

#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
// The light's orthographic box: LIGHT_HALF metres either side of the volume's
// centre, its eye LIGHT_BACK metres back along the light, depth LIGHT_NEAR to
// LIGHT_FAR from there.
#define LIGHT_HALF 25.0f
#define LIGHT_BACK 100.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 200.0f

static const voe_math_float3 VOLUME_AT = { 0.0f, 0.0f, -30.0f };
// Light travelling straight down, and low from behind the eye towards −Z.
static const voe_math_float3 ABOVE = { 0.0f, -1.0f, 0.0f };
static const voe_math_float3 BEHIND = { 0.0f, -0.2873479f, -0.9578263f };
// The slab over the left half, 50 m up, and behind the eye across the low
// light's path to the left half.
static const voe_math_float3 SLAB_ABOVE_AT = { -15.0f, 50.0f, -30.0f };
static const voe_math_float3 SLAB_ABOVE_SCALE = { 30.0f, 1.0f, 50.0f };
static const voe_math_float3 SLAB_BEHIND_AT = { -15.0f, 15.0f, 20.0f };
static const voe_math_float3 SLAB_BEHIND_SCALE = { 30.0f, 30.0f, 1.0f };

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 2,
	.shadings = 1,
	.passes = 2,
	.targets = 1,
	.shadow_size = SHADOW_SIDE,
};

struct scene {
	voe_render_device *device;
	voe_base_arena *arena;
	voe_render_target target;
	voe_render_geometry cube;
	voe_render_shading grey;
};

// One picture's makings: the light's direction, the slab when `slab`, the
// volume's godrays, the clouds, and no fog at all when `fogless`.
struct setup {
	voe_math_float3 direction;
	bool slab;
	voe_math_float3 slab_at;
	voe_math_float3 slab_scale;
	float godrays;
	voe_render_clouds clouds;
	bool fogless;
};

// The eye at the origin looking along −Z, reverse-Z.
static voe_render_view looking_ahead(void)
{
	voe_render_view view = { .view = voe_math_float4x4_identity() };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The light's view along unit `direction`, which has no x, over the volume:
// x across, reverse-Z orthographic, depth one at LIGHT_NEAR and nought at
// LIGHT_FAR.
static voe_render_view light_view(voe_math_float3 direction)
{
	voe_math_float3 eye = {
		VOLUME_AT.x - direction.x * LIGHT_BACK,
		VOLUME_AT.y - direction.y * LIGHT_BACK,
		VOLUME_AT.z - direction.z * LIGHT_BACK,
	};
	voe_math_float3 x = { 1.0f, 0.0f, 0.0f };
	voe_math_float3 z = { -direction.x, -direction.y, -direction.z };
	voe_math_float3 rows[3] = { x, voe_math_float3_cross(z, x), z };
	voe_render_view light = { .eye = eye };
	float span = LIGHT_FAR - LIGHT_NEAR;

	assert(direction.x == 0.0f);
	for (int r = 0; r < 3; r++) {
		light.view.m[r][0] = rows[r].x;
		light.view.m[r][1] = rows[r].y;
		light.view.m[r][2] = rows[r].z;
		light.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	light.view.m[3][3] = 1.0f;
	light.projection.m[0][0] = 1.0f / LIGHT_HALF;
	light.projection.m[1][1] = 1.0f / LIGHT_HALF;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = LIGHT_FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

// The volume of the header, unturned, at `godrays`; `count` 0 is no fog.
static voe_render_fog fog_of(uint32_t count, float godrays)
{
	voe_render_fog fog = { .count = count };

	fog.volumes[0] = (voe_render_fog_volume){
		.rows = {
			{ 1.0f / 20.0f, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f / 5.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f / 10.0f, -VOLUME_AT.z / 10.0f },
		},
		.patches = { 1.0f, 1.0f, 1.0f },
		.density = 0.05f,
		.change = { 1.0f, 1.0f, 1.0f },
		.patchiness = 0.0f,
		.godrays = godrays,
	};
	return fog;
}

static voe_render_object placed(voe_render_shading shading, voe_math_float3 at,
				voe_math_float3 scale)
{
	return (voe_render_object){
		.world = voe_math_float4x4_mul(
			voe_math_float4x4_from_translation(at),
			voe_math_float4x4_from_scale(scale)),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// One frame of `setup`: the slab into cascade 0, then the fog into the
// target, read back; NULL when the read failed.
static const uint8_t *picture_of(struct scene *scene, struct setup setup)
{
	voe_platform_size size = { SIDE, SIDE };
	const voe_render_view light = light_view(setup.direction);
	voe_render_pass_camera camera = {
		.view = looking_ahead(),
		.light = { .direction = setup.direction,
			   .intensity = 3.0f,
			   .colour = { 1.0f, 1.0f, 1.0f },
			   .fill = { 0.1f, 0.1f, 0.1f } },
		.shadow = { .splits = { FAR_PLANE },
			    .texels = { 2.0f * LIGHT_HALF / SHADOW_SIDE },
			    .count = 1 },
		.exposure = { .tone = VOE_RENDER_TONE_CLIP },
		.clouds = setup.clouds,
		.fog = fog_of(setup.fogless ? 0 : 1, setup.godrays),
	};
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	camera.shadow.cascades[0] =
		voe_math_float4x4_mul(light.projection, light.view);
	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return NULL;
	VOE_TEST_CHECK(voe_render_shadow_pass_begin(scene->device, 0, &light));
	if (setup.slab)
		VOE_TEST_CHECK(voe_render_frame_draw(
			scene->device, scene->cube,
			placed(scene->grey, setup.slab_at, setup.slab_scale)));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device, scene->target,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw_fog(scene->device));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	VOE_TEST_CHECK(voe_render_target_read(scene->device, scene->target,
					      scene->arena, &picture, &error));
	if (picture.pixels == NULL || picture.width != SIDE ||
	    picture.height != SIDE)
		return NULL;
	return picture.pixels;
}

// The mean of the colour channels over the PATCH × PATCH pixels from
// PATCH_ROW and `column`, in bytes.
static float patch_mean(const uint8_t *pixels, int column)
{
	float sum = 0.0f;

	assert(column >= 0 && column + PATCH <= SIDE);
	for (int y = PATCH_ROW; y < PATCH_ROW + PATCH; y++)
		for (int x = column; x < column + PATCH; x++)
			for (int c = 0; c < 3; c++)
				sum += pixels[((size_t)y * SIDE + (size_t)x) * 4 + c];
	return sum / (float)(PATCH * PATCH * 3);
}

// How much darker the left patch is than the right, in bytes.
static float left_darker(const char *name, const uint8_t *pixels)
{
	const float left = patch_mean(pixels, LEFT_COLUMN);
	const float right = patch_mean(pixels, RIGHT_COLUMN);

	printf("%s: left %.2f, right %.2f\n", name, left, right);
	return right - left;
}

static const voe_render_clouds NO_CLOUDS = { .kind = VOE_RENDER_CLOUDS_NONE };

static voe_render_clouds clouds_of(float strength)
{
	return (voe_render_clouds){
		.kind = VOE_RENDER_CLOUDS_VOLUMETRIC,
		.coverage = 1.0f,
		.bottom = 500.0f,
		.thickness = 500.0f,
		.origin = { 100.0f, 100.0f },
		.shadow_strength = strength,
	};
}

// Cases 1 to 4: the slab's shadow in the fog, by godrays.
static void check_slab(struct scene *scene)
{
	struct setup above = {
		.direction = ABOVE,
		.slab = true,
		.slab_at = SLAB_ABOVE_AT,
		.slab_scale = SLAB_ABOVE_SCALE,
		.clouds = NO_CLOUDS,
	};
	struct setup behind = above;
	struct setup fogless = above;

	fogless.fogless = true;
	behind.direction = BEHIND;
	behind.slab_at = SLAB_BEHIND_AT;
	behind.slab_scale = SLAB_BEHIND_SCALE;
	behind.godrays = 1.0f;
	const uint8_t *none = picture_of(scene, fogless);
	const uint8_t *even = picture_of(scene, above);
	above.godrays = 0.5f;
	const uint8_t *half = picture_of(scene, above);
	above.godrays = 1.0f;
	const uint8_t *full = picture_of(scene, above);
	const uint8_t *low = picture_of(scene, behind);

	VOE_TEST_CHECK(none != NULL && even != NULL && half != NULL &&
		       full != NULL && low != NULL);
	if (none == NULL || even == NULL || half == NULL || full == NULL ||
	    low == NULL)
		return;
	const float beams = left_darker("godrays 1", full);
	const float flat = left_darker("godrays 0", even);
	const float between = left_darker("godrays 0.5", half);

	VOE_TEST_CHECK(beams > BEAMS);
	VOE_TEST_CHECK(fabsf(flat) <= EVEN);
	printf("no fog: left %.2f, right %.2f\n", patch_mean(none, LEFT_COLUMN),
	       patch_mean(none, RIGHT_COLUMN));
	VOE_TEST_CHECK(patch_mean(even, LEFT_COLUMN) -
			       patch_mean(none, LEFT_COLUMN) >
		       VISIBLE);
	VOE_TEST_CHECK(patch_mean(even, RIGHT_COLUMN) -
			       patch_mean(none, RIGHT_COLUMN) >
		       VISIBLE);
	VOE_TEST_CHECK(flat < between && between < beams);
	VOE_TEST_CHECK(left_darker("behind the eye, godrays 1", low) > BEAMS);
}

// Case 5: the clouds' shadow in the fog.
static void check_clouds(struct scene *scene)
{
	struct setup clear = {
		.direction = ABOVE,
		.godrays = 1.0f,
		.clouds = NO_CLOUDS,
	};
	struct setup shadowed = clear;
	struct setup unshadowed = clear;

	shadowed.clouds = clouds_of(1.0f);
	unshadowed.clouds = clouds_of(0.0f);
	const uint8_t *plain = picture_of(scene, clear);
	const uint8_t *dark = picture_of(scene, shadowed);
	const uint8_t *zero = picture_of(scene, unshadowed);

	VOE_TEST_CHECK(plain != NULL && dark != NULL && zero != NULL);
	if (plain == NULL || dark == NULL || zero == NULL)
		return;
	const int columns[2] = { LEFT_COLUMN, RIGHT_COLUMN };

	for (int i = 0; i < 2; i++) {
		const float without = patch_mean(plain, columns[i]);

		printf("clouds, patch %d: strength 1 %.2f, 0 %.2f, none %.2f\n",
		       i, patch_mean(dark, columns[i]),
		       patch_mean(zero, columns[i]), without);
		VOE_TEST_CHECK(without - patch_mean(dark, columns[i]) > BEAMS);
		VOE_TEST_CHECK(fabsf(patch_mean(zero, columns[i]) - without) <=
			       SAME);
	}
}

// A unit cube about the origin: only its depth is drawn, so its normals and
// its winding's way round do not matter.
static bool build_scene(struct scene *scene)
{
	static const voe_render_vertex vertices[8] = {
		{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
	};
	static const uint32_t indices[36] = {
		0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
		3, 6, 2, 3, 7, 6, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5,
	};
	const voe_render_shading_values grey = {
		.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_texture shown = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool built = true;

	built &= voe_render_target_create(scene->device, SIDE, SIDE,
					  &scene->target, &shown, &error);
	built &= voe_render_geometry_create(scene->device, vertices, 8, indices,
					    36, &scene->cube, &error);
	built &= voe_render_shading_create(scene->device, grey, &scene->grey,
					   &error);
	return built;
}

int main(void)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { .arena = voe_base_arena_new(1024 * 1024) };

	scene.device = voe_render_device_new_headless(scene.arena, size,
						      CAPACITIES, &error);
	if (scene.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(scene.device != NULL);
		voe_base_arena_destroy(scene.arena);
		return voe_test_result();
	}
	if (build_scene(&scene)) {
		check_slab(&scene);
		check_clouds(&scene);
	} else {
		VOE_TEST_CHECK(false);
	}

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
