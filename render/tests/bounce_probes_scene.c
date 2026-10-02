// Probes relit a level at a time (ADR-0326 point 6), read in the picture, and
// what 0312 and 0317 ask of the bounce. Headless; read through
// ../src/device_internal.h for the relight's dispatch count.
//
// A FRAME: begin, the scene into cascade 0 of the sun's shadow map through an
// orthographic view along the sun (as shadow.c draws), voe_render_bounce_begin
// with that shadow record about the origin (lowest cell (−12, −6, −12)),
// capture passes drawing the scene until one does not open, relight, then a
// camera pass of the scene read back. A scene is settled by such frames until
// one opens no capture pass; a new scene is queued whole by a stale sphere.
// Every probe is captured, nearest the origin first, in about 108 frames.
//
// Grey ground (0.5) everywhere; a sun along (0.6, −0.8, 0). Points are world
// points projected through the camera, with the engine's one Y flip.
// - A red wall (0.6), 0.5 × 4 × 12 m across x = 0, sun 2, fill 0.1: ground on
//   its lit side 1.5 m off is redder than ground 10 m off; ground in its shadow
//   at its foot within 4/255 of bounces 0; five open ground points 2 m apart
//   along x within 2/255 of each other; strength 0 the same picture as bounces
//   0; strength 2 redder by the wall than 1, open ground within 1/255 at both.
//   The first picture, nothing changed since settling, opens no capture pass
//   and dispatches nothing.
// - A red box (0.6), 1 m a side at (0, 0.5, 0.5), sun 2, fill 0 (0.1 hid this
//   leak): the ground in its shadow 0.25 m from its foot within 4/255 of
//   bounces 0 (0312).
// - A closed room (8 m, walls and roof 0.5 m thick), sun 10, fill 0, camera
//   inside: the floor's middle within 2/255 of bounces 0. A doorway 2 × 3 m in
//   the +z wall, out of the sun: the back wall facing it brighter at bounces 1
//   than at 0, and the corner beside it, (3, 0, 3), brighter at bounces 2 than
//   at 1.
// - A white wall (0.9) under a dim sun (0.05), a lamp of range 3 m 2 m before
//   it: the wall 4.5 m from the lamp, past its reach, brighter at lamp bounces 1
//   than at 0.
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
// The sun's orthographic box: LIGHT_HALF metres either side of the origin,
// from LIGHT_DISTANCE back along the sun, depth LIGHT_NEAR to LIGHT_FAR.
#define LIGHT_HALF 30.0f
#define LIGHT_DISTANCE 60.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 120.0f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 200.0f
#define WIDE 1.6f
#define NARROW 1.0471976f

enum { GREY, RED, WHITE, SHADINGS };

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

// One box of a scene: its centre, its size and its shading.
struct box {
	voe_math_float3 at;
	voe_math_float3 size;
	uint32_t shading;
};

#define GROUND { { 0.0f, -0.05f, 0.0f }, { 100.0f, 0.1f, 100.0f }, GREY }
#define ROOM_SIDES                                                             \
	GROUND, { { -4.0f, 1.875f, 0.0f }, { 0.5f, 3.75f, 8.5f }, GREY },      \
		{ { 4.0f, 1.875f, 0.0f }, { 0.5f, 3.75f, 8.5f }, GREY },       \
		{ { 0.0f, 1.875f, -4.0f }, { 7.5f, 3.75f, 0.5f }, GREY },      \
		{ { 0.0f, 4.0f, 0.0f }, { 8.5f, 0.5f, 8.5f }, GREY }

static const struct box WALL_SCENE[] = {
	GROUND,
	{ { 0.0f, 2.0f, 0.0f }, { 0.5f, 4.0f, 12.0f }, RED },
};
static const struct box BOX_SCENE[] = {
	GROUND,
	{ { 0.0f, 0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f }, RED },
};
static const struct box CLOSED_ROOM[] = {
	ROOM_SIDES,
	{ { 0.0f, 1.875f, 4.0f }, { 7.5f, 3.75f, 0.5f }, GREY },
};
static const struct box DOORWAY_ROOM[] = {
	ROOM_SIDES,
	{ { -2.375f, 1.875f, 4.0f }, { 2.75f, 3.75f, 0.5f }, GREY },
	{ { 2.375f, 1.875f, 4.0f }, { 2.75f, 3.75f, 0.5f }, GREY },
	{ { 0.0f, 3.375f, 4.0f }, { 2.0f, 0.75f, 0.5f }, GREY },
};
static const struct box LAMP_SCENE[] = {
	GROUND,
	{ { 0.0f, 4.0f, 0.0f }, { 0.5f, 8.0f, 12.0f }, WHITE },
};

struct scene {
	voe_render_device *device;
	voe_base_arena *arena;
	voe_render_geometry cube;
	voe_render_shading shadings[SHADINGS];
	const struct box *boxes;
	uint32_t box_count;
	voe_render_view light;
	voe_render_pass_camera camera;
	struct voe_render_bounce_frame bounce;
	voe_render_point_light lamp;
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

// The sun's view: from LIGHT_DISTANCE back along it, orthographic, reverse-Z.
static voe_render_view sun_view(void)
{
	voe_render_view light = view_from(
		voe_math_float3_scale(SUN_DIRECTION, -LIGHT_DISTANCE),
		voe_math_float3_scale(SUN_DIRECTION, -1.0f));
	float span = LIGHT_FAR - LIGHT_NEAR;

	light.projection.m[0][0] = 1.0f / LIGHT_HALF;
	light.projection.m[1][1] = 1.0f / LIGHT_HALF;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = LIGHT_FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

static void draw_boxes(struct scene *s)
{
	for (uint32_t i = 0; i < s->box_count; i++) {
		const struct box *b = &s->boxes[i];
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
	if (s->restale && s->device->window_volume.built) {
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

// `boxes` in place and every probe captured: frames until one opens no
// capture pass, past the first, which may only want the volume.
static void settle(struct scene *s, const struct box *boxes, uint32_t count)
{
	uint32_t frame = 0;

	s->boxes = boxes;
	s->box_count = count;
	s->restale = true;
	for (; frame < FRAMES_MAX; frame++)
		if (one_frame(s, NULL) == 0 && frame > 0)
			break;
	printf("settled in %u frames\n", frame);
	VOE_TEST_CHECK(frame < FRAMES_MAX);
}

// A picture with the scene's lights as they now are.
static voe_render_picture picture_of(struct scene *s)
{
	voe_render_picture picture = { 0 };

	VOE_TEST_CHECK_INT(one_frame(s, &picture), 0);
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

	VOE_TEST_CHECK(picture->pixels != NULL && clip.w > 0.0f &&
		       column >= 0.0f && row >= 0.0f &&
		       column < (float)picture->width &&
		       row < (float)picture->height);
	if (picture->pixels == NULL || clip.w <= 0.0f || column < 0.0f ||
	    row < 0.0f || column >= (float)picture->width ||
	    row >= (float)picture->height)
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

static int brightness_at(const struct scene *s, const voe_render_picture *p,
			 voe_math_float3 world)
{
	int rgb[3];

	rgb_at(s, p, world, rgb);
	return rgb[0] + rgb[1] + rgb[2];
}

static int redness_at(const struct scene *s, const voe_render_picture *p,
		      voe_math_float3 world)
{
	int rgb[3];

	rgb_at(s, p, world, rgb);
	return rgb[0] - rgb[1];
}

// The open ground points, five 2 m apart along x, out of the wall's sight.
static voe_math_float3 open_ground(int i)
{
	return (voe_math_float3){ 6.0f + 2.0f * (float)i, 0.0f, -14.0f };
}

static void a_red_wall(struct scene *s)
{
	const voe_math_float3 lit = { -1.5f, 0.0f, 0.0f };
	const voe_math_float3 far = { -10.0f, 0.0f, 0.0f };
	const voe_math_float3 foot = { 0.75f, 0.0f, 0.0f };
	const size_t bytes = (size_t)SIDE * SIDE * 4;
	voe_render_picture one, none, nought, twice;
	uint32_t before;
	int low = 255 * 3;
	int high = 0;

	s->camera.view = look_at((voe_math_float3){ 2.0f, 40.0f, -4.0f },
				 (voe_math_float3){ 2.0f, 0.0f, -7.0f }, NARROW);
	s->camera.light.intensity = 2.0f;
	s->camera.light.fill = (voe_math_float3){ 0.1f, 0.1f, 0.1f };
	s->bounce.sun = s->camera.light;
	s->bounce.sun_bounces = 1;
	s->bounce.sun_strength = 1.0f;
	settle(s, WALL_SCENE, 2);
	before = s->device->relight_dispatches;
	one = picture_of(s);
	VOE_TEST_CHECK_INT(s->device->relight_dispatches, before);
	s->bounce.sun_bounces = 0;
	none = picture_of(s);
	s->bounce.sun_bounces = 1;
	s->bounce.sun_strength = 0.0f;
	nought = picture_of(s);
	s->bounce.sun_strength = 2.0f;
	twice = picture_of(s);

	printf("wall: redness lit %d, far %d; foot gap %d; red by the wall %d, at strength 2 %d\n",
	       redness_at(s, &one, lit), redness_at(s, &one, far),
	       gap_at(s, &one, &none, foot), redness_at(s, &one, lit),
	       redness_at(s, &twice, lit));
	VOE_TEST_CHECK(redness_at(s, &one, lit) > redness_at(s, &one, far));
	VOE_TEST_CHECK(gap_at(s, &one, &none, foot) <= 4);
	for (int i = 0; i < 5; i++) {
		int b = brightness_at(s, &one, open_ground(i));

		low = b < low ? b : low;
		high = b > high ? b : high;
		VOE_TEST_CHECK(gap_at(s, &one, &twice, open_ground(i)) <= 1);
	}
	printf("open ground: %d to %d over three channels\n", low, high);
	for (int i = 1; i < 5; i++) {
		int a[3];
		int b[3];

		rgb_at(s, &one, open_ground(0), a);
		rgb_at(s, &one, open_ground(i), b);
		for (int c = 0; c < 3; c++)
			VOE_TEST_CHECK(abs(a[c] - b[c]) <= 2);
	}
	VOE_TEST_CHECK(none.pixels != NULL && nought.pixels != NULL &&
		       memcmp(none.pixels, nought.pixels, bytes) == 0);
	VOE_TEST_CHECK(redness_at(s, &twice, lit) > redness_at(s, &one, lit));
}

// The box's shadow on the ground runs x 0.5 to 1.25 m along the sun, z 0 to 1.
static void a_red_box(struct scene *s)
{
	const voe_math_float3 shade = { 0.75f, 0.0f, 0.5f };
	voe_render_picture one, none;
	int a[3];
	int b[3];

	s->camera.view = look_at((voe_math_float3){ 2.0f, 6.0f, -2.0f }, shade,
				 NARROW);
	s->camera.light.intensity = 2.0f;
	s->camera.light.fill = (voe_math_float3){ 0.0f, 0.0f, 0.0f };
	s->bounce.sun = s->camera.light;
	s->bounce.sun_bounces = 1;
	s->bounce.sun_strength = 1.0f;
	settle(s, BOX_SCENE, 2);
	one = picture_of(s);
	s->bounce.sun_bounces = 0;
	none = picture_of(s);
	rgb_at(s, &one, shade, a);
	rgb_at(s, &none, shade, b);
	printf("box: its shadow 0.25 m from its foot %d %d %d at bounces 1, %d %d %d at 0\n",
	       a[0], a[1], a[2], b[0], b[1], b[2]);
	VOE_TEST_CHECK(gap_at(s, &one, &none, shade) <= 4);
}

static void a_room(struct scene *s)
{
	const voe_math_float3 middle = { 0.0f, 0.0f, 0.0f };
	const voe_math_float3 facing_door = { 0.0f, 1.5f, -3.75f };
	const voe_math_float3 corner = { 3.0f, 0.0f, 3.0f };
	voe_render_picture one, none, two;

	s->camera.view = look_at((voe_math_float3){ 0.0f, 3.2f, -3.0f },
				 (voe_math_float3){ 0.0f, 0.0f, 2.0f }, WIDE);
	s->camera.light.intensity = 10.0f;
	s->camera.light.fill = (voe_math_float3){ 0.0f, 0.0f, 0.0f };
	s->bounce.sun = s->camera.light;
	s->bounce.sun_bounces = 1;
	s->bounce.sun_strength = 1.0f;
	settle(s, CLOSED_ROOM, 6);
	one = picture_of(s);
	s->bounce.sun_bounces = 0;
	none = picture_of(s);
	printf("closed room: middle gap %d\n", gap_at(s, &one, &none, middle));
	VOE_TEST_CHECK(gap_at(s, &one, &none, middle) <= 2);

	settle(s, DOORWAY_ROOM, 8);
	none = picture_of(s);
	s->bounce.sun_bounces = 1;
	one = picture_of(s);
	s->bounce.sun_bounces = 2;
	two = picture_of(s);
	printf("doorway: corner %d at 1, %d at 2\n",
	       brightness_at(s, &one, corner), brightness_at(s, &two, corner));
	VOE_TEST_CHECK(brightness_at(s, &two, corner) >
		       brightness_at(s, &one, corner));

	// The floor takes nothing from the sunlit ground outside, below its
	// plane (0327); the back wall, which faces the doorway, does.
	s->camera.view = look_at((voe_math_float3){ 0.0f, 2.0f, 2.0f },
				 facing_door, WIDE);
	s->bounce.sun_bounces = 0;
	none = picture_of(s);
	s->bounce.sun_bounces = 1;
	one = picture_of(s);
	printf("doorway: the wall facing it %d at 0, %d at 1\n",
	       brightness_at(s, &none, facing_door),
	       brightness_at(s, &one, facing_door));
	VOE_TEST_CHECK(brightness_at(s, &one, facing_door) >
		       brightness_at(s, &none, facing_door));
}

static void a_lamp_by_a_white_wall(struct scene *s)
{
	const voe_math_float3 above = { -0.25f, 5.5f, 0.0f };
	voe_render_picture one, none;

	s->camera.view = look_at((voe_math_float3){ -12.0f, 4.0f, 0.0f },
				 (voe_math_float3){ 0.0f, 4.0f, 0.0f }, NARROW);
	s->camera.light.intensity = 0.05f;
	s->bounce.sun = s->camera.light;
	s->bounce.sun_bounces = 0;
	s->lamp = (voe_render_point_light){
		.position = { -2.0f, 1.5f, 0.0f },
		.range = 3.0f,
		.colour = { 4.0f, 4.0f, 4.0f },
		.falloff = 1.0f,
		.bounces = 0,
		.bounce_strength = 1.0f,
	};
	s->camera.points = (voe_render_point_lights){ &s->lamp, 1 };
	s->bounce.points = s->camera.points;
	settle(s, LAMP_SCENE, 2);
	none = picture_of(s);
	s->lamp.bounces = 1;
	one = picture_of(s);
	printf("white wall above the lamp: %d at 0, %d at 1\n",
	       brightness_at(s, &none, above), brightness_at(s, &one, above));
	VOE_TEST_CHECK(brightness_at(s, &one, above) >
		       brightness_at(s, &none, above));
}

static void run(struct scene *s)
{
	static const voe_render_shading_values values[SHADINGS] = {
		{ .base_colour = { 0.5f, 0.5f, 0.5f, 1.0f }, .roughness = 1.0f,
		  .base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ .base_colour = { 0.6f, 0.0f, 0.0f, 1.0f }, .roughness = 1.0f,
		  .base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ .base_colour = { 0.9f, 0.9f, 0.9f, 1.0f }, .roughness = 1.0f,
		  .base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f } },
	};
	voe_base_error error = VOE_BASE_OK;

	for (int i = 0; i < SHADINGS; i++)
		VOE_TEST_CHECK(voe_render_shading_create(s->device, values[i],
							 &s->shadings[i], &error));
	VOE_TEST_CHECK(voe_render_geometry_create(s->device, CUBE_VERTICES, 24,
						  CUBE_INDICES, 36, &s->cube,
						  &error));
	s->light = sun_view();
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
		.shadow = s->camera.shadow,
	};
	a_red_wall(s);
	a_red_box(s);
	a_room(s);
	a_lamp_by_a_white_wall(s);
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
