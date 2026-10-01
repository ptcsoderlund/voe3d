// The water path (ADR-0305): a `water` shading record drawn blended over the
// pass's depth copy. A ground plane slopes down through y = 0 along +x, the water
// is a flat 6 m quad at y = 0 turned from XY to face up, and the camera of
// tests/shadow.c looks down at both from (0, 5, 5). Six claims.
//
// THE SHORE IS CLEAR AND THE DEEP IS DARK. Where the ground meets the water the
// pixel reads as the ground drawn alone; where the water is deeper than `deep`
// it reads as the darkened body colour this file works out on its own, and the
// same with no copy made, which the shader treats as deep.
//
// THE WAVES MOVE AND THE CLOCK WRAPS. Somewhere in the picture a pixel changes
// between seconds 0 and 1.3; seconds 0 and 60 are the same picture within one
// step of 8 bits.
//
// THE SUN GLINTS AT THE MIRROR ANGLE. Over the deep water, the brightest pixel
// with the sun mirrored about the surface from the eye is brighter than the
// brightest with the sun behind the eye.
//
// A SHADOW FALLS ON IT. A square caster above the deep water, the sun straight
// down through cascade 0: the water under it is darker than with no cascades.
//
// `water` 0 IS AN ORDINARY BLENDED SURFACE. The same quad with waves and sky set
// draws exactly as with them zeroed, and does not fade at the shore.
//
// Pixels are found by projecting world points through the camera, with the
// engine's one Y flip. A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SIDE 128
#define SHADOW_SIDE 256
#define LIGHT_HALF 4.0f
#define LIGHT_HEIGHT 10.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 20.0f
#define EYE 5.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define WATER_SIDE 6.0f
#define DEEP 1.0f
// draw.slang's constants the deep colour is worked out from.
#define DEEP_COVERAGE 0.95f
#define DEEP_DARKENING 0.4f
#define REFLECTANCE 0.02f
// A driver's rounding into an sRGB target, and for the shore a few per cent of
// water over the ground.
#define TOLERANCE 4
#define SHORE_TOLERANCE 12

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 12,
	.geometries = 2,
	.objects = 6,
	.shadings = 4,
	.passes = 2,
	.shadow_size = SHADOW_SIDE,
};

static const voe_math_float3 GROUND = { 0.8f, 0.6f, 0.3f };
static const voe_math_float3 WATER = { 0.05f, 0.3f, 0.35f };
static const voe_math_float3 SKY = { 0.5f, 0.7f, 0.9f };
static const voe_math_float3 EYE_AT = { 0.0f, EYE, EYE };
static const voe_math_float3 DOWN = { 0.0f, -1.0f, 0.0f };
// Where the claims look: just past the shore, over deep water, and under the
// caster, which floats at CASTER_HEIGHT out of the camera's line to it.
static const voe_math_float3 SHORE = { 0.05f, 0.0f, 0.0f };
static const voe_math_float3 DEEP_AT = { 2.5f, 0.0f, 0.0f };
static const voe_math_float3 UNDER = { 2.0f, 0.0f, -1.0f };
#define CASTER_HEIGHT 1.5f

// The ground, y = -x / 2 across 8 m, counter-clockwise from above.
static const voe_render_vertex GROUND_VERTICES[4] = {
	{ { -4.0f, 2.0f, 4.0f }, { 0.4472136f, 0.8944272f, 0 }, { 0, 0 } },
	{ { 4.0f, -2.0f, 4.0f }, { 0.4472136f, 0.8944272f, 0 }, { 0, 0 } },
	{ { 4.0f, -2.0f, -4.0f }, { 0.4472136f, 0.8944272f, 0 }, { 0, 0 } },
	{ { -4.0f, 2.0f, -4.0f }, { 0.4472136f, 0.8944272f, 0 }, { 0, 0 } },
};
static const uint32_t GROUND_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

// A unit quad in model XY facing +z, as the model store's quad is.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -0.5f, 0.5f, 0 }, { 0, 0, 1 }, { 0, 0 } },
	{ { 0.5f, 0.5f, 0 }, { 0, 0, 1 }, { 1, 0 } },
	{ { 0.5f, -0.5f, 0 }, { 0, 0, 1 }, { 1, 1 } },
	{ { -0.5f, -0.5f, 0 }, { 0, 0, 1 }, { 0, 1 } },
};
static const uint32_t QUAD_INDICES[6] = { 3, 2, 1, 3, 1, 0 };

struct scene {
	voe_render_device *device;
	voe_render_geometry ground;
	voe_render_geometry quad;
	voe_render_shading ground_record;
	voe_render_shading water_record;
	voe_render_shading plain_record;
	voe_render_shading caster_record;
	voe_render_view camera;
	voe_render_view light;
};

// What one frame draws: the water quad's object, or none, whether the depth is
// copied before it, the sun's direction, and whether the caster and cascade 0
// are in it.
struct look {
	const voe_render_object *water;
	bool copy;
	voe_math_float3 sun;
	bool shadowed;
};

// tests/shadow.c's camera: at (0, EYE, EYE) looking at the origin, reverse-Z.
static voe_render_view scene_camera(void)
{
	voe_math_float3 z = voe_math_float3_normalize(EYE_AT);
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x =
		voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_math_float3 y = voe_math_float3_cross(z, x);
	voe_math_float3 rows[3] = { x, y, z };
	voe_render_view view = { .eye = EYE_AT };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], EYE_AT);
	}
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// tests/shadow.c's sun: straight down from LIGHT_HEIGHT, orthographic, reverse-Z.
static voe_render_view scene_light(void)
{
	voe_render_view light = { .eye = { 0.0f, LIGHT_HEIGHT, 0.0f } };
	float span = LIGHT_FAR - LIGHT_NEAR;

	light.view.m[0][0] = 1.0f;
	light.view.m[1][2] = -1.0f;
	light.view.m[2][1] = 1.0f;
	light.view.m[2][3] = -LIGHT_HEIGHT;
	light.view.m[3][3] = 1.0f;
	light.projection.m[0][0] = 1.0f / LIGHT_HALF;
	light.projection.m[1][1] = 1.0f / LIGHT_HALF;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = LIGHT_FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

// The quad turned from XY to XZ facing up, `side` metres square, centred on
// `at`: model +y runs along world -z, so the turn keeps the winding.
static voe_render_object lying(voe_render_shading shading, voe_math_float3 at,
			       float side)
{
	voe_render_object object = {
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};

	object.world.m[0][0] = side;
	object.world.m[0][3] = at.x;
	object.world.m[1][2] = 1.0f;
	object.world.m[1][3] = at.y;
	object.world.m[2][1] = -side;
	object.world.m[2][3] = at.z;
	object.world.m[3][3] = 1.0f;
	object.normal.m[0][0] = 1.0f / side;
	object.normal.m[1][2] = 1.0f;
	object.normal.m[2][1] = -1.0f / side;
	object.normal.m[3][3] = 1.0f;
	return object;
}

static voe_render_object water_object(voe_render_shading shading, float height,
				      float seconds)
{
	voe_render_object object =
		lying(shading, (voe_math_float3){ 0 }, WATER_SIDE);

	object.waves = (voe_math_float4){ height, 2.0f, seconds, DEEP };
	object.sky = (voe_math_float4){ SKY.x, SKY.y, SKY.z, 0.0f };
	return object;
}

static voe_render_object ground_object(voe_render_shading shading)
{
	return (voe_render_object){
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// The ground, and the caster above UNDER when the look has a shadow.
static void draw_solids(struct scene *scene, const struct look *look)
{
	voe_math_float3 above = { UNDER.x, CASTER_HEIGHT, UNDER.z };

	VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->ground,
					     ground_object(scene->ground_record)));
	if (look->shadowed)
		VOE_TEST_CHECK(voe_render_frame_draw(
			scene->device, scene->quad,
			lying(scene->caster_record, above, 1.0f)));
}

// One frame of `look` into the window, read back into `arena`.
static voe_render_picture draw_frame(struct scene *scene,
				     const struct look *look,
				     voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	voe_render_pass_camera camera = {
		.view = scene->camera,
		.light = { .direction = look->sun,
			   .intensity = 3.14159265f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
		.shadow = { .splits = { FAR_PLANE },
			    .texels = { 2.0f * LIGHT_HALF / SHADOW_SIDE },
			    .count = look->shadowed ? 1 : 0 },
	};
	bool drawing = false;

	camera.shadow.cascades[0] = voe_math_float4x4_mul(
		scene->light.projection, scene->light.view);
	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	if (!drawing)
		return picture;
	if (look->shadowed) {
		VOE_TEST_CHECK(voe_render_shadow_pass_begin(scene->device, 0,
							    &scene->light));
		draw_solids(scene, look);
		voe_render_pass_end(scene->device);
	}
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device,
					     VOE_RENDER_TARGET_WINDOW, &camera));
	draw_solids(scene, look);
	if (look->copy)
		VOE_TEST_CHECK(voe_render_frame_copy_depth(scene->device));
	if (look->water != NULL)
		VOE_TEST_CHECK(voe_render_frame_draw_blended(
			scene->device, scene->quad, *look->water));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	VOE_TEST_CHECK(voe_render_target_read(scene->device,
					      VOE_RENDER_TARGET_WINDOW, arena,
					      &picture, &error));
	return picture;
}

// The RGBA pixel `world` lands on, or NULL off the picture.
static const uint8_t *pixel_at(const voe_render_picture *picture,
			       const voe_render_view *camera,
			       voe_math_float3 world)
{
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(camera->projection, camera->view),
		(voe_math_float4){ world.x, world.y, world.z, 1.0f });
	float column = (clip.x / clip.w * 0.5f + 0.5f) * (float)picture->width;
	float row = (0.5f - clip.y / clip.w * 0.5f) * (float)picture->height;

	if (picture->pixels == NULL || column < 0.0f || row < 0.0f ||
	    column >= (float)picture->width || row >= (float)picture->height)
		return NULL;
	return picture->pixels +
	       ((size_t)row * picture->width + (size_t)column) * 4;
}

// The largest difference of any channel between two pixels; 256 for a missing one.
static int pixel_gap(const uint8_t *a, const uint8_t *b)
{
	int gap = 0;

	if (a == NULL || b == NULL)
		return 256;
	for (int c = 0; c < 3; c++)
		if (abs(a[c] - b[c]) > gap)
			gap = abs(a[c] - b[c]);
	return gap;
}

// The largest difference of any channel anywhere in two pictures.
static int picture_gap(const voe_render_picture *a, const voe_render_picture *b)
{
	int gap = 0;

	if (a->pixels == NULL || b->pixels == NULL)
		return 256;
	for (size_t i = 0; i < (size_t)SIDE * SIDE; i++) {
		int here = pixel_gap(a->pixels + i * 4, b->pixels + i * 4);

		if (here > gap)
			gap = here;
	}
	return gap;
}

static float srgb_encode(float linear)
{
	return linear <= 0.0031308f ? linear * 12.92f :
				      1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;
}

static float srgb_decode(int byte)
{
	float encoded = (float)byte / 255.0f;

	return encoded <= 0.04045f ? encoded / 12.92f :
				     powf((encoded + 0.055f) / 1.055f, 2.4f);
}

// The deep water at `at` under the sun straight down, flat, worked out here:
// the darkened body lit head-on, the sky's fresnel share, and the ground's
// pixel `under` through what coverage leaves.
static void expect_deep(voe_math_float3 at, const uint8_t *under,
			uint8_t expected[3])
{
	voe_math_float3 v = voe_math_float3_normalize(
		voe_math_float3_sub(EYE_AT, at));
	float sky_share = REFLECTANCE +
			  (1.0f - REFLECTANCE) * powf(1.0f - v.y, 5.0f);
	float body[3] = { WATER.x, WATER.y, WATER.z };
	float sky[3] = { SKY.x, SKY.y, SKY.z };

	for (int c = 0; c < 3; c++) {
		float lit = (1.0f - sky_share) * body[c] * DEEP_DARKENING +
			    sky_share * sky[c];
		float out = DEEP_COVERAGE * lit +
			    (1.0f - DEEP_COVERAGE) * srgb_decode(under[c]);

		expected[c] = (uint8_t)lroundf(srgb_encode(out) * 255.0f);
	}
}

static void the_shore_is_clear_and_the_deep_dark(struct scene *scene,
						 voe_base_arena *arena)
{
	voe_render_object water = water_object(scene->water_record, 0.0f, 0.0f);
	struct look bare = { .sun = DOWN };
	struct look wet = { .water = &water, .copy = true, .sun = DOWN };
	struct look uncopied = { .water = &water, .sun = DOWN };
	voe_render_picture ground = draw_frame(scene, &bare, arena);
	voe_render_picture drawn = draw_frame(scene, &wet, arena);
	voe_render_picture deep = draw_frame(scene, &uncopied, arena);
	const uint8_t *ground_deep = pixel_at(&ground, &scene->camera, DEEP_AT);
	uint8_t expected[3] = { 0 };

	const uint8_t *got = pixel_at(&drawn, &scene->camera, DEEP_AT);

	VOE_TEST_CHECK(ground_deep != NULL && got != NULL);
	if (ground_deep == NULL || got == NULL)
		return;
	expect_deep(DEEP_AT, ground_deep, expected);

	printf("shore gap %d, deep %d %d %d against %d %d %d, no copy gap %d\n",
	       pixel_gap(pixel_at(&ground, &scene->camera, SHORE),
			 pixel_at(&drawn, &scene->camera, SHORE)),
	       got[0], got[1], got[2], expected[0], expected[1], expected[2],
	       pixel_gap(got, pixel_at(&deep, &scene->camera, DEEP_AT)));
	VOE_TEST_CHECK(pixel_gap(pixel_at(&ground, &scene->camera, SHORE),
				 pixel_at(&drawn, &scene->camera, SHORE)) <=
		       SHORE_TOLERANCE);
	VOE_TEST_CHECK(pixel_gap(got, expected) <= TOLERANCE);
	VOE_TEST_CHECK(pixel_gap(got, ground_deep) > 40);
	VOE_TEST_CHECK(pixel_gap(got, pixel_at(&deep, &scene->camera,
					       DEEP_AT)) <= 1);
}

static void the_waves_move_and_wrap(struct scene *scene, voe_base_arena *arena)
{
	voe_render_object at_zero = water_object(scene->water_record, 0.05f, 0.0f);
	voe_render_object later = water_object(scene->water_record, 0.05f, 1.3f);
	voe_render_object wrapped = water_object(scene->water_record, 0.05f, 60.0f);
	struct look look = { .water = &at_zero, .copy = true, .sun = DOWN };
	voe_render_picture zero = draw_frame(scene, &look, arena);
	voe_render_picture moved;
	voe_render_picture minute;

	look.water = &later;
	moved = draw_frame(scene, &look, arena);
	look.water = &wrapped;
	minute = draw_frame(scene, &look, arena);
	printf("waves: 1.3 s gap %d, 60 s gap %d\n", picture_gap(&zero, &moved),
	       picture_gap(&zero, &minute));
	VOE_TEST_CHECK(picture_gap(&zero, &moved) > TOLERANCE);
	VOE_TEST_CHECK(picture_gap(&zero, &minute) <= 1);
}

// The brightest pixel, summed over its channels, among points over the deep
// water from x 1 to 3 and z -2 to 2.
static int peak_over_deep(const voe_render_picture *picture,
			  const voe_render_view *camera)
{
	int peak = 0;

	for (int i = 0; i <= 40; i++) {
		for (int j = 0; j <= 80; j++) {
			voe_math_float3 at = { 1.0f + i * 0.05f, 0.0f,
					       -2.0f + j * 0.05f };
			const uint8_t *pixel = pixel_at(picture, camera, at);

			if (pixel != NULL && pixel[0] + pixel[1] + pixel[2] > peak)
				peak = pixel[0] + pixel[1] + pixel[2];
		}
	}
	return peak;
}

static void the_sun_glints_at_the_mirror_angle(struct scene *scene,
					       voe_base_arena *arena)
{
	voe_render_object water = water_object(scene->water_record, 0.0f, 0.0f);
	voe_math_float3 seen = voe_math_float3_normalize(
		voe_math_float3_sub(EYE_AT, DEEP_AT));
	voe_math_float3 mirror = { seen.x, -seen.y, seen.z };
	struct look look = { .water = &water, .copy = true, .sun = mirror };
	voe_render_picture glint = draw_frame(scene, &look, arena);
	voe_render_picture behind;

	look.sun = voe_math_float3_neg(seen);
	behind = draw_frame(scene, &look, arena);
	printf("peak: mirror %d, behind %d\n",
	       peak_over_deep(&glint, &scene->camera),
	       peak_over_deep(&behind, &scene->camera));
	VOE_TEST_CHECK(peak_over_deep(&glint, &scene->camera) >
		       peak_over_deep(&behind, &scene->camera) + 60);
}

static void a_shadow_falls_on_it(struct scene *scene, voe_base_arena *arena)
{
	voe_render_object water = water_object(scene->water_record, 0.0f, 0.0f);
	struct look look = { .water = &water, .copy = true, .sun = DOWN,
			     .shadowed = true };
	voe_render_picture shadowed = draw_frame(scene, &look, arena);
	const uint8_t *dark = pixel_at(&shadowed, &scene->camera, UNDER);

	VOE_TEST_CHECK(dark != NULL);
	if (dark == NULL)
		return;
	// The same frame with no caster and no cascades read.
	look.shadowed = false;
	voe_render_picture lit = draw_frame(scene, &look, arena);
	const uint8_t *bright = pixel_at(&lit, &scene->camera, UNDER);

	printf("under the caster: green %d shadowed, %d lit\n", dark[1],
	       bright != NULL ? bright[1] : -1);
	VOE_TEST_CHECK(bright != NULL && dark[1] + 30 < bright[1]);
}

static void no_water_is_an_ordinary_blended_surface(struct scene *scene,
						    voe_base_arena *arena)
{
	voe_render_object waved = water_object(scene->plain_record, 0.05f, 1.3f);
	voe_render_object calm = waved;
	struct look look = { .water = &waved, .copy = true, .sun = DOWN };
	struct look bare = { .sun = DOWN };
	voe_render_picture with_waves = draw_frame(scene, &look, arena);
	voe_render_picture without;
	voe_render_picture ground = draw_frame(scene, &bare, arena);

	calm.waves = (voe_math_float4){ 0 };
	calm.sky = (voe_math_float4){ 0 };
	look.water = &calm;
	without = draw_frame(scene, &look, arena);
	VOE_TEST_CHECK_INT(picture_gap(&with_waves, &without), 0);
	VOE_TEST_CHECK(pixel_gap(pixel_at(&ground, &scene->camera, SHORE),
				 pixel_at(&with_waves, &scene->camera, SHORE)) >
		       SHORE_TOLERANCE);
}

static bool build_scene(struct scene *scene)
{
	voe_render_shading_values ground = {
		.base_colour = { GROUND.x, GROUND.y, GROUND.z, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_shading_values water = {
		.base_colour = { WATER.x, WATER.y, WATER.z, 1.0f },
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_BLENDED,
		.water = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_shading_values plain = water;
	voe_render_shading_values caster = ground;
	voe_base_error error = VOE_BASE_OK;
	bool built = true;

	plain.water = 0;
	plain.base_colour.w = 0.5f;
	caster.base_colour = (voe_math_float4){ 0.5f, 0.5f, 0.5f, 1.0f };
	built &= voe_render_shading_create(scene->device, ground,
					   &scene->ground_record, &error);
	built &= voe_render_shading_create(scene->device, water,
					   &scene->water_record, &error);
	built &= voe_render_shading_create(scene->device, plain,
					   &scene->plain_record, &error);
	built &= voe_render_shading_create(scene->device, caster,
					   &scene->caster_record, &error);
	built &= voe_render_geometry_create(scene->device, GROUND_VERTICES, 4,
					    GROUND_INDICES, 6, &scene->ground,
					    &error);
	built &= voe_render_geometry_create(scene->device, QUAD_VERTICES, 4,
					    QUAD_INDICES, 6, &scene->quad,
					    &error);
	return built;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4 * 1024 * 1024);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { .camera = scene_camera(),
			       .light = scene_light() };

	scene.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						      &error);
	if (scene.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(scene.device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	if (build_scene(&scene)) {
		the_shore_is_clear_and_the_deep_dark(&scene, arena);
		the_waves_move_and_wrap(&scene, arena);
		the_sun_glints_at_the_mirror_angle(&scene, arena);
		a_shadow_falls_on_it(&scene, arena);
		no_water_is_an_ordinary_blended_surface(&scene, arena);
	} else {
		VOE_TEST_CHECK(false);
	}

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
