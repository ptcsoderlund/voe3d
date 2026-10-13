// THE SKY IS SEEN THROUGH THE HAZE (ADR-0418 points 2 to 4, 0419 points 3
// and 4). sky.c's set-up: a target of the test's own seen by a camera at the
// origin looking along −Z, a white sun 30° up straight ahead, ADR-0406 point
// 1's colours drawn, exposure 0 and Clip, read back with
// voe_render_target_read, each pixel's direction as sky.c reads it. `H` is
// the haze's colour as haze.c computes it: hue × B under a drawn sky, the
// lights' colour × intensity / (4π) without. Each within 3/255 of the sRGB
// named:
//
// 1. haze 10, start 0, top 1000: the centre and a pixel near the top, `H`;
// 2. haze 2000, top 1000: the pixel near the top lerp(radiance, H, f), f =
//    1 − 2^(−(1000 / v.y) / 2000), radiance from voe_render_sky_radiance
//    (../src/sky.h); a pixel below the centre, `H`;
// 3. haze 10, top −10: the pixel near the top byte for byte as with haze 0;
// 4. haze 10, top 1000: the pixel nearest the sun `H`, not the disc;
// 5. not drawn, haze 10, top 1000: the centre `H`; with top −10, the pixel
//    near the top byte for byte as a zeroed sky reads there, the clear colour.
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

#define SIDE 64
// The rows the near-the-top and below-the-centre pixels are read at.
#define TOP_ROW 2
#define BELOW_ROW (SIDE / 2 + 8)
#define TOLERANCE 3

#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

#define FOUR_PI (4.0f * 3.14159265f)

// ADR-0406 point 1's defaults, drawn; each case sets its haze.
static const voe_render_sky DRAWN = {
	.zenith = { 0.15f, 0.35f, 0.80f },
	.horizon_height = 0.25f,
	.horizon = { 0.60f, 0.75f, 0.90f },
	.strength = 1.0f,
	.ground = { 0.20f, 0.18f, 0.16f },
	.drawn = 1,
	.disc_size = { 1.5f },
	.glow_size = { 0.5f },
};

// The sun 30° above the horizon straight ahead; its light travels away from it.
static const voe_render_light SUN = {
	.direction = { 0.0f, -0.5f, 0.8660254f },
	.intensity = 1.0f,
	.colour = { 1.0f, 1.0f, 1.0f },
};

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 1,
	.passes = 1,
	.targets = 1,
};

struct scene {
	voe_render_device *device;
	voe_base_arena *arena;
	voe_render_target target;
};

// sky.c's camera: the eye at the origin looking along −Z, +Y up, reverse-Z.
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

// One frame of one empty pass under `sky`, the target read back; NULL when
// the read failed.
static const uint8_t *picture_of(struct scene *scene, voe_render_sky sky)
{
	voe_platform_size size = { SIDE, SIDE };
	const voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = SUN,
		.sky = sky,
		.exposure = { .tone = VOE_RENDER_TONE_CLIP },
	};
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return NULL;
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device, scene->target,
					     &camera));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	VOE_TEST_CHECK(voe_render_target_read(scene->device, scene->target,
					      scene->arena, &picture, &error));
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

static const uint8_t *pixel_at(const uint8_t *pixels, int column, int row)
{
	return pixels + ((size_t)row * SIDE + column) * 4;
}

static void check_pixel(const char *name, const uint8_t *pixel,
			voe_math_float3 expected)
{
	printf("%s: %d %d %d, want %d %d %d\n", name, pixel[0], pixel[1],
	       pixel[2], srgb_byte(expected.x), srgb_byte(expected.y),
	       srgb_byte(expected.z));
	VOE_TEST_CHECK(abs(pixel[0] - srgb_byte(expected.x)) <= TOLERANCE);
	VOE_TEST_CHECK(abs(pixel[1] - srgb_byte(expected.y)) <= TOLERANCE);
	VOE_TEST_CHECK(abs(pixel[2] - srgb_byte(expected.z)) <= TOLERANCE);
}

static void check_same(const char *name, const uint8_t *pixel,
		       const uint8_t *want)
{
	printf("%s: %d %d %d, want %d %d %d\n", name, pixel[0], pixel[1],
	       pixel[2], want[0], want[1], want[2]);
	for (int i = 0; i < 3; i++)
		VOE_TEST_CHECK_INT(pixel[i], want[i]);
}

// `H` as haze.c computes it: under a drawn sky hue × B, hue the gradient
// along a level view at strength 1 over its largest channel, B the sun's
// largest channel × intensity / (4π); without, its colour × intensity / (4π).
static voe_math_float3 haze_colour(bool drawn)
{
	const float brightness = SUN.intensity / FOUR_PI;
	voe_render_sky full = DRAWN;
	voe_math_float3 horizon;
	float peak;

	if (!drawn)
		return voe_math_float3_scale(SUN.colour, brightness);
	full.strength = 1.0f;
	horizon = voe_render_sky_gradient(&full,
					  (voe_math_float3){ 0.0f, 0.0f, -1.0f });
	peak = fmaxf(horizon.x, fmaxf(horizon.y, horizon.z));
	return voe_math_float3_scale(horizon, brightness / peak);
}

// DRAWN with `haze`, start 0 and `top`, drawn when `drawn`.
static voe_render_sky hazed(float haze, float top, bool drawn)
{
	voe_render_sky sky = DRAWN;

	sky.haze = haze;
	sky.haze_top = top;
	sky.drawn = drawn ? 1u : 0u;
	return sky;
}

// Cases 1 and 2: dense haze veils the whole sky, thin haze the horizon most.
static void check_drawn(struct scene *scene)
{
	const voe_math_float3 h = haze_colour(true);
	const voe_math_float3 up = direction_of(SIDE / 2, TOP_ROW);
	const float f = 1.0f - exp2f(-(1000.0f / up.y) / 2000.0f);
	const voe_math_float3 radiance =
		voe_render_sky_radiance(&DRAWN, &SUN, 1, up);
	const uint8_t *pixels;

	if ((pixels = picture_of(scene, hazed(10.0f, 1000.0f, true))) != NULL) {
		check_pixel("dense, centre", pixel_at(pixels, SIDE / 2, SIDE / 2),
			    h);
		check_pixel("dense, near the top",
			    pixel_at(pixels, SIDE / 2, TOP_ROW), h);
	}
	if ((pixels = picture_of(scene, hazed(2000.0f, 1000.0f, true))) == NULL)
		return;
	check_pixel("thin, near the top", pixel_at(pixels, SIDE / 2, TOP_ROW),
		    voe_math_float3_add(voe_math_float3_scale(radiance, 1.0f - f),
					voe_math_float3_scale(h, f)));
	check_pixel("thin, below the centre",
		    pixel_at(pixels, SIDE / 2, BELOW_ROW), h);
}

// Cases 3 and 4: an eye above the layer sees the sky clear above it; dense
// haze takes the sun's disc with the sky.
static void check_above_and_sun(struct scene *scene)
{
	const float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	const voe_math_float3 at = voe_math_float3_neg(SUN.direction);
	const int column = (int)floorf((focal * at.x / -at.z + 1.0f) * 0.5f *
				       SIDE);
	const int row = (int)floorf((1.0f - focal * at.y / -at.z) * 0.5f * SIDE);
	const uint8_t *clear = picture_of(scene, hazed(0.0f, -10.0f, true));
	const uint8_t *above = picture_of(scene, hazed(10.0f, -10.0f, true));
	const uint8_t *pixels;

	if (clear != NULL && above != NULL)
		check_same("above the layer, near the top",
			   pixel_at(above, SIDE / 2, TOP_ROW),
			   pixel_at(clear, SIDE / 2, TOP_ROW));
	if ((pixels = picture_of(scene, hazed(10.0f, 1000.0f, true))) != NULL)
		check_pixel("dense, nearest the sun",
			    pixel_at(pixels, column < 0 ? 0 : column,
				     row < 0 ? 0 : row),
			    haze_colour(true));
}

// Case 5: with no sky drawn, the clear backdrop veiled the same way.
static void check_not_drawn(struct scene *scene)
{
	const uint8_t *zeroed = picture_of(scene, (voe_render_sky){ 0 });
	const uint8_t *above = picture_of(scene, hazed(10.0f, -10.0f, false));
	const uint8_t *pixels;

	if ((pixels = picture_of(scene, hazed(10.0f, 1000.0f, false))) != NULL)
		check_pixel("not drawn, centre",
			    pixel_at(pixels, SIDE / 2, SIDE / 2),
			    haze_colour(false));
	if (zeroed != NULL && above != NULL)
		check_same("not drawn, above the layer, near the top",
			   pixel_at(above, SIDE / 2, TOP_ROW),
			   pixel_at(zeroed, SIDE / 2, TOP_ROW));
}

int main(void)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { .arena = voe_base_arena_new(1024 * 1024) };
	voe_render_texture shown = { 0 };

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

	check_drawn(&scene);
	check_above_and_sun(&scene);
	check_not_drawn(&scene);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
