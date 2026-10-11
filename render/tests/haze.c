// FAR SURFACES FADE INTO HAZE (ADR-0405 point 6). A target of the test's own
// seen by a camera at the origin looking along −Z, a white sun 30° up straight
// ahead, haze 1000 m, exposure 0 and Clip, read back with voe_render_target_read
// at the centre pixel. The quad is an unlit black square filling the view at a
// distance, scaled by its object matrix:
//
// 1. at 2000 m: within 3/255 of the sRGB of (1 − 2^−2) × the horizon's radiance
//    along the pixel's direction, from voe_render_sky_radiance (../src/sky.h);
// 2. at 50 m: darker than a tenth of that radiance;
// 3. haze 0 at 2000 m: black, byte for byte;
// 4. drawn after the pass's depth clear at 2000 m: black, as authored;
// 5. a blended black quad of alpha 0.5 at 2000 m over the drawn sky: half its
//    hazed colour plus half the sky along the pixel, within 3/255.
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

static const voe_render_capacities CAPACITIES = {
	.vertices = 4,
	.indices = 6,
	.geometries = 1,
	.objects = 1,
	.shadings = 2,
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
};

// One picture: the quad's distance and record, the sky, and how it is drawn.
struct haze_case {
	float distance;
	float haze;
	bool drawn;
	bool blended;
	bool after_clear;
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

// One frame of one case onto the target.
static void draw_frame(struct scene *scene, const struct haze_case *with)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = SUN,
		.sky = { .drawn = with->drawn, .haze = with->haze,
			 .tone = VOE_RENDER_TONE_CLIP },
	};
	voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = with->blended ? scene->see_through.index :
					   scene->black.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	bool drawing = false;

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

// The world direction through the centre pixel, (SIDE/2, SIDE/2)'s centre.
static voe_math_float3 centre_direction(void)
{
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float x = 2.0f * ((float)(SIDE / 2) + 0.5f) / SIDE - 1.0f;
	float y = 1.0f - 2.0f * ((float)(SIDE / 2) + 0.5f) / SIDE;

	return voe_math_float3_normalize(
		(voe_math_float3){ x / focal, y / focal, -1.0f });
}

static voe_math_float3 radiance_along(voe_math_float3 view)
{
	return voe_render_sky_radiance(SUN.direction,
				       voe_math_float3_scale(SUN.colour,
							     SUN.intensity),
				       view);
}

// The haze's colour at the centre: the sky along it, its height clamped up to
// the horizon, as tone.slangh's voe_render_haze_colour.
static voe_math_float3 horizon_radiance(void)
{
	voe_math_float3 view = centre_direction();

	view.y = fmaxf(view.y, 0.0f);
	return radiance_along(voe_math_float3_normalize(view));
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
	const struct haze_case far = { FAR_QUAD, HAZE, false, false, false };
	const struct haze_case near = { NEAR_QUAD, HAZE, false, false, false };
	const struct haze_case none = { FAR_QUAD, 0.0f, false, false, false };
	const struct haze_case plain = { FAR_QUAD, HAZE, false, false, true };
	const struct haze_case blended = { FAR_QUAD, HAZE, true, true, false };
	const voe_math_float3 horizon = horizon_radiance();
	const voe_math_float3 hazed = voe_math_float3_scale(horizon, 0.75f);
	const voe_math_float3 black = { 0.0f, 0.0f, 0.0f };
	const uint8_t *pixel;

	if ((pixel = centre_of(scene, &far)) != NULL)
		check_pixel("2000 m", pixel, hazed, TOLERANCE);
	if ((pixel = centre_of(scene, &near)) != NULL) {
		voe_math_float3 tenth = voe_math_float3_scale(horizon, 0.1f);

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
				    voe_math_float3_scale(
					    radiance_along(centre_direction()),
					    0.5f)),
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

	see_through.base_colour.w = 0.5f;
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

	check_pictures(&scene);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
