// THE FOG IS DRAWN OVER THE WORLD AFTER THE CLOUDS (ADR-0493 point 5):
// voe_render_frame_draw_fog after the world's draws. A target of the test's
// own seen by a camera at the origin looking along −Z, ADR-0406 point 1's sky
// drawn, one white sun high behind the eye, exposure 0 and Clip, read back
// with voe_render_target_read; `none` is that picture with no call at all.
// The volume is a box 20 m a side centred 30 m ahead, density 0.05,
// patchiness 0, godrays 0.
//
// 1. count 0: the same bytes as `none`, and the breakdown, some frames later,
//    lists no `fog` span;
// 2. the volume: the centre pixel more than 8/255 off `none`'s, and the
//    breakdown lists `<pass>: fog` (a card that writes no timestamps skips
//    the breakdown);
// 3. the same at density 0: every pixel within 1/255 of `none`;
// 4. the corner pixel, whose view misses the box: within 1/255 of `none`;
// 5. an unlit opaque quad 5 m ahead covering the centre: its middle pixels
//    within 1/255 of the quad with no call;
// 6. the box centred on the eye: the centre changed;
// 7. Volumetric clouds at coverage 1 and the volume in one pass: the centre
//    more than 8/255 off the clouds alone, some pixel more than 8/255 off the
//    fog alone, and the breakdown listing both spans.
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
#define FRAMES 4
#define CHANGED 8
#define SAME 1
// The middle square of the picture the quad covers whole, in pixels a side.
#define MIDDLE 16

#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// ADR-0406 point 1's defaults, drawn.
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

// The sun 60° up behind the eye; its light travels away from it, towards −Z.
static const voe_render_light SUN = {
	.direction = { 0.0f, -0.8660254f, -0.5f },
	.intensity = 3.0f,
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
	voe_render_geometry quad;
	voe_render_shading grey;
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

// One volume 20 m a side, unturned, centred `ahead` metres along −Z of the
// eye, at `density`; `count` 0 is no fog.
static voe_render_fog fog_of(uint32_t count, float ahead, float density)
{
	const float half = 10.0f;
	voe_render_fog fog = { .count = count };

	fog.volumes[0] = (voe_render_fog_volume){
		.rows = {
			{ 1.0f / half, 0.0f, 0.0f, 0.0f },
			{ 0.0f, 1.0f / half, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 1.0f / half, ahead / half },
		},
		.patches = { 1.0f, 1.0f, 1.0f },
		.density = density,
		.change = { 1.0f, 1.0f, 1.0f },
		.patchiness = 0.0f,
		.godrays = 0.0f,
	};
	return fog;
}

static const voe_render_clouds NO_CLOUDS = { .kind = VOE_RENDER_CLOUDS_NONE };

static const voe_render_clouds FULL_CLOUDS = {
	.kind = VOE_RENDER_CLOUDS_VOLUMETRIC,
	.coverage = 1.0f,
	.bottom = 100.0f,
	.thickness = 500.0f,
	.origin = { 100.0f, 100.0f },
};

// One frame of one pass under `clouds` and `fog`, the quad drawn when `quad`,
// the clouds' call made when they have a kind and the fog's when `call`.
static void draw_frame(struct scene *scene, voe_render_clouds clouds,
		       const voe_render_fog *fog, bool quad, bool call)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_pass_camera camera = {
		.view = looking_ahead(),
		.light = SUN,
		.sky = DRAWN,
		.exposure = { .tone = VOE_RENDER_TONE_CLIP },
		.clouds = clouds,
	};
	const voe_render_object object = {
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = scene->grey.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
	bool drawing = false;

	camera.fog = *fog;
	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device, scene->target,
					     &camera));
	if (quad)
		VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->quad,
						     object));
	if (clouds.kind != VOE_RENDER_CLOUDS_NONE)
		VOE_TEST_CHECK(voe_render_frame_draw_clouds(scene->device));
	if (call)
		VOE_TEST_CHECK(voe_render_frame_draw_fog(scene->device));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
}

// That frame read back; NULL when the read failed.
static const uint8_t *picture_of(struct scene *scene, voe_render_clouds clouds,
				 voe_render_fog fog, bool quad, bool call)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	draw_frame(scene, clouds, &fog, quad, call);
	VOE_TEST_CHECK(voe_render_target_read(scene->device, scene->target,
					      scene->arena, &picture, &error));
	if (picture.pixels == NULL || picture.width != SIDE ||
	    picture.height != SIDE)
		return NULL;
	return picture.pixels;
}

// The largest channel difference between `a` and `b` over `count` bytes.
static int largest_difference(const uint8_t *a, const uint8_t *b, int count)
{
	int largest = 0;

	for (int i = 0; i < count; i++)
		if ((i & 3) != 3 && abs(a[i] - b[i]) > largest)
			largest = abs(a[i] - b[i]);
	return largest;
}

// The largest channel difference over the middle MIDDLE × MIDDLE pixels.
static int middle_difference(const uint8_t *a, const uint8_t *b)
{
	const int first = (SIDE - MIDDLE) / 2;
	int largest = 0;

	for (int y = first; y < first + MIDDLE; y++) {
		const size_t row = ((size_t)y * SIDE + (size_t)first) * 4;
		const int difference =
			largest_difference(a + row, b + row, MIDDLE * 4);

		largest = difference > largest ? difference : largest;
	}
	return largest;
}

static const uint8_t *centre(const uint8_t *pixels)
{
	return pixels + ((size_t)(SIDE / 2) * SIDE + SIDE / 2) * 4;
}

static void check_centre_changed(const char *name, const uint8_t *pixels,
				 const uint8_t *none)
{
	if (pixels == NULL || none == NULL)
		return;
	printf("%s: centre %d %d %d, none %d %d %d\n", name, centre(pixels)[0],
	       centre(pixels)[1], centre(pixels)[2], centre(none)[0],
	       centre(none)[1], centre(none)[2]);
	VOE_TEST_CHECK(largest_difference(centre(pixels), centre(none), 4) >
		       CHANGED);
}

static void check_all_same(const char *name, const uint8_t *pixels,
			   const uint8_t *none, int within)
{
	if (pixels == NULL || none == NULL)
		return;
	printf("%s: largest difference %d\n", name,
	       largest_difference(pixels, none, PIXELS));
	VOE_TEST_CHECK(largest_difference(pixels, none, PIXELS) <= within);
}

// Whether the newest measured frame lists `<pass>: clouds` and `<pass>: fog`
// as `clouds` and `fog` say, after FRAMES frames under them; skipped without
// timestamps.
static void check_breakdown(struct scene *scene, voe_render_clouds clouds,
			    voe_render_fog fog, bool clouds_listed,
			    bool fog_listed)
{
	voe_render_pass_time times[8];
	char clouds_span[VOE_RENDER_PASS_NAME + 16];
	char fog_span[VOE_RENDER_PASS_NAME + 16];
	double seconds = 0.0;
	uint32_t count;
	bool clouds_found = false;
	bool fog_found = false;

	for (int i = 0; i < FRAMES; i++)
		draw_frame(scene, clouds, &fog, false, true);
	if (!voe_render_frame_gpu_time(scene->device, &seconds)) {
		printf("skip: this card writes no timestamps, so there is no breakdown\n");
		return;
	}
	(void)snprintf(clouds_span, sizeof(clouds_span),
		       "view target %u: clouds", scene->target.index);
	(void)snprintf(fog_span, sizeof(fog_span), "view target %u: fog",
		       scene->target.index);
	count = voe_render_frame_pass_times(scene->device, times, 8);
	for (uint32_t i = 0; i < count; i++) {
		printf("breakdown %u: %s\n", i, times[i].name);
		clouds_found |= strcmp(times[i].name, clouds_span) == 0;
		fog_found |= strcmp(times[i].name, fog_span) == 0;
	}
	VOE_TEST_CHECK(clouds_found == clouds_listed);
	VOE_TEST_CHECK(fog_found == fog_listed);
}

// Case 7: the clouds and the fog in one pass, each seen in the picture.
static void check_both(struct scene *scene, voe_render_fog fog)
{
	const uint8_t *clouds = picture_of(scene, FULL_CLOUDS, fog_of(0, 30.0f,
								      0.05f),
					   false, true);
	const uint8_t *fogged = picture_of(scene, NO_CLOUDS, fog, false, true);
	const uint8_t *both = picture_of(scene, FULL_CLOUDS, fog, false, true);

	check_centre_changed("both against the clouds alone", both, clouds);
	if (both != NULL && fogged != NULL) {
		printf("both against the fog alone: largest difference %d\n",
		       largest_difference(both, fogged, PIXELS));
		VOE_TEST_CHECK(largest_difference(both, fogged, PIXELS) >
			       CHANGED);
	}
	check_breakdown(scene, FULL_CLOUDS, fog, true, true);
}

static void check_fog(struct scene *scene)
{
	const voe_render_fog volume = fog_of(1, 30.0f, 0.05f);
	const uint8_t *none = picture_of(scene, NO_CLOUDS, volume, false, false);
	const uint8_t *quad_none = picture_of(scene, NO_CLOUDS, volume, true,
					      false);
	const uint8_t *fogged = picture_of(scene, NO_CLOUDS, volume, false, true);
	const uint8_t *quad = picture_of(scene, NO_CLOUDS, volume, true, true);

	check_all_same("count 0",
		       picture_of(scene, NO_CLOUDS, fog_of(0, 30.0f, 0.05f),
				  false, true),
		       none, 0);
	check_breakdown(scene, NO_CLOUDS, fog_of(0, 30.0f, 0.05f), false,
			false);
	check_centre_changed("the volume", fogged, none);
	check_breakdown(scene, NO_CLOUDS, volume, false, true);
	check_all_same("density 0",
		       picture_of(scene, NO_CLOUDS, fog_of(1, 30.0f, 0.0f),
				  false, true),
		       none, SAME);
	if (fogged != NULL && none != NULL) {
		printf("the corner: largest difference %d\n",
		       largest_difference(fogged, none, 4));
		VOE_TEST_CHECK(largest_difference(fogged, none, 4) <= SAME);
	}
	if (quad != NULL && quad_none != NULL) {
		printf("a quad before the volume: largest difference %d\n",
		       middle_difference(quad, quad_none));
		VOE_TEST_CHECK(middle_difference(quad, quad_none) <= SAME);
	}
	check_centre_changed("the eye inside",
			     picture_of(scene, NO_CLOUDS,
					fog_of(1, 0.0f, 0.05f), false, true),
			     none);
	check_both(scene, volume);
}

// An unlit grey quad 5 m ahead, 4 m a side, facing the eye.
static bool build_scene(struct scene *scene)
{
	const voe_render_vertex vertices[4] = {
		{ { -2.0f, -2.0f, -5.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { 2.0f, -2.0f, -5.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 0 } },
		{ { 2.0f, 2.0f, -5.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0 } },
		{ { -2.0f, 2.0f, -5.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 0 } },
	};
	const uint32_t indices[6] = { 0, 1, 2, 0, 2, 3 };
	const voe_render_shading_values grey = {
		.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
		.roughness = 1.0f,
		.unlit = 1,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	voe_render_texture shown = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool built = true;

	built &= voe_render_target_create(scene->device, SIDE, SIDE,
					  &scene->target, &shown, &error);
	built &= voe_render_geometry_create(scene->device, vertices, 4, indices,
					    6, &scene->quad, &error);
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
	if (build_scene(&scene))
		check_fog(&scene);
	else
		VOE_TEST_CHECK(false);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
