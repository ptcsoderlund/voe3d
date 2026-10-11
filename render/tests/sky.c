// A PASS DRAWS THE SKY BEHIND EVERYTHING (ADR-0405 point 4). A target of the
// test's own seen by a camera at the origin looking along −Z, the horizon
// through its middle, a white sun 30° up straight ahead, read back with
// voe_render_target_read:
//
// 1. with `drawn`, exposure 0 and Clip, the centre pixel and one near the top
//    are the sRGB of voe_render_sky_radiance (render/src/sky.h, the CPU twin)
//    along their own directions, within 2/255;
// 2. an unlit red quad drawn in the pass covers the sky at the centre;
// 3. the sky zeroed reads pass.c's clear colour, the old picture;
// 4. a sun of intensity 0 is a black sky;
// 5. some frames later, `voe_render_frame_pass_times` lists the target's pass
//    then `<pass>: sky`. A card that writes no timestamps skips this part.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO. The skip is a pass and it
// prints its reason.
#include "../src/sky.h"

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
// The row the near-the-top pixel is read at.
#define TOP_ROW 2
#define TOLERANCE 2
// Frames enough that the newest measured one drew the sky.
#define FRAMES 4

#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// pass.c's clear colour, linear.
#define CLEAR_RED 0.00857f
#define CLEAR_BLUE 0.01033f

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
	.targets = 1,
};

// Half a metre square at z = −2, facing the eye, counter-clockwise from it.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -0.25f, -0.25f, -2.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 0.25f, -0.25f, -2.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 0.25f, 0.25f, -2.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { -0.25f, 0.25f, -2.0f }, { 0, 0, 1 }, { 0, 0 } },
};

static const uint32_t QUAD_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

struct scene {
	voe_render_device *device;
	voe_base_arena *arena;
	voe_render_target target;
	voe_render_geometry quad;
	voe_render_shading red;
};

// The eye at the origin looking along −Z, +Y up, reverse-Z as exposure.c's.
static voe_render_view the_camera(void)
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

// The sun 30° above the horizon straight ahead; its light travels away from it.
static voe_render_light the_sun(float intensity)
{
	return (voe_render_light){
		.direction = { 0.0f, -0.5f, 0.8660254f },
		.intensity = intensity,
		.colour = { 1.0f, 1.0f, 1.0f },
	};
}

// One frame of one pass onto the target, the quad drawn when `quad`.
static void draw_frame(struct scene *scene, voe_render_light light,
		       voe_render_sky sky, bool quad)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = light,
		.sky = sky,
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = scene->red.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device, scene->target,
					     &camera));
	if (quad)
		VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->quad,
						     object));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
}

// One frame drawn and the target read back; NULL when the read failed.
static const uint8_t *picture_of(struct scene *scene, voe_render_light light,
				 voe_render_sky sky, bool quad)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	draw_frame(scene, light, sky, quad);
	VOE_TEST_CHECK(voe_render_target_read(scene->device, scene->target,
					      scene->arena, &picture, &error));
	VOE_TEST_CHECK_INT(picture.width, SIDE);
	if (picture.pixels == NULL || picture.width != SIDE ||
	    picture.height != SIDE)
		return NULL;
	return picture.pixels;
}

// A linear value as the byte an sRGB target stores for it, clamped as it does.
static int srgb_byte(float linear)
{
	float x = fminf(fmaxf(linear, 0.0f), 1.0f);
	float encoded = x <= 0.0031308f ? x * 12.92f :
					  1.055f * powf(x, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

// The world direction through pixel (column, row)'s centre, row 0 the top.
static voe_math_float3 direction_of(int column, int row)
{
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float x = 2.0f * ((float)column + 0.5f) / SIDE - 1.0f;
	float y = 1.0f - 2.0f * ((float)row + 0.5f) / SIDE;

	return voe_math_float3_normalize(
		(voe_math_float3){ x / focal, y / focal, -1.0f });
}

static void check_pixel(const uint8_t *pixels, int column, int row,
			voe_math_float3 expected, int tolerance)
{
	const uint8_t *pixel = pixels + ((size_t)row * SIDE + column) * 4;

	printf("pixel (%d, %d): %d %d %d, want %d %d %d\n", column, row,
	       pixel[0], pixel[1], pixel[2], srgb_byte(expected.x),
	       srgb_byte(expected.y), srgb_byte(expected.z));
	VOE_TEST_CHECK(abs(pixel[0] - srgb_byte(expected.x)) <= tolerance);
	VOE_TEST_CHECK(abs(pixel[1] - srgb_byte(expected.y)) <= tolerance);
	VOE_TEST_CHECK(abs(pixel[2] - srgb_byte(expected.z)) <= tolerance);
}

static void check_sky(const uint8_t *pixels, voe_render_light sun)
{
	const voe_math_float3 sunlight = voe_math_float3_scale(sun.colour,
							       sun.intensity);
	const int rows[2] = { SIDE / 2, TOP_ROW };

	for (int i = 0; i < 2; i++)
		check_pixel(pixels, SIDE / 2, rows[i],
			    voe_render_sky_radiance(sun.direction, sunlight,
						    direction_of(SIDE / 2,
								 rows[i])),
			    TOLERANCE);
}

static void check_pictures(struct scene *scene)
{
	const voe_render_sky drawn = { .drawn = 1,
				       .tone = VOE_RENDER_TONE_CLIP };
	const voe_render_light sun = the_sun(1.0f);
	const uint8_t *pixels;

	pixels = picture_of(scene, sun, drawn, false);
	if (pixels != NULL)
		check_sky(pixels, sun);
	pixels = picture_of(scene, sun, drawn, true);
	if (pixels != NULL)
		check_pixel(pixels, SIDE / 2, SIDE / 2,
			    (voe_math_float3){ 1.0f, 0.0f, 0.0f }, 0);
	pixels = picture_of(scene, sun, (voe_render_sky){ 0 }, false);
	if (pixels != NULL)
		check_pixel(pixels, SIDE / 2, SIDE / 2,
			    (voe_math_float3){ CLEAR_RED, CLEAR_RED,
					       CLEAR_BLUE },
			    1);
	pixels = picture_of(scene, the_sun(0.0f), drawn, false);
	if (pixels != NULL)
		check_pixel(pixels, SIDE / 2, SIDE / 2,
			    (voe_math_float3){ 0.0f, 0.0f, 0.0f }, 0);
}

static void check_breakdown(struct scene *scene)
{
	const voe_render_sky drawn = { .drawn = 1 };
	voe_render_pass_time times[4];
	char pass[VOE_RENDER_PASS_NAME];
	char span[VOE_RENDER_PASS_NAME + 8];
	double seconds = 0.0;
	uint32_t count;

	for (int i = 0; i < FRAMES; i++)
		draw_frame(scene, the_sun(1.0f), drawn, false);
	if (!voe_render_frame_gpu_time(scene->device, &seconds)) {
		printf("skip: this card writes no timestamps, so there is no breakdown\n");
		return;
	}
	(void)snprintf(pass, sizeof(pass), "view target %u", scene->target.index);
	(void)snprintf(span, sizeof(span), "%s: sky", pass);
	count = voe_render_frame_pass_times(scene->device, times, 4);
	VOE_TEST_CHECK_INT(count, 2);
	if (count != 2)
		return;
	printf("%s, %s %.9f s\n", times[0].name, times[1].name,
	       times[1].seconds);
	VOE_TEST_CHECK(strcmp(times[0].name, pass) == 0);
	VOE_TEST_CHECK(strcmp(times[1].name, span) == 0);
}

int main(void)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { .arena = voe_base_arena_new(1024 * 1024) };
	voe_render_texture shown = { 0 };
	voe_render_shading_values red = {
		.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};

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
	VOE_TEST_CHECK(voe_render_target_create(scene.device, SIDE, SIDE,
						&scene.target, &shown, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(scene.device, QUAD_VERTICES, 4,
						  QUAD_INDICES, 6, &scene.quad,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, red, &scene.red,
						 &error));

	check_pictures(&scene);
	check_breakdown(&scene);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
