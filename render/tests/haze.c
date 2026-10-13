// FAR SURFACES FADE INTO HAZE (ADR-0405 point 6). A target of the test's own
// seen by a camera at the origin looking along −Z, a white sun 30° up straight
// ahead, ADR-0406 point 1's sky colours drawn, haze 1000 m, exposure 0 and
// Clip, read back with voe_render_target_read at the centre pixel. The quad is
// an unlit black square filling the view at a distance, scaled by its object
// matrix. Under a drawn sky the haze's colour is hue × B (ADR-0416 points 2
// and 3, ADR-0417 point 2): hue the sky's gradient along a level view at
// strength 1 over its largest channel, white when that is not above 0, from
// voe_render_sky_gradient (../src/sky.h) as tone.slangh takes it; B the shown
// lights' Σ largest colour channel × intensity / (4π).
//
// 1. at 2000 m: within 3/255 of the sRGB of (1 − 2^−2) × hue × B;
// 2. at 50 m: darker than a tenth of hue × B;
// 3. haze 0 at 2000 m: black, byte for byte;
// 4. drawn after the pass's depth clear at 2000 m: black, as authored;
// 5. a blended black quad of alpha 0.5 at 2000 m, haze_top 1000 so the sky
//    behind it, looking just below level, is fully veiled: half its hazed
//    colour, (1 − 2^−2) × hue × B, plus half of hue × B, within 3/255;
// 6. the sun 0.5° up ahead, and 90° round to the side, at 2000 m: each within
//    3/255 of case 1, no view term (0415 point 2);
// 7. the sky's strength 0.25 and its zenith, horizon and ground each × 0.1, at
//    2000 m: within 3/255 of case 1, neither strength nor brightness read;
// 8. haze_start 100 m at 50 m: black, byte for byte as with haze 0;
// 9. haze_start 1000 m at 2000 m: within 3/255 of (1 − 2^−1) × hue × B
//    (ADR-0414 point 1);
// 10. an orange light (1, 0.5, 0.2) at intensity 3, at 2000 m: within 3/255 of
//     the sRGB of (1 − 2^−2) × hue × 3/(4π), its colour not in it;
// 11. the sun plus one `more` light of another colour, at 2000 m: within 3/255
//     of (1 − 2^−2) × hue × the sum of both lights' B;
// 12. the sun at intensity 0, at 2000 m: the black quad black byte for byte,
//     and a white unlit quad within 3/255 of the sRGB of 2^−2 × white (0415
//     point 3);
// 13. a horizon of (0.9, 0.5, 0.2), at 2000 m: within 3/255 of
//     (1 − 2^−2) × that hue × B;
// 14. a black horizon, at 2000 m: within 3/255 of (1 − 2^−2) × white × B;
// 15. the sky not drawn and case 10's orange light, at 2000 m: within 3/255 of
//     (1 − 2^−2) × its colour × 3/(4π), the lights' colour (0415 point 1).
// Cases 1 to 7 and 10 to 15 keep haze_start 0.
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
#define TOLERANCE 3
#define HAZE 1000.0f
#define FAR_QUAD 2000.0f
#define NEAR_QUAD 50.0f

#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 10000.0f

#define FOUR_PI (4.0f * 3.14159265f)

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 3,
	.passes = 1,
	.targets = 1,
};

// Two units square at z = −1, facing the eye: scaled by d it fills the view at
// d metres.
static const voe_render_vertex QUAD_VERTICES[4] = {
	{ { -1.0f, -1.0f, -1.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 1.0f, -1.0f, -1.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { 1.0f, 1.0f, -1.0f }, { 0, 0, 1 }, { 0, 0 } },
	{ { -1.0f, 1.0f, -1.0f }, { 0, 0, 1 }, { 0, 0 } },
};

static const uint32_t QUAD_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

struct scene {
	voe_render_device *device;
	voe_base_arena *arena;
	voe_render_target target;
	voe_render_geometry quad;
	voe_render_shading black;
	voe_render_shading see_through;
	voe_render_shading white;
};

// One picture: the quad's distance and record, the sky, and how it is drawn;
// `light` and `sky` NULL are SUN and SKY, `more` NULL no second light, and
// `white` the white unlit quad in place of the black one.
struct haze_case {
	float distance;
	float haze;
	bool drawn;
	bool blended;
	bool after_clear;
	const voe_render_light *light;
	const voe_render_sky *sky;
	const voe_render_directional_light *more;
	bool white;
};

// sky.c's camera with its far plane past the far quad.
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
static const voe_render_light SUN = {
	.direction = { 0.0f, -0.5f, 0.8660254f },
	.intensity = 1.0f,
	.colour = { 1.0f, 1.0f, 1.0f },
};

// ADR-0406 point 1's colours and shape; `drawn` and `haze` are each case's.
static const voe_render_sky SKY = {
	.zenith = { 0.15f, 0.35f, 0.80f },
	.horizon_height = 0.25f,
	.horizon = { 0.60f, 0.75f, 0.90f },
	.strength = 1.0f,
	.ground = { 0.20f, 0.18f, 0.16f },
	.disc_size = { 1.5f },
	.glow_size = { 0.5f },
};

// One frame of one case onto the target.
static void draw_frame(struct scene *scene, const struct haze_case *with)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = with->light != NULL ? *with->light : SUN,
		.more = { with->more, with->more != NULL ? 1u : 0u },
		.sky = with->sky != NULL ? *with->sky : SKY,
		.exposure = { .tone = VOE_RENDER_TONE_CLIP },
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = with->blended ? scene->see_through.index :
			   with->white ? scene->white.index :
					   scene->black.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	bool drawing = false;

	camera.sky.drawn = with->drawn;
	camera.sky.haze = with->haze;
	for (int i = 0; i < 3; i++)
		object.world.m[i][i] = with->distance;
	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device, scene->target,
					     &camera));
	if (with->after_clear)
		voe_render_frame_clear_depth(scene->device);
	if (with->blended)
		VOE_TEST_CHECK(voe_render_frame_draw_blended(
			scene->device, scene->quad, object));
	else
		VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->quad,
						     object));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
}

// One case drawn and its centre pixel read back; NULL when the read failed.
static const uint8_t *centre_of(struct scene *scene, const struct haze_case *with)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	draw_frame(scene, with);
	VOE_TEST_CHECK(voe_render_target_read(scene->device, scene->target,
					      scene->arena, &picture, &error));
	if (picture.pixels == NULL || picture.width != SIDE ||
	    picture.height != SIDE)
		return NULL;
	return picture.pixels + ((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4;
}

// A linear value as the byte an sRGB target stores for it, clamped as it does.
static int srgb_byte(float linear)
{
	float x = fminf(fmaxf(linear, 0.0f), 1.0f);
	float encoded = x <= 0.0031308f ? x * 12.92f :
					  1.055f * powf(x, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

// The haze's colour one shown light gives without a drawn sky: colour ×
// intensity / (4π), as tone.slangh's voe_render_haze_colour adds it.
static voe_math_float3 haze_of(const voe_render_light *light)
{
	return voe_math_float3_scale(light->colour, light->intensity / FOUR_PI);
}

// One shown light's B: its largest colour channel × intensity / (4π).
static float brightness_of(const voe_render_light *light)
{
	voe_math_float3 c = light->colour;

	return fmaxf(c.x, fmaxf(c.y, c.z)) * light->intensity / FOUR_PI;
}

// hue × `brightness` under drawn `sky`, hue as tone.slangh takes it: the
// gradient along a level view at strength 1 over its largest channel, white
// when that is not above 0.
static voe_math_float3 sky_haze_of(const voe_render_sky *sky, float brightness)
{
	voe_render_sky full = *sky;
	voe_math_float3 horizon;
	float peak;

	full.strength = 1.0f;
	horizon = voe_render_sky_gradient(&full,
					  (voe_math_float3){ 0.0f, 0.0f, -1.0f });
	peak = fmaxf(horizon.x, fmaxf(horizon.y, horizon.z));
	if (!(peak > 0.0f))
		return (voe_math_float3){ brightness, brightness, brightness };
	return voe_math_float3_scale(horizon, brightness / peak);
}

static void check_pixel(const char *name, const uint8_t *pixel,
			voe_math_float3 expected, int tolerance)
{
	printf("%s: %d %d %d, want %d %d %d\n", name, pixel[0], pixel[1],
	       pixel[2], srgb_byte(expected.x), srgb_byte(expected.y),
	       srgb_byte(expected.z));
	VOE_TEST_CHECK(abs(pixel[0] - srgb_byte(expected.x)) <= tolerance);
	VOE_TEST_CHECK(abs(pixel[1] - srgb_byte(expected.y)) <= tolerance);
	VOE_TEST_CHECK(abs(pixel[2] - srgb_byte(expected.z)) <= tolerance);
}

static void check_pictures(struct scene *scene)
{
	voe_render_sky topped = SKY;
	const struct haze_case far = { FAR_QUAD, HAZE, true, false, false,
				       NULL, NULL, NULL, false };
	const struct haze_case near = { NEAR_QUAD, HAZE, true, false, false,
					NULL, NULL, NULL, false };
	const struct haze_case none = { FAR_QUAD, 0.0f, true, false, false,
					NULL, NULL, NULL, false };
	const struct haze_case plain = { FAR_QUAD, HAZE, true, false, true,
					 NULL, NULL, NULL, false };
	const struct haze_case blended = { FAR_QUAD, HAZE, true, true, false,
					   NULL, &topped, NULL, false };
	const voe_math_float3 lit = sky_haze_of(&SKY, brightness_of(&SUN));
	const voe_math_float3 hazed = voe_math_float3_scale(lit, 0.75f);
	const voe_math_float3 black = { 0.0f, 0.0f, 0.0f };
	const uint8_t *pixel;

	topped.haze_top = 1000.0f;
	if ((pixel = centre_of(scene, &far)) != NULL)
		check_pixel("2000 m", pixel, hazed, TOLERANCE);
	if ((pixel = centre_of(scene, &near)) != NULL) {
		voe_math_float3 tenth = voe_math_float3_scale(lit, 0.1f);

		printf("50 m: %d %d %d, under %d %d %d\n", pixel[0], pixel[1],
		       pixel[2], srgb_byte(tenth.x), srgb_byte(tenth.y),
		       srgb_byte(tenth.z));
		VOE_TEST_CHECK(pixel[0] < srgb_byte(tenth.x));
		VOE_TEST_CHECK(pixel[1] < srgb_byte(tenth.y));
		VOE_TEST_CHECK(pixel[2] < srgb_byte(tenth.z));
	}
	if ((pixel = centre_of(scene, &none)) != NULL)
		check_pixel("haze 0", pixel, black, 0);
	if ((pixel = centre_of(scene, &plain)) != NULL)
		check_pixel("after the depth clear", pixel, black, 0);
	if ((pixel = centre_of(scene, &blended)) != NULL)
		check_pixel("blended", pixel,
			    voe_math_float3_add(
				    voe_math_float3_scale(hazed, 0.5f),
				    voe_math_float3_scale(lit, 0.5f)),
			    TOLERANCE);
}

// The sun 0.5° up ahead, its disc reaching the horizon, and 90° round to the
// side: the far quad as case 1 both times, the haze having no view term.
static void check_low_sun(struct scene *scene)
{
	const float e = 0.5f * (3.14159265f / 180.0f);
	const voe_render_light ahead = { .direction = { 0.0f, -sinf(e), cosf(e) },
					 .intensity = 1.0f,
					 .colour = { 1.0f, 1.0f, 1.0f } };
	voe_render_light side = ahead;
	const struct haze_case low = { FAR_QUAD, HAZE, true, false, false,
				       &ahead, NULL, NULL, false };
	const struct haze_case to_side = { FAR_QUAD, HAZE, true, false, false,
					   &side, NULL, NULL, false };
	const voe_math_float3 hazed = voe_math_float3_scale(
		sky_haze_of(&SKY, brightness_of(&SUN)), 0.75f);
	const uint8_t *pixel;

	side.direction = (voe_math_float3){ -cosf(e), -sinf(e), 0.0f };
	if ((pixel = centre_of(scene, &low)) != NULL)
		check_pixel("low sun ahead", pixel, hazed, TOLERANCE);
	if ((pixel = centre_of(scene, &to_side)) != NULL)
		check_pixel("low sun to the side", pixel, hazed, TOLERANCE);
}

// A sky of strength 0.25 and a tenth as bright: the far quad as case 1, the
// sky's strength and brightness not reaching the haze.
static void check_dim_sky(struct scene *scene)
{
	voe_render_sky dim = SKY;
	const struct haze_case far = { FAR_QUAD, HAZE, true, false, false,
				       NULL, &dim, NULL, false };
	const uint8_t *pixel;

	dim.strength = 0.25f;
	dim.zenith = voe_math_float3_scale(SKY.zenith, 0.1f);
	dim.horizon = voe_math_float3_scale(SKY.horizon, 0.1f);
	dim.ground = voe_math_float3_scale(SKY.ground, 0.1f);
	if ((pixel = centre_of(scene, &far)) != NULL)
		check_pixel("strength 0.25, a tenth as bright", pixel,
			    voe_math_float3_scale(
				    sky_haze_of(&SKY, brightness_of(&SUN)),
				    0.75f),
			    TOLERANCE);
}

// Two other horizons: the haze takes the first's hue, and white from a black
// one, at the sun's brightness.
static void check_horizons(struct scene *scene)
{
	voe_render_sky orange = SKY;
	voe_render_sky black = SKY;
	const struct haze_case to_orange = { FAR_QUAD, HAZE, true, false, false,
					     NULL, &orange, NULL, false };
	const struct haze_case to_black = { FAR_QUAD, HAZE, true, false, false,
					    NULL, &black, NULL, false };
	const voe_math_float3 white = { 1.0f, 1.0f, 1.0f };
	const uint8_t *pixel;

	orange.horizon = (voe_math_float3){ 0.9f, 0.5f, 0.2f };
	black.horizon = (voe_math_float3){ 0.0f, 0.0f, 0.0f };
	if ((pixel = centre_of(scene, &to_orange)) != NULL)
		check_pixel("an orange horizon", pixel,
			    voe_math_float3_scale(
				    sky_haze_of(&orange, brightness_of(&SUN)),
				    0.75f),
			    TOLERANCE);
	if ((pixel = centre_of(scene, &to_black)) != NULL)
		check_pixel("a black horizon", pixel,
			    voe_math_float3_scale(white,
						  0.75f * brightness_of(&SUN)),
			    TOLERANCE);
}

// An orange light, the sun with a second light in `more`, and the sun dark:
// under the sky the haze its hue at the lights' brightness summed, nought
// with none shown; with no sky drawn, the orange light's colour.
static void check_lights(struct scene *scene)
{
	const voe_render_light orange = { .direction = SUN.direction,
					  .intensity = 3.0f,
					  .colour = { 1.0f, 0.5f, 0.2f } };
	const voe_render_directional_light blue = {
		.light = { .direction = { -0.8660254f, -0.5f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 0.2f, 0.4f, 1.0f } },
	};
	voe_render_light dark = SUN;
	const struct haze_case lit_orange = { FAR_QUAD, HAZE, true, false,
					      false, &orange, NULL, NULL, false };
	const struct haze_case two = { FAR_QUAD, HAZE, true, false, false,
				       NULL, NULL, &blue, false };
	const struct haze_case none = { FAR_QUAD, HAZE, true, false, false,
					&dark, NULL, NULL, false };
	const struct haze_case white = { FAR_QUAD, HAZE, true, false, false,
					 &dark, NULL, NULL, true };
	const struct haze_case bare = { FAR_QUAD, HAZE, false, false, false,
					&orange, NULL, NULL, false };
	const voe_math_float3 black = { 0.0f, 0.0f, 0.0f };
	const voe_math_float3 quarter = { 0.25f, 0.25f, 0.25f };
	const uint8_t *pixel;

	dark.intensity = 0.0f;
	if ((pixel = centre_of(scene, &lit_orange)) != NULL)
		check_pixel("orange at 3", pixel,
			    voe_math_float3_scale(
				    sky_haze_of(&SKY, brightness_of(&orange)),
				    0.75f),
			    TOLERANCE);
	if ((pixel = centre_of(scene, &two)) != NULL)
		check_pixel("sun and a blue more", pixel,
			    voe_math_float3_scale(
				    sky_haze_of(&SKY,
						brightness_of(&SUN) +
							brightness_of(
								&blue.light)),
				    0.75f),
			    TOLERANCE);
	if ((pixel = centre_of(scene, &none)) != NULL)
		check_pixel("no light, black", pixel, black, 0);
	if ((pixel = centre_of(scene, &white)) != NULL)
		check_pixel("no light, white", pixel, quarter, TOLERANCE);
	if ((pixel = centre_of(scene, &bare)) != NULL)
		check_pixel("orange at 3, no sky drawn", pixel,
			    voe_math_float3_scale(haze_of(&orange), 0.75f),
			    TOLERANCE);
}

// A haze that starts at a distance: nearer surfaces untouched, farther ones
// hazed by their distance past the start.
static void check_haze_start(struct scene *scene)
{
	voe_render_sky near_start = SKY;
	voe_render_sky far_start = SKY;
	const struct haze_case inside = { NEAR_QUAD, HAZE, true, false, false,
					  NULL, &near_start, NULL, false };
	const struct haze_case past = { FAR_QUAD, HAZE, true, false, false,
					NULL, &far_start, NULL, false };
	const voe_math_float3 black = { 0.0f, 0.0f, 0.0f };
	const uint8_t *pixel;

	near_start.haze_start = 100.0f;
	far_start.haze_start = 1000.0f;
	if ((pixel = centre_of(scene, &inside)) != NULL)
		check_pixel("haze_start 100 m at 50 m", pixel, black, 0);
	if ((pixel = centre_of(scene, &past)) != NULL)
		check_pixel("haze_start 1000 m at 2000 m", pixel,
			    voe_math_float3_scale(
				    sky_haze_of(&SKY, brightness_of(&SUN)),
				    0.5f),
			    TOLERANCE);
}

int main(void)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { .arena = voe_base_arena_new(1024 * 1024) };
	voe_render_texture shown = { 0 };
	voe_render_shading_values black = {
		.base_colour = { 0.0f, 0.0f, 0.0f, 1.0f },
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_shading_values see_through = black;
	voe_render_shading_values white = black;

	see_through.base_colour.w = 0.5f;
	white.base_colour = (voe_math_float4){ 1.0f, 1.0f, 1.0f, 1.0f };
	see_through.alpha_mode = VOE_RENDER_ALPHA_BLENDED;
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
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, black,
						 &scene.black, &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, see_through,
						 &scene.see_through, &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, white,
						 &scene.white, &error));

	check_pictures(&scene);
	check_low_sun(&scene);
	check_dim_sky(&scene);
	check_haze_start(&scene);
	check_lights(&scene);
	check_horizons(&scene);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
