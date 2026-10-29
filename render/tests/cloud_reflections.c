// SHINY SURFACES AND WATER MIRROR THE CLOUDS (ADR-0492 point 8). A floor at
// y = 0 seen from 2 m up, the camera pitched 20° down so the centre pixel
// looks at it at a slant; the sky drawn at sky_light.c's values with haze 0,
// one white light straight down, exposure 0 at Clip; the clouds Flat,
// coverage 1, bottom 500 m above the eye, shadow strength 0 so only what is
// mirrored changes, read by the surfaces and never drawn. Three claims:
//
// 1. a smooth white metal floor: its centre more than 8/255 off its bytes
//    with no clouds;
// 2. that floor under clouds of coverage 0: every pixel within 1/255 of no
//    clouds;
// 3. a flat water plane, deep with no depth copy: its centre changed against
//    no clouds.
//
// Built as cloud_shadows.c builds its device, reading the window back through
// voe_render_target_read as RGBA8.
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

#define SIDE 64
#define PIXELS (SIDE * SIDE * 4)
#define CHANGED 8
#define SAME 1

#define EYE_Y 2.0f
#define PITCH 0.34906585f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define WATER_SIDE 40.0f
#define DEEP 1.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 12,
	.geometries = 2,
	.objects = 1,
	.shadings = 2,
	.passes = 1,
};

// A 40 m square at y = 0 facing up, counter-clockwise seen from above.
#define G 20.0f
static const voe_render_vertex FLOOR_VERTICES[4] = {
	{ { -G, 0, G }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { G, 0, G }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { G, 0, -G }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
	{ { -G, 0, -G }, { 0, 1, 0 }, { 0, 0 }, { 0 } },
};
static const uint32_t FLOOR_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

// A unit quad in model XY facing +z, as the model store's quad is: water.c's.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -0.5f, 0.5f, 0 }, { 0, 0, 1 }, { 0, 0 }, { 0 } },
	{ { 0.5f, 0.5f, 0 }, { 0, 0, 1 }, { 1, 0 }, { 0 } },
	{ { 0.5f, -0.5f, 0 }, { 0, 0, 1 }, { 1, 1 }, { 0 } },
	{ { -0.5f, -0.5f, 0 }, { 0, 0, 1 }, { 0, 1 }, { 0 } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

// sky_light.c's sky, drawn, at haze 0.
static const voe_render_sky SKY = {
	.zenith = { 0.15f, 0.35f, 0.80f },
	.horizon_height = 0.25f,
	.horizon = { 0.60f, 0.75f, 0.90f },
	.strength = 1.0f,
	.ground = { 0.20f, 0.18f, 0.16f },
	.drawn = 1,
	.disc_size = { 1.5f },
	.glow_size = { 0.5f },
};

struct scene {
	voe_render_device *device;
	voe_render_geometry floor;
	voe_render_geometry quad;
	voe_render_shading metal;
	voe_render_shading water;
	voe_base_arena *arena;
};

// At (0, EYE_Y, 0) looking along −Z pitched PITCH down, +X to the right,
// reverse-Z.
static voe_render_view the_camera(void)
{
	voe_math_float3 eye = { 0.0f, EYE_Y, 0.0f };
	voe_math_float3 rows[3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, cosf(PITCH), -sinf(PITCH) },
		{ 0.0f, sinf(PITCH), cosf(PITCH) },
	};
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

// Flat clouds of `coverage` 500 m above the eye, casting no shadow.
static voe_render_clouds clouds_of(float coverage)
{
	return (voe_render_clouds){
		.kind = VOE_RENDER_CLOUDS_FLAT,
		.coverage = coverage,
		.bottom = 500.0f,
		.origin = { 100.0f, 100.0f },
	};
}

// The water quad turned from XY to XZ facing up, flat, centred on the origin:
// water.c's `lying`, with no waves and its sky.
static voe_render_object water_object(voe_render_shading shading)
{
	voe_render_object object = {
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.waves = { 0.0f, 2.0f, 0.0f, DEEP },
		.sky = { 0.5f, 0.7f, 0.9f, 0.0f },
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

// The metal floor, or with `water` the water plane, drawn into the open pass.
static void draw_surface(struct scene *scene, bool water)
{
	const voe_render_object metal = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = scene->metal.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};

	if (water)
		VOE_TEST_CHECK(voe_render_frame_draw_blended(
			scene->device, scene->quad, water_object(scene->water)));
	else
		VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->floor,
						     metal));
}

// One frame of the floor or the water under `clouds`, read back; pixels NULL
// when it failed.
static voe_render_picture draw_case(struct scene *scene, bool water,
				    voe_render_clouds clouds)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	const voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
		.sky = SKY,
		.exposure = { .tone = VOE_RENDER_TONE_CLIP },
		.clouds = clouds,
	};
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device,
					     VOE_RENDER_TARGET_WINDOW, &camera));
	draw_surface(scene, water);
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

static void print_centres(const char *name, const voe_render_picture *with,
			  const voe_render_picture *without)
{
	const uint8_t *a = centre(with);
	const uint8_t *b = centre(without);

	printf("%s: centre %d %d %d, without clouds %d %d %d\n", name, a[0],
	       a[1], a[2], b[0], b[1], b[2]);
}

static void check_reflections(struct scene *scene)
{
	const voe_render_clouds none = { 0 };
	const voe_render_picture plain = draw_case(scene, false, none);
	const voe_render_picture full = draw_case(scene, false, clouds_of(1.0f));
	const voe_render_picture clear = draw_case(scene, false, clouds_of(0.0f));
	const voe_render_picture water_plain = draw_case(scene, true, none);
	const voe_render_picture water = draw_case(scene, true, clouds_of(1.0f));

	if (plain.pixels == NULL || full.pixels == NULL || clear.pixels == NULL ||
	    water_plain.pixels == NULL || water.pixels == NULL)
		return;
	print_centres("metal, coverage 1", &full, &plain);
	VOE_TEST_CHECK(largest_difference(centre(&full), centre(&plain), 4) >
		       CHANGED);
	printf("metal, coverage 0: largest difference %d\n",
	       largest_difference(clear.pixels, plain.pixels, PIXELS));
	VOE_TEST_CHECK(largest_difference(clear.pixels, plain.pixels, PIXELS) <=
		       SAME);
	print_centres("water, coverage 1", &water, &water_plain);
	VOE_TEST_CHECK(largest_difference(centre(&water), centre(&water_plain),
					  4) > 0);
}

static bool build_scene(struct scene *scene)
{
	const voe_render_shading_values metal = {
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 1.0f,
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
					    FLOOR_INDICES, 6, &scene->floor,
					    &error);
	built &= voe_render_geometry_create(scene->device, QUAD_VERTICES, 4,
					    QUAD_INDICES, 6, &scene->quad,
					    &error);
	built &= voe_render_shading_create(scene->device, metal, &scene->metal,
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
		check_reflections(&scene);
	else
		VOE_TEST_CHECK(false);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
