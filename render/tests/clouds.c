// THE CLOUDS ARE DRAWN OVER THE WORLD (ADR-0492 point 5):
// voe_render_frame_draw_clouds after the world's draws. A target of the test's
// own seen by a camera at the origin looking straight up, ADR-0406 point 1's
// sky drawn, one white sun high in the south, exposure 0 and Clip, read back
// with voe_render_target_read; `none` is that picture with no call at all.
//
// 1. kind none: the same bytes as `none`, and the breakdown, some frames
//    later, lists no `clouds` span;
// 2. Volumetric, coverage 1, bottom 500, thickness 500: the centre pixel more
//    than 8/255 off `none`'s, and the breakdown lists `<pass>: clouds` after
//    the pass (a card that writes no timestamps skips the breakdown);
// 3. the same at coverage 0: every pixel within 1/255 of `none`;
// 4. Flat, coverage 1, bottom 500: the centre changed;
// 5. an unlit opaque quad 10 m above the eye filling the view, under case 2's
//    clouds: every pixel within 1/255 of the quad with kind none;
// 6. the eye at 700 m, inside case 2's layer (bottom −200): the centre changed.
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

// The sun 60° up in the south; its light travels away from it.
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

// The eye at the origin looking up +Y, screen right +X and screen up +Z,
// reverse-Z.
static voe_render_view looking_up(void)
{
	voe_render_view view = { .view = voe_math_float4x4_identity() };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	view.view.m[1][1] = 0.0f;
	view.view.m[1][2] = 1.0f;
	view.view.m[2][1] = -1.0f;
	view.view.m[2][2] = 0.0f;
	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

static voe_render_clouds clouds_of(uint32_t kind, float coverage, float bottom)
{
	return (voe_render_clouds){
		.kind = kind,
		.coverage = coverage,
		.bottom = bottom,
		.thickness = 500.0f,
		.origin = { 100.0f, 100.0f },
	};
}

// One frame of one pass under `clouds`, the quad drawn when `quad`, the call
// made when `call`.
static void draw_frame(struct scene *scene, voe_render_clouds clouds,
		       bool quad, bool call)
{
	voe_platform_size size = { SIDE, SIDE };
	const voe_render_pass_camera camera = {
		.view = looking_up(),
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

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device, scene->target,
					     &camera));
	if (quad)
		VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->quad,
						     object));
	if (call)
		VOE_TEST_CHECK(voe_render_frame_draw_clouds(scene->device));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
}

// That frame read back; NULL when the read failed.
static const uint8_t *picture_of(struct scene *scene, voe_render_clouds clouds,
				 bool quad, bool call)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	draw_frame(scene, clouds, quad, call);
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

// Whether the newest measured frame lists a span ending `: clouds` right after
// the target's pass, after FRAMES frames of `clouds`; skipped without
// timestamps.
static void check_breakdown(struct scene *scene, voe_render_clouds clouds,
			    bool listed)
{
	voe_render_pass_time times[4];
	char span[VOE_RENDER_PASS_NAME + 16];
	double seconds = 0.0;
	uint32_t count;
	bool found = false;

	for (int i = 0; i < FRAMES; i++)
		draw_frame(scene, clouds, false, true);
	if (!voe_render_frame_gpu_time(scene->device, &seconds)) {
		printf("skip: this card writes no timestamps, so there is no breakdown\n");
		return;
	}
	(void)snprintf(span, sizeof(span), "view target %u: clouds",
		       scene->target.index);
	count = voe_render_frame_pass_times(scene->device, times, 4);
	for (uint32_t i = 0; i < count; i++) {
		printf("breakdown %u: %s\n", i, times[i].name);
		found |= strcmp(times[i].name, span) == 0;
	}
	VOE_TEST_CHECK(found == listed);
}

static void check_clouds(struct scene *scene)
{
	const voe_render_clouds none_kind = clouds_of(VOE_RENDER_CLOUDS_NONE,
						      1.0f, 500.0f);
	const voe_render_clouds full =
		clouds_of(VOE_RENDER_CLOUDS_VOLUMETRIC, 1.0f, 500.0f);
	const uint8_t *none = picture_of(scene, none_kind, false, false);
	const uint8_t *quad_none = picture_of(scene, none_kind, true, false);

	check_all_same("kind none", picture_of(scene, none_kind, false, true),
		       none, 0);
	check_breakdown(scene, none_kind, false);
	check_centre_changed("volumetric",
			     picture_of(scene, full, false, true), none);
	check_breakdown(scene, full, true);
	check_all_same("coverage 0",
		       picture_of(scene,
				  clouds_of(VOE_RENDER_CLOUDS_VOLUMETRIC, 0.0f,
					    500.0f),
				  false, true),
		       none, SAME);
	check_centre_changed("flat",
			     picture_of(scene,
					clouds_of(VOE_RENDER_CLOUDS_FLAT, 1.0f,
						  500.0f),
					false, true),
			     none);
	check_all_same("a quad under the clouds",
		       picture_of(scene, full, true, true), quad_none, SAME);
	check_centre_changed("inside the layer",
			     picture_of(scene,
					clouds_of(VOE_RENDER_CLOUDS_VOLUMETRIC,
						  1.0f, -200.0f),
					false, true),
			     none);
}

// An unlit grey quad 10 m above the eye, 40 m a side, facing down.
static bool build_scene(struct scene *scene)
{
	const voe_render_vertex vertices[4] = {
		{ { -20.0f, 10.0f, -20.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 0 } },
		{ { 20.0f, 10.0f, -20.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0 } },
		{ { 20.0f, 10.0f, 20.0f }, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 0 } },
		{ { -20.0f, 10.0f, 20.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0 } },
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
		check_clouds(&scene);
	else
		VOE_TEST_CHECK(false);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(scene.arena);
	return voe_test_result();
}
