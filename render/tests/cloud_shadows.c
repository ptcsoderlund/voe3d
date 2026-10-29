// CLOUDS SHADOW WHAT THE LIGHTS REACH (ADR-0492 point 7). A grey floor seen
// straight down from 10 m, one white light from above travelling (−0.6, −0.8,
// 0) with a fill of 0.1, no cascades; the clouds Volumetric, coverage 1,
// bottom 500, thickness 500, read for their shadow and never drawn. Six claims:
//
// 1. shadow strength 1: the floor's centre more than 8/255 darker than with no
//    clouds;
// 2. strength 0: the same bytes as no clouds;
// 3. strength 0.5: the centre between the two, every channel;
// 4. a floor tilted to face away from the light, N·L below 0, lit by its fill
//    alone: every pixel within 1/255 with clouds at strength 1 and without;
// 5. clouds whose bottom is 200 m below the floor, 100 m thick: the same bytes
//    as no clouds;
// 6. a water plane, deep with no depth copy, under strength 1: its centre
//    darker than without.
//
// Built as directional_lights.c builds its device, reading the window back
// through voe_render_target_read as RGBA8.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. The skip is a pass and it
// prints its reason.
#include <render/device.h>

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
#define PIXELS (SIDE * SIDE * 4)
#define CHANGED 8
#define SAME 1

#define EYE_Y 10.0f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define GREY 0.5f
#define WATER_SIDE 20.0f
#define DEEP 1.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 12,
	.indices = 18,
	.geometries = 3,
	.objects = 2,
	.shadings = 2,
	.passes = 1,
};

// A 20 m square at y = 0 facing up, counter-clockwise seen from above.
#define G 10.0f
static const voe_render_vertex FLOOR_VERTICES[4] = {
	{ { -G, 0, G }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { G, 0, G }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { G, 0, -G }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { -G, 0, -G }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
};

// The floor sheared to y = √3·x over 4 m by 6 m, facing (−√3/2, 1/2, 0): up
// enough to be seen from above, away from a light towards (0.6, 0.8, 0).
#define T 3.4641016f
#define TILT_NORMAL { -0.8660254f, 0.5f, 0.0f }
static const voe_render_vertex TILTED_VERTICES[4] = {
	{ { -2.0f, -T, 3.0f }, TILT_NORMAL, { 0, 0 }, { 0 } },
	{ { 2.0f, T, 3.0f }, TILT_NORMAL, { 0, 0 }, { 0 } },
	{ { 2.0f, T, -3.0f }, TILT_NORMAL, { 0, 0 }, { 0 } },
	{ { -2.0f, -T, -3.0f }, TILT_NORMAL, { 0, 0 }, { 0 } },
};

static const uint32_t SQUARE_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

// A unit quad in model XY facing +z, as the model store's quad is: water.c's.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -0.5f, 0.5f, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { 0.5f, 0.5f, 0 }, { 0, 0, 1 }, { 1, 0 }, { 0 } },
	{ { 0.5f, -0.5f, 0 }, { 0, 0, 1 }, { 1, 1 }, { 0 } },
	{ { -0.5f, -0.5f, 0 }, { 0, 0, 1 }, { 0, 1 }, { 0 } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

// What a frame draws.
enum surface { FLOOR, TILTED, WATER };

struct scene {
	voe_render_device *device;
	voe_render_geometry floor;
	voe_render_geometry tilted;
	voe_render_geometry quad;
	voe_render_shading grey;
	voe_render_shading water;
	voe_base_arena *arena;
};

// Straight down from (0, EYE_Y, 0), world +X to the right and −Z up the
// picture, a 90° field of view, reverse-Z: directional_lights.c's camera.
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

// Volumetric clouds at coverage 1 casting `strength`, `bottom` metres above
// the eye and `thickness` deep.
static voe_render_clouds clouds_of(float strength, float bottom, float thickness)
{
	return (voe_render_clouds){
		.kind = VOE_RENDER_CLOUDS_VOLUMETRIC,
		.coverage = 1.0f,
		.bottom = bottom,
		.thickness = thickness,
		.origin = { 100.0f, 100.0f },
		.shadow_strength = strength,
	};
}

// The water quad turned from XY to XZ facing up, flat, centred on the origin:
// water.c's `lying`, with no waves.
static voe_render_object water_object(voe_render_shading shading)
{
	voe_render_object object = {
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.waves = { 0.0f, 2.0f, 0.0f, DEEP },
	};

	object.world.m[0][0] = WATER_SIDE;
	object.world.m[1][2] = 1.0f;
	object.world.m[2][1] = -WATER_SIDE;
	object.world.m[3][3] = 1.0f;
	object.normal.m[0][0] = 1.0f / WATER_SIDE;
	object.normal.m[1][2] = 1.0f;
	object.normal.m[2][1] = -1.0f / WATER_SIDE;
	object.normal.m[3][3] = 1.0f;
	return object;
}

// `surface` drawn into the open pass.
static void draw_surface(struct scene *scene, enum surface surface)
{
	voe_render_object grey = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = scene->grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};

	if (surface == WATER)
		VOE_TEST_CHECK(voe_render_frame_draw_blended(
			scene->device, scene->quad, water_object(scene->water)));
	else
		VOE_TEST_CHECK(voe_render_frame_draw(
			scene->device,
			surface == FLOOR ? scene->floor : scene->tilted, grey));
}

// One frame of `surface` under `clouds`, read back; pixels NULL when it failed.
static voe_render_picture draw_case(struct scene *scene, enum surface surface,
				    voe_render_clouds clouds)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = { .direction = { -0.6f, -0.8f, 0.0f },
			   .intensity = 3.0f,
			   .colour = { 1.0f, 1.0f, 1.0f },
			   .fill = { 0.1f, 0.1f, 0.1f } },
		.clouds = clouds,
	};
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device,
					     VOE_RENDER_TARGET_WINDOW, &camera));
	draw_surface(scene, surface);
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	VOE_TEST_CHECK(voe_render_target_read(scene->device,
					      VOE_RENDER_TARGET_WINDOW,
					      scene->arena, &picture, &error));
	if (picture.width != SIDE || picture.height != SIDE)
		picture.pixels = NULL;
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

static const uint8_t *centre(const voe_render_picture *picture)
{
	return picture->pixels + ((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4;
}

// The largest colour channel difference between `a` and `b` over `count`
// bytes.
static int largest_difference(const uint8_t *a, const uint8_t *b, int count)
{
	int largest = 0;

	for (int i = 0; i < count; i++)
		if ((i & 3) != 3 && abs(a[i] - b[i]) > largest)
			largest = abs(a[i] - b[i]);
	return largest;
}

// `dark`'s centre no channel brighter than `bright`'s and one more than
// CHANGED darker.
static void check_darker(const char *name, const voe_render_picture *dark,
			 const voe_render_picture *bright)
{
	const uint8_t *d = centre(dark);
	const uint8_t *b = centre(bright);

	printf("%s: centre %d %d %d, without %d %d %d\n", name, d[0], d[1], d[2],
	       b[0], b[1], b[2]);
	for (int c = 0; c < 3; c++)
		VOE_TEST_CHECK(d[c] <= b[c]);
	VOE_TEST_CHECK(largest_difference(d, b, 4) > CHANGED);
}

// Every channel of `middle`'s centre strictly between `low`'s and `high`'s.
static void check_between(const voe_render_picture *middle,
			  const voe_render_picture *low,
			  const voe_render_picture *high)
{
	printf("strength 0.5: centre %d, between %d and %d\n", centre(middle)[0],
	       centre(low)[0], centre(high)[0]);
	for (int c = 0; c < 3; c++) {
		VOE_TEST_CHECK(centre(low)[c] < centre(middle)[c]);
		VOE_TEST_CHECK(centre(middle)[c] < centre(high)[c]);
	}
}

static void check_shadows(struct scene *scene)
{
	const voe_render_clouds none = { 0 };
	const voe_render_picture plain = draw_case(scene, FLOOR, none);
	const voe_render_picture full =
		draw_case(scene, FLOOR, clouds_of(1.0f, 500.0f, 500.0f));
	const voe_render_picture zero =
		draw_case(scene, FLOOR, clouds_of(0.0f, 500.0f, 500.0f));
	const voe_render_picture half =
		draw_case(scene, FLOOR, clouds_of(0.5f, 500.0f, 500.0f));
	const voe_render_picture tilted_plain = draw_case(scene, TILTED, none);
	const voe_render_picture tilted =
		draw_case(scene, TILTED, clouds_of(1.0f, 500.0f, 500.0f));
	// The floor is EYE_Y below the eye; the layer's bottom 200 m below it.
	const voe_render_picture below =
		draw_case(scene, FLOOR, clouds_of(1.0f, -EYE_Y - 200.0f, 100.0f));
	const voe_render_picture water_plain = draw_case(scene, WATER, none);
	const voe_render_picture water =
		draw_case(scene, WATER, clouds_of(1.0f, 500.0f, 500.0f));

	if (plain.pixels == NULL || full.pixels == NULL || zero.pixels == NULL ||
	    half.pixels == NULL || tilted_plain.pixels == NULL ||
	    tilted.pixels == NULL || below.pixels == NULL ||
	    water_plain.pixels == NULL || water.pixels == NULL)
		return;
	check_darker("strength 1", &full, &plain);
	VOE_TEST_CHECK(memcmp(zero.pixels, plain.pixels, PIXELS) == 0);
	check_between(&half, &full, &plain);
	printf("facing away: centre %d, without clouds %d, largest difference %d\n",
	       centre(&tilted)[0], centre(&tilted_plain)[0],
	       largest_difference(tilted.pixels, tilted_plain.pixels, PIXELS));
	VOE_TEST_CHECK(largest_difference(tilted.pixels, tilted_plain.pixels,
					  PIXELS) <= SAME);
	VOE_TEST_CHECK(centre(&tilted)[0] > 0);
	VOE_TEST_CHECK(memcmp(below.pixels, plain.pixels, PIXELS) == 0);
	check_darker("water", &water, &water_plain);
}

static bool build_scene(struct scene *scene)
{
	const voe_render_shading_values grey = {
		.base_colour = { GREY, GREY, GREY, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	const voe_render_shading_values water = {
		.base_colour = { 0.2f, 0.5f, 0.6f, 1.0f },
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_BLENDED,
		.water = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_base_error error = VOE_BASE_OK;
	bool built = true;

	built &= voe_render_geometry_create(scene->device, FLOOR_VERTICES, 4,
					    SQUARE_INDICES, 6, &scene->floor,
					    &error);
	built &= voe_render_geometry_create(scene->device, TILTED_VERTICES, 4,
					    SQUARE_INDICES, 6, &scene->tilted,
					    &error);
	built &= voe_render_geometry_create(scene->device, QUAD_VERTICES, 4,
					    QUAD_INDICES, 6, &scene->quad,
					    &error);
	built &= voe_render_shading_create(scene->device, grey, &scene->grey,
					   &error);
	built &= voe_render_shading_create(scene->device, water, &scene->water,
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
	if (build_scene(&scene))
		check_shadows(&scene);
	else
		VOE_TEST_CHECK(false);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
