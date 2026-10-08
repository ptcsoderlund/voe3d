// A blocker's kind gates the bounce as it gates direct light (ADR-0350 point
// 5), read in the picture. Headless; built and settled as blocked_bounce.c
// does, whose frame this repeats: the sun's cascade, a bounce begin about the
// origin at spacing 2 (lowest cell (−12, −6, −12)), capture passes until one
// does not open, the bounce shadow pass, the relight, then a camera pass read
// back. The same blockers go to the camera pass and the bounce begin.
//
// Grey ground (0.5) everywhere, a red wall (0.6) 0.5 × 4 × 12 m across x = 0,
// a sun along (0.6, −0.8, 0) lighting its −x side, sun 2, fill 0.1, the sun
// bouncing once. The patch at (−3, 0, 0) reads red with no blocker.
// 1. A Wall slab at x −2.6 to −2.4, y −1 to 2, z ±8, between the red wall and
//    the patch: low enough that the sun still lights the red wall over it, so
//    only the probe segments are crossed. The patch within 2/255 of bounces 0;
//    ground at (−0.75, 0, 3), on the red wall's side and still sunlit, red.
// 2. An Indoors box round the patch (x −5 to −1, y ±1.5, z ±2) instead: the
//    patch within 2/255 of the picture with no blocker.
// 3. The same box, the three words zero, a Room: the patch within 2/255 of
//    bounces 0, as blocked_bounce.c's blocked patch.
//
// A card without shaderOutputLayer bounces nothing, and that is said. A machine
// with no usable Vulkan skips and says so.
#include "../src/device_internal.h"

#include <base/arena.h>
#include <base/error.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIDE 128
#define SHADOW_SIDE 2048
#define FRAMES_MAX 200
#define LIGHT_HALF 30.0f
#define VOLUME_HALF 60.0f
#define LIGHT_DISTANCE 60.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 120.0f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 200.0f
#define NARROW 1.0471976f
#define TOLERANCE 2

enum { GREY, RED, SHADINGS };

static const voe_render_capacities CAPACITIES = {
	.vertices = 24,
	.indices = 36,
	.geometries = 1,
	.objects = 128,
	.shadings = SHADINGS,
	.passes = 8,
	.shadow_size = SHADOW_SIDE,
};

static const voe_math_float3 SUN_DIRECTION = { 0.6f, -0.8f, 0.0f };

// Counter-clockwise from outside, four vertices a face, a unit cube.
#define H 0.5f
static const voe_render_vertex CUBE_VERTICES[24] = {
	{ { -H, H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, -H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { -H, -H, H }, { 0, 0, 1 }, { 0, 0 } },
	{ { H, H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { -H, H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { -H, -H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { H, -H, -H }, { 0, 0, -1 }, { 0, 0 } },
	{ { H, H, H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, H, -H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, -H, -H }, { 1, 0, 0 }, { 0, 0 } },
	{ { H, -H, H }, { 1, 0, 0 }, { 0, 0 } },
	{ { -H, H, -H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, H, H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, -H, H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, -H, -H }, { -1, 0, 0 }, { 0, 0 } },
	{ { -H, H, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, H, -H }, { 0, 1, 0 }, { 0, 0 } },
	{ { H, H, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, H, H }, { 0, 1, 0 }, { 0, 0 } },
	{ { -H, -H, H }, { 0, -1, 0 }, { 0, 0 } },
	{ { H, -H, H }, { 0, -1, 0 }, { 0, 0 } },
	{ { H, -H, -H }, { 0, -1, 0 }, { 0, 0 } },
	{ { -H, -H, -H }, { 0, -1, 0 }, { 0, 0 } },
};

static const uint32_t CUBE_INDICES[36] = {
	3, 2, 1, 3, 1, 0,	 7, 6, 5, 7, 5, 4,
	11, 10, 9, 11, 9, 8,	 15, 14, 13, 15, 13, 12,
	19, 18, 17, 19, 17, 16,	 23, 22, 21, 23, 21, 20,
};

// One box of the scene: its centre, its size and its shading.
struct box {
	voe_math_float3 at;
	voe_math_float3 size;
	uint32_t shading;
};

static const struct box WALL_SCENE[] = {
	{ { 0.0f, -0.05f, 0.0f }, { 100.0f, 0.1f, 100.0f }, GREY },
	{ { 0.0f, 2.0f, 0.0f }, { 0.5f, 4.0f, 12.0f }, RED },
};

struct scene {
	voe_render_device *device;
	voe_base_arena *arena;
	voe_render_geometry cube;
	voe_render_shading shadings[SHADINGS];
	voe_render_view light;
	voe_render_view volume_light;
	voe_render_pass_camera camera;
	struct voe_render_bounce_frame bounce;
	bool restale;
};

static const voe_math_float4 EVERYTHING = { 0.0f, 0.0f, 0.0f, 1000.0f };

// A view with rows x, y, z and the eye at `eye`.
static voe_render_view view_from(voe_math_float3 eye, voe_math_float3 z)
{
	const voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x = voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_math_float3 rows[3] = { x, voe_math_float3_cross(z, x), z };
	voe_render_view view = { .eye = eye };

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	view.view.m[3][3] = 1.0f;
	return view;
}

// A camera at `eye` looking at `target`, reverse-Z perspective over `fov`.
static voe_render_view look_at(voe_math_float3 eye, voe_math_float3 target,
			       float fov)
{
	voe_render_view view = view_from(
		eye, voe_math_float3_normalize(voe_math_float3_sub(eye, target)));
	float focal = 1.0f / tanf(fov * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The sun's view at `at`: from LIGHT_DISTANCE back along it, orthographic over
// `half` either side, reverse-Z.
static voe_render_view sun_view(voe_math_float3 at, float half)
{
	voe_render_view light = view_from(
		voe_math_float3_add(at, voe_math_float3_scale(SUN_DIRECTION,
							       -LIGHT_DISTANCE)),
		voe_math_float3_scale(SUN_DIRECTION, -1.0f));
	float span = LIGHT_FAR - LIGHT_NEAR;

	light.projection.m[0][0] = 1.0f / half;
	light.projection.m[1][1] = 1.0f / half;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = LIGHT_FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

// An unturned box of centre `c` and half sizes `h`: rows (a_i / h_i,
// −a_i·c / h_i) and a sphere of radius |h|.
static voe_render_light_blocker blocker(voe_math_float3 c, voe_math_float3 h)
{
	return (voe_render_light_blocker){
		.rows = {
			{ 1.0f / h.x, 0.0f, 0.0f, -c.x / h.x },
			{ 0.0f, 1.0f / h.y, 0.0f, -c.y / h.y },
			{ 0.0f, 0.0f, 1.0f / h.z, -c.z / h.z },
		},
		.sphere = { c.x, c.y, c.z, sqrtf(voe_math_float3_dot(h, h)) },
	};
}

// The same blockers to the camera pass and the bounce begin.
static void block_with(struct scene *s, voe_render_light_blockers blockers)
{
	s->camera.blockers = blockers;
	s->bounce.blockers = blockers;
}

static void draw_boxes(struct scene *s)
{
	for (uint32_t i = 0; i < 2; i++) {
		const struct box *b = &WALL_SCENE[i];
		voe_render_object object = {
			.world = voe_math_float4x4_mul(
				voe_math_float4x4_from_translation(b->at),
				voe_math_float4x4_from_scale(b->size)),
			.normal = voe_math_float4x4_from_scale((voe_math_float3){
				1.0f / b->size.x, 1.0f / b->size.y,
				1.0f / b->size.z }),
			.shading = s->shadings[b->shading].index,
			.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		};

		VOE_TEST_CHECK(voe_render_frame_draw(s->device, s->cube, object));
	}
}

// One frame of the header's; the capture passes it opened. The picture goes
// into `picture` when it is not NULL.
static uint32_t one_frame(struct scene *s, voe_render_picture *picture)
{
	struct voe_render_bounce_frame bounce = s->bounce;
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	bool opened = true;
	uint32_t passes = 0;

	VOE_TEST_CHECK(voe_render_frame_begin(s->device,
					      (voe_platform_size){ SIDE, SIDE },
					      &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return 0;
	VOE_TEST_CHECK(voe_render_shadow_pass_begin(s->device, 0, &s->light));
	draw_boxes(s);
	voe_render_pass_end(s->device);
	if (s->restale && s->device->window_volume[0].built) {
		bounce.stale = &EVERYTHING;
		bounce.stale_count = 1;
		s->restale = false;
	}
	voe_render_bounce_begin(s->device, VOE_RENDER_TARGET_WINDOW, &bounce);
	while (opened) {
		VOE_TEST_CHECK(voe_render_bounce_capture_pass_begin(s->device,
								    &opened));
		if (!opened)
			break;
		draw_boxes(s);
		voe_render_pass_end(s->device);
		passes++;
	}
	VOE_TEST_CHECK(voe_render_bounce_shadow_pass_begin(
		s->device, 0, &s->volume_light, &opened));
	if (opened) {
		draw_boxes(s);
		voe_render_pass_end(s->device);
	}
	voe_render_bounce_relight(s->device);
	VOE_TEST_CHECK(voe_render_pass_begin(s->device, VOE_RENDER_TARGET_WINDOW,
					     &s->camera));
	draw_boxes(s);
	voe_render_pass_end(s->device);
	VOE_TEST_CHECK(voe_render_frame_end(s->device));
	if (picture != NULL)
		VOE_TEST_CHECK(voe_render_target_read(s->device,
						      VOE_RENDER_TARGET_WINDOW,
						      s->arena, picture, &error));
	return passes;
}

// Every probe captured: frames until one opens no capture pass, past the
// first, which may only want the volume.
static void settle(struct scene *s)
{
	uint32_t frame = 0;

	s->restale = true;
	for (; frame < FRAMES_MAX; frame++)
		if (one_frame(s, NULL) == 0 && frame > 0)
			break;
	printf("settled in %u frames\n", frame);
	VOE_TEST_CHECK(frame < FRAMES_MAX);
}

// A picture with the scene's lights and blockers as they now are.
static voe_render_picture picture_of(struct scene *s)
{
	voe_render_picture picture = { 0 };

	VOE_TEST_CHECK_INT(one_frame(s, &picture), 0);
	return picture;
}

// A picture with the sun not bouncing, the blockers as they now are.
static voe_render_picture unbounced_picture_of(struct scene *s)
{
	voe_render_picture picture;

	s->bounce.sun_bounces = 0;
	picture = picture_of(s);
	s->bounce.sun_bounces = 1;
	return picture;
}

// The pixel `world` lands on, or NULL off the picture: clip +y is row 0.
static const uint8_t *pixel_at(const voe_render_picture *picture,
			       const voe_render_view *camera,
			       voe_math_float3 world)
{
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(camera->projection, camera->view),
		(voe_math_float4){ world.x, world.y, world.z, 1.0f });
	float column = (clip.x / clip.w * 0.5f + 0.5f) * (float)picture->width;
	float row = (0.5f - clip.y / clip.w * 0.5f) * (float)picture->height;
	bool on = picture->pixels != NULL && clip.w > 0.0f && column >= 0.0f &&
		  row >= 0.0f && column < (float)picture->width &&
		  row < (float)picture->height;

	VOE_TEST_CHECK(on);
	if (!on)
		return NULL;
	return &picture->pixels[((size_t)row * picture->width + (size_t)column) *
				4];
}

// The pixel at `world` in `picture` as three ints, -1000 off the picture.
static void rgb_at(const struct scene *s, const voe_render_picture *picture,
		   voe_math_float3 world, int out[3])
{
	const uint8_t *p = pixel_at(picture, &s->camera.view, world);

	for (int c = 0; c < 3; c++)
		out[c] = p != NULL ? p[c] : -1000;
}

// The largest channel difference at `world` between two pictures.
static int gap_at(const struct scene *s, const voe_render_picture *a,
		  const voe_render_picture *b, voe_math_float3 world)
{
	int x[3];
	int y[3];
	int gap = 0;

	rgb_at(s, a, world, x);
	rgb_at(s, b, world, y);
	for (int c = 0; c < 3; c++)
		gap = abs(x[c] - y[c]) > gap ? abs(x[c] - y[c]) : gap;
	return gap;
}

static int redness_at(const struct scene *s, const voe_render_picture *p,
		      voe_math_float3 world)
{
	int rgb[3];

	rgb_at(s, p, world, rgb);
	return rgb[0] - rgb[1];
}

static const voe_math_float3 PATCH = { -3.0f, 0.0f, 0.0f };

// The header's three cases against the picture with no blocker.
static void kinds_on_the_patch(struct scene *s)
{
	const voe_math_float3 red_side = { -0.75f, 0.0f, 3.0f };
	const voe_render_light_blocker slab[1] = {
		blocker((voe_math_float3){ -2.5f, 0.5f, 0.0f },
			(voe_math_float3){ 0.1f, 1.5f, 8.0f }),
	};
	const voe_render_light_blocker patch[1] = {
		blocker(PATCH, (voe_math_float3){ 2.0f, 1.5f, 2.0f }),
	};
	voe_render_picture plain, wall, wall_off, indoors, room, room_off;

	s->camera.light.intensity = 2.0f;
	s->camera.light.fill = (voe_math_float3){ 0.1f, 0.1f, 0.1f };
	s->bounce.sun = s->camera.light;
	s->bounce.sun_bounces = 1;
	s->bounce.sun_strength = 1.0f;
	settle(s);
	plain = picture_of(s);
	block_with(s, (voe_render_light_blockers){ .blockers = slab, .count = 1,
						   .walls = 1 });
	wall = picture_of(s);
	wall_off = unbounced_picture_of(s);
	block_with(s, (voe_render_light_blockers){ .blockers = patch, .count = 1,
						   .indoors = 1 });
	indoors = picture_of(s);
	block_with(s, (voe_render_light_blockers){ .blockers = patch, .count = 1 });
	room = picture_of(s);
	room_off = unbounced_picture_of(s);

	printf("patch redness %d unblocked; Wall: gap to bounces 0 %d, red side redness %d; Indoors: gap to unblocked %d; Room: gap to bounces 0 %d\n",
	       redness_at(s, &plain, PATCH), gap_at(s, &wall, &wall_off, PATCH),
	       redness_at(s, &wall, red_side), gap_at(s, &indoors, &plain, PATCH),
	       gap_at(s, &room, &room_off, PATCH));
	VOE_TEST_CHECK(redness_at(s, &plain, PATCH) > TOLERANCE);
	VOE_TEST_CHECK(gap_at(s, &wall, &wall_off, PATCH) <= TOLERANCE);
	VOE_TEST_CHECK(redness_at(s, &wall, red_side) > TOLERANCE);
	VOE_TEST_CHECK(gap_at(s, &indoors, &plain, PATCH) <= TOLERANCE);
	VOE_TEST_CHECK(gap_at(s, &room, &room_off, PATCH) <= TOLERANCE);
	block_with(s, (voe_render_light_blockers){ 0 });
}

static void run(struct scene *s)
{
	static const voe_render_shading_values values[SHADINGS] = {
		{ .base_colour = { 0.5f, 0.5f, 0.5f, 1.0f }, .roughness = 1.0f,
		  .base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ .base_colour = { 0.6f, 0.0f, 0.0f, 1.0f }, .roughness = 1.0f,
		  .base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f } },
	};
	voe_base_error error = VOE_BASE_OK;

	for (int i = 0; i < SHADINGS; i++)
		VOE_TEST_CHECK(voe_render_shading_create(s->device, values[i],
							 &s->shadings[i], &error));
	VOE_TEST_CHECK(voe_render_geometry_create(s->device, CUBE_VERTICES, 24,
						  CUBE_INDICES, 36, &s->cube,
						  &error));
	s->light = sun_view((voe_math_float3){ 0.0f, 0.0f, 0.0f }, LIGHT_HALF);
	s->camera.view = look_at((voe_math_float3){ -4.0f, 30.0f, -4.0f },
				 (voe_math_float3){ -4.0f, 0.0f, 1.0f }, NARROW);
	s->camera.light = (voe_render_light){ .direction = SUN_DIRECTION,
					      .colour = { 1.0f, 1.0f, 1.0f } };
	s->camera.shadow = (voe_render_shadow){
		.splits = { FAR_PLANE },
		.texels = { 2.0f * LIGHT_HALF / SHADOW_SIDE },
		.count = 1,
	};
	s->camera.shadow.cascades[0] =
		voe_math_float4x4_mul(s->light.projection, s->light.view);
	s->bounce = (struct voe_render_bounce_frame){
		.cell = { -12, -6, -12 },
		.corner = { -24.0f, -12.0f, -24.0f },
		.spacing = VOE_RENDER_BOUNCE_SPACING,
	};
	s->volume_light = sun_view(
		voe_math_float3_add(
			s->bounce.corner,
			voe_math_float3_scale(
				(voe_math_float3){ VOE_RENDER_BOUNCE_PROBES_XZ,
						   VOE_RENDER_BOUNCE_PROBES_Y,
						   VOE_RENDER_BOUNCE_PROBES_XZ },
				0.5f * VOE_RENDER_BOUNCE_SPACING)),
		VOLUME_HALF);
	kinds_on_the_patch(s);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4 * 1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { .arena = arena };

	scene.device = voe_render_device_new_headless(
		arena, (voe_platform_size){ SIDE, SIDE }, CAPACITIES, &error);
	if (scene.device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				     error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		voe_base_arena_destroy(arena);
		return 0;
	}
	VOE_TEST_CHECK(scene.device != NULL);
	if (scene.device != NULL) {
		if (scene.device->output_layer)
			run(&scene);
		else
			printf("note: no shaderOutputLayer, so nothing bounces\n");
		voe_render_device_destroy(scene.device);
	}
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
