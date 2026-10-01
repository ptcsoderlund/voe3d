// The bounce where the editor runs it (bug 01): bounce.c's wall and ground,
// but off the world origin, drawn about the eye the way 3d draws, under a
// bounce view as wide as 3d's fit, so a negative grid cell that is no multiple
// of 32 and coarse VPLs are what the stages see; then scrolled, and from two
// views in one frame. Each claim reads pixels. Shares nothing with bounce.c by
// include: the few statics it needs are copied.
//
// THE SCENE: grey ground (a 40 m flat box, base 0.5) and a red wall (base 1,
// 0, 0) on it at world (−71, 0, −45), its red face to +x, on a probe plane on
// purpose; the eye 10 m above at (−67, 10, −31), every position handed to
// render as world − eye. A sun from +x at 45°; four cascades through a 32 m
// orthographic box along it, the bounce pass through one 112 m across; the
// window's grid updated at lowest cell (−52, −8, −39), its corner that cell ×
// 2 m − eye; then a camera pass looking down at the ground beside the wall.
// Pixels are found by projecting through the camera, with the engine's one Y
// flip.
//
// OFF THE ORIGIN THE WALL STILL REDDENS THE GROUND: sun π, fill 0, with the
// update the ground 1 m from the wall has red over green greater than the
// ground 10 m away by at least 0.05.
//
// THE BOUNCE NEVER DARKENS: sun 9, fill 0.09, at a 7 × 7 lattice of ground
// points across the wall's sunlit side and its shadow, every channel with the
// update is at least the same pixel with no update, less 1/255.
//
// SCROLLING: twelve frames with sun π, both frame slots in turn, the lowest
// cell from 12 below its x to 1 below, +1 a frame and the corner with it, the
// eye fixed; then at the scene's cell the two claims above still hold.
//
// TWO VIEWS, ONE FRAME: a second device with `targets` 2. View A is the
// scene's eye and grid onto target A; view B's eye and bounce light sit 40 m
// further along +x, its grid at lowest cell (−46, −8, −39), looking at the
// same wall, onto target B. In one frame: the cascades about eye A, A's bounce pass
// and update, B's bounce pass and update, a camera pass on each. Each target's
// ground pixel 1 m from the wall is, within 2/255 a channel, the one it shows
// in a frame where only its own bounce pass and update ran.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SIDE 128
#define SHADOW_SIDE 512
#define PASSES (VOE_RENDER_SHADOW_CASCADES + 4)
// The cascades' box is SHADOW_HALF metres either side of LIGHT_AT across the
// sun, the bounce pass's BOUNCE_HALF; both LIGHT_BACK metres back toward the
// sun, depth LIGHT_NEAR to LIGHT_FAR.
#define SHADOW_HALF 16.0f
#define BOUNCE_HALF 56.0f
#define LIGHT_BACK 30.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 60.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define REDDER_BY 0.05f
#define BRIGHT_SUN 9.0f
#define DIM_FILL 0.09f
#define LATTICE 7
#define SCROLL_FRAMES 12
#define VIEWS_APART 40.0f
#define VIEWS_WITHIN 2

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

static const voe_render_shading_values RED = {
	.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// Where the wall stands and where the eye is, in world metres.
static const voe_math_float3 SCENE = { -71.0f, 0.0f, -45.0f };
static const voe_math_float3 EYE = { -67.0f, 10.0f, -31.0f };
static const int32_t LOWEST_CELL[3] = { -52, -8, -39 };
static const int32_t LOWEST_CELL_B[3] = { -46, -8, -39 };

// Counter-clockwise from outside, four vertices a face, a unit cube about the
// origin with real normals.
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

// Offsets from SCENE: the ground 40 m square with its top at y = 0; the wall
// 0.2 m thick, 4 m tall and 6 m long, its red face at x = 0.1.
static const voe_math_float3 GROUND_SCALE = { 40.0f, 0.1f, 40.0f };
static const voe_math_float3 GROUND_AT = { 0.0f, -0.05f, 0.0f };
static const voe_math_float3 WALL_SCALE = { 0.2f, 4.0f, 6.0f };
static const voe_math_float3 WALL_AT = { 0.0f, 2.0f, 0.0f };
static const voe_math_float3 NEAR = { 1.0f, 0.0f, 0.0f };
static const voe_math_float3 FAR = { 10.0f, 0.0f, 0.0f };
static const voe_math_float3 LOOK_AT = { 4.0f, 0.0f, 0.0f };
static const voe_math_float3 LIGHT_AT = { 3.0f, 0.0f, 0.0f };

// One view of the scene: its eye in world metres, the target it draws onto,
// its camera, its bounce light and its grid's update.
struct look {
	voe_math_float3 eye;
	voe_render_target target;
	voe_render_pass_camera camera;
	voe_render_view bounce;
	struct voe_render_bounce_update update;
};

struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_shading red;
	voe_render_view shadow;
	struct look window;
};

// An offset from SCENE as render sees it: about `eye`.
static voe_math_float3 about_eye(voe_math_float3 eye, voe_math_float3 offset)
{
	return voe_math_float3_sub(voe_math_float3_add(SCENE, offset), eye);
}

// A view whose rows are `x`, `y`, `z` from `eye`: it looks along −z.
static voe_render_view basis_view(voe_math_float3 x, voe_math_float3 y,
				  voe_math_float3 z, voe_math_float3 eye)
{
	voe_math_float3 rows[3] = { x, y, z };
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

// The camera at `eye`, the origin of render's space, looking at LOOK_AT,
// reverse-Z perspective.
static voe_render_view scene_camera(voe_math_float3 eye)
{
	voe_math_float3 origin = { 0.0f, 0.0f, 0.0f };
	voe_math_float3 z = voe_math_float3_normalize(
		voe_math_float3_sub(origin, about_eye(eye, LOOK_AT)));
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x =
		voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_render_view view =
		basis_view(x, voe_math_float3_cross(z, x), z, origin);
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The sun's view about `eye`, `half` metres either side: along (−1, −1, 0)
// from LIGHT_BACK metres back from the scene offset `at`, orthographic and
// reverse-Z.
static voe_render_view scene_light(voe_math_float3 eye, voe_math_float3 at,
				   float half)
{
	float r = 0.70710678f;
	voe_math_float3 x = { r, -r, 0.0f };
	voe_math_float3 y = { 0.0f, 0.0f, -1.0f };
	voe_math_float3 z = { r, r, 0.0f };
	voe_render_view light = basis_view(
		x, y, z,
		voe_math_float3_add(about_eye(eye, at),
				    voe_math_float3_scale(z, LIGHT_BACK)));
	float span = LIGHT_FAR - LIGHT_NEAR;

	light.projection.m[0][0] = 1.0f / half;
	light.projection.m[1][1] = 1.0f / half;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = LIGHT_FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

static voe_render_object placed(voe_render_shading shading, voe_math_float3 eye,
				voe_math_float3 at, voe_math_float3 scale)
{
	voe_math_float3 inverse = { 1.0f / scale.x, 1.0f / scale.y,
				    1.0f / scale.z };

	return (voe_render_object){
		.world = voe_math_float4x4_mul(
			voe_math_float4x4_from_translation(about_eye(eye, at)),
			voe_math_float4x4_from_scale(scale)),
		.normal = voe_math_float4x4_from_scale(inverse),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

static void draw_scene(struct scene *scene, voe_math_float3 eye)
{
	VOE_TEST_CHECK(voe_render_frame_draw(
		scene->device, scene->cube,
		placed(scene->grey, eye, GROUND_AT, GROUND_SCALE)));
	VOE_TEST_CHECK(voe_render_frame_draw(
		scene->device, scene->cube,
		placed(scene->red, eye, WALL_AT, WALL_SCALE)));
}

// The grid's lowest cell `cell` and its corner about the look's eye.
static void set_cell(struct look *look, const int32_t cell[3])
{
	for (int a = 0; a < 3; a++)
		look->update.cell[a] = cell[a];
	look->update.corner = (voe_math_float3){
		(float)cell[0] * VOE_RENDER_BOUNCE_SPACING - look->eye.x,
		(float)cell[1] * VOE_RENDER_BOUNCE_SPACING - look->eye.y,
		(float)cell[2] * VOE_RENDER_BOUNCE_SPACING - look->eye.z,
	};
}

// The frame's start and the cascades about the window look's eye; false when
// there was nothing to draw into.
static bool begin_frame(struct scene *scene)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return false;
	for (uint32_t i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		VOE_TEST_CHECK(voe_render_shadow_pass_begin(scene->device, i,
							    &scene->shadow));
		draw_scene(scene, scene->window.eye);
		voe_render_pass_end(scene->device);
	}
	return true;
}

// `look`'s bounce pass, and its grid's update when `update`.
static void bounce_look(struct scene *scene, struct look *look, bool update)
{
	VOE_TEST_CHECK(voe_render_bounce_pass_begin(
		scene->device, &look->bounce, &look->camera.light));
	draw_scene(scene, look->eye);
	if (update)
		VOE_TEST_CHECK(voe_render_bounce_update(
			scene->device, look->target, &look->update));
}

static void camera_look(struct scene *scene, const struct look *look)
{
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device, look->target,
					     &look->camera));
	draw_scene(scene, look->eye);
	voe_render_pass_end(scene->device);
}

static voe_render_picture read_look(struct scene *scene,
				    const struct look *look,
				    voe_base_arena *arena)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_render_target_read(scene->device, look->target,
					      arena, &picture, &error));
	return picture;
}

// One frame: the cascades, the bounce map, the window's grid updated when
// `update`, the camera pass, and the window read back.
static voe_render_picture draw_frame(struct scene *scene, bool update,
				     voe_base_arena *arena)
{
	voe_render_picture picture = { 0 };

	if (!begin_frame(scene))
		return picture;
	bounce_look(scene, &scene->window, update);
	camera_look(scene, &scene->window);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	return read_look(scene, &scene->window, arena);
}

// The pixel the scene offset `at` lands on: through `look`'s camera to clip,
// and clip +y is row 0 — the engine's one Y flip. NULL off the picture.
static const uint8_t *pixel_at(const voe_render_picture *picture,
			       const struct look *look, voe_math_float3 at)
{
	const voe_render_view *camera = &look->camera.view;
	voe_math_float3 p = about_eye(look->eye, at);
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(camera->projection, camera->view),
		(voe_math_float4){ p.x, p.y, p.z, 1.0f });
	float column = (clip.x / clip.w * 0.5f + 0.5f) * (float)picture->width;
	float row = (0.5f - clip.y / clip.w * 0.5f) * (float)picture->height;

	if (picture->pixels == NULL || column < 0.0f || row < 0.0f ||
	    column >= (float)picture->width || row >= (float)picture->height)
		return NULL;
	return &picture->pixels[((size_t)row * picture->width +
				 (size_t)column) *
				4];
}

// Red over green at `at`, or −1 when it is off the picture or black.
static float redness(const voe_render_picture *picture,
		     const struct look *look, voe_math_float3 at)
{
	const uint8_t *p = pixel_at(picture, look, at);

	if (p == NULL || p[1] == 0)
		return -1.0f;
	printf("  (%g, %g, %g): %d %d %d\n", at.x, at.y, at.z, p[0], p[1],
	       p[2]);
	return (float)p[0] / (float)p[1];
}

static void off_the_origin_the_wall_reddens(struct scene *scene,
					    voe_base_arena *arena)
{
	voe_render_picture bounced = draw_frame(scene, true, arena);

	printf("off the origin:\n");
	float near = redness(&bounced, &scene->window, NEAR);
	float far = redness(&bounced, &scene->window, FAR);

	VOE_TEST_CHECK(far >= 0.0f);
	VOE_TEST_CHECK(near >= far + REDDER_BY);
}

static void the_bounce_never_darkens(struct scene *scene,
				     voe_base_arena *arena)
{
	voe_render_picture bounced;
	voe_render_picture plain;
	int darker = 0;

	scene->window.camera.light.intensity = BRIGHT_SUN;
	scene->window.camera.light.fill = (voe_math_float3){ DIM_FILL, DIM_FILL,
							     DIM_FILL };
	bounced = draw_frame(scene, true, arena);
	plain = draw_frame(scene, false, arena);
	for (int i = 0; i < LATTICE * LATTICE; i++) {
		// x across the shadow (−4..0) and the lit side, z along the wall.
		voe_math_float3 at = { -3.4f + 1.2f * (float)(i % LATTICE), 0.0f,
				       -2.5f + 5.0f / 6.0f * (float)(i / LATTICE) };
		const uint8_t *b = pixel_at(&bounced, &scene->window, at);
		const uint8_t *p = pixel_at(&plain, &scene->window, at);

		VOE_TEST_CHECK(b != NULL && p != NULL);
		if (b == NULL || p == NULL)
			continue;
		for (int c = 0; c < 3; c++)
			if ((int)b[c] < (int)p[c] - 1) {
				printf("darker at (%g, %g): %d %d %d against %d %d %d\n",
				       at.x, at.z, b[0], b[1], b[2], p[0], p[1],
				       p[2]);
				darker++;
				break;
			}
	}
	VOE_TEST_CHECK_INT(darker, 0);
}

// The sun π with no fill, as the scene opens.
static void plain_sun(struct look *look)
{
	look->camera.light.intensity = 3.14159265f;
	look->camera.light.fill = (voe_math_float3){ 0.0f, 0.0f, 0.0f };
}

static void scrolling_keeps_both_claims(struct scene *scene,
					voe_base_arena *arena)
{
	int32_t cell[3] = { LOWEST_CELL[0], LOWEST_CELL[1], LOWEST_CELL[2] };

	printf("scrolling:\n");
	plain_sun(&scene->window);
	for (int f = 0; f < SCROLL_FRAMES; f++) {
		cell[0] = LOWEST_CELL[0] - SCROLL_FRAMES + f;
		set_cell(&scene->window, cell);
		(void)draw_frame(scene, true, arena);
	}
	set_cell(&scene->window, LOWEST_CELL);
	off_the_origin_the_wall_reddens(scene, arena);
	the_bounce_never_darkens(scene, arena);
}

// The look at `eye` onto `target`, its grid at `cell`, its bounce light at
// the scene offset `light_at`, and the cascades `shadow` drawn about
// `shadow_eye`.
static struct look open_look(voe_math_float3 eye, voe_render_target target,
			     const int32_t cell[3], voe_math_float3 light_at,
			     const voe_render_view *shadow,
			     voe_math_float3 shadow_eye)
{
	// About this eye, then about the eye the cascades were drawn about.
	voe_math_float4x4 to_shadow = voe_math_float4x4_mul(
		voe_math_float4x4_mul(shadow->projection, shadow->view),
		voe_math_float4x4_from_translation(
			voe_math_float3_sub(eye, shadow_eye)));
	struct look look = {
		.eye = eye,
		.target = target,
		.camera = {
			.view = scene_camera(eye),
			.light = { .direction = { -0.70710678f, -0.70710678f, 0.0f },
				   .intensity = 3.14159265f,
				   .colour = { 1.0f, 1.0f, 1.0f } },
			.shadow = { .splits = { FAR_PLANE, FAR_PLANE, FAR_PLANE,
						FAR_PLANE },
				    .count = VOE_RENDER_SHADOW_CASCADES },
		},
		.bounce = scene_light(eye, light_at, BOUNCE_HALF),
	};

	for (uint32_t i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		look.camera.shadow.cascades[i] = to_shadow;
		look.camera.shadow.texels[i] = 2.0f * SHADOW_HALF / SHADOW_SIDE;
	}
	set_cell(&look, cell);
	return look;
}

// The device with `targets` of the caller's own, both records and the cube;
// false when it skipped.
static bool open_device(struct scene *scene, uint32_t targets,
			voe_base_arena *arena)
{
	voe_render_capacities room = {
		.vertices = 24,
		.indices = 36,
		.geometries = 1,
		.objects = 2 * PASSES,
		.shadings = 2,
		.passes = PASSES,
		.shadow_size = SHADOW_SIDE,
		.targets = targets,
	};
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;

	scene->device = voe_render_device_new_headless(arena, size, room, &error);
	if (scene->device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				      error == VOE_BASE_ERROR_UNSUPPORTED)) {
		printf("skip: %s\n", voe_base_error_string(error));
		return false;
	}
	VOE_TEST_CHECK(scene->device != NULL);
	if (scene->device == NULL)
		return false;
	VOE_TEST_CHECK(voe_render_shading_create(scene->device, GREY,
						 &scene->grey, &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene->device, RED,
						 &scene->red, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(scene->device, CUBE_VERTICES,
						  24, CUBE_INDICES, 36,
						  &scene->cube, &error));
	scene->shadow = scene_light(EYE, LIGHT_AT, SHADOW_HALF);
	scene->window = open_look(EYE, VOE_RENDER_TARGET_WINDOW, LOWEST_CELL,
				  LIGHT_AT, &scene->shadow, EYE);
	return true;
}

// The pixel 1 m from the wall in `look`'s picture, or NULL off it.
static const uint8_t *near_pixel(const voe_render_picture *picture,
				 const struct look *look)
{
	const uint8_t *p = pixel_at(picture, look, NEAR);

	VOE_TEST_CHECK(p != NULL);
	if (p != NULL)
		printf("  %d %d %d\n", p[0], p[1], p[2]);
	return p;
}

// `both` against `alone`, every channel within VIEWS_WITHIN.
static void check_alike(const uint8_t *both, const uint8_t *alone)
{
	if (both == NULL || alone == NULL)
		return;
	for (int c = 0; c < 3; c++)
		VOE_TEST_CHECK(abs((int)both[c] - (int)alone[c]) <= VIEWS_WITHIN);
}

// A frame with `first`'s bounce pass and update, then `second`'s when not
// NULL, then a camera pass on each.
static void views_frame(struct scene *scene, struct look *first,
			struct look *second)
{
	if (!begin_frame(scene))
		return;
	bounce_look(scene, first, true);
	if (second != NULL)
		bounce_look(scene, second, true);
	camera_look(scene, first);
	if (second != NULL)
		camera_look(scene, second);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
}

static void two_views_in_one_frame(voe_base_arena *arena)
{
	struct scene scene = { 0 };
	voe_render_target targets[2];
	voe_render_texture textures[2];
	voe_base_error error = VOE_BASE_OK;
	voe_math_float3 apart = { VIEWS_APART, 0.0f, 0.0f };
	struct look a;
	struct look b;

	if (!open_device(&scene, 2, arena))
		return;
	for (int t = 0; t < 2; t++)
		VOE_TEST_CHECK(voe_render_target_create(scene.device, SIDE, SIDE,
							&targets[t], &textures[t],
							&error));
	a = open_look(EYE, targets[0], LOWEST_CELL, LIGHT_AT, &scene.shadow,
		      EYE);
	b = open_look(voe_math_float3_add(EYE, apart), targets[1], LOWEST_CELL_B,
		      voe_math_float3_add(LIGHT_AT, apart), &scene.shadow, EYE);

	printf("two views, one frame:\n");
	views_frame(&scene, &a, &b);
	voe_render_picture both_a = read_look(&scene, &a, arena);
	voe_render_picture both_b = read_look(&scene, &b, arena);
	views_frame(&scene, &a, NULL);
	voe_render_picture alone_a = read_look(&scene, &a, arena);
	views_frame(&scene, &b, NULL);
	voe_render_picture alone_b = read_look(&scene, &b, arena);

	check_alike(near_pixel(&both_a, &a), near_pixel(&alone_a, &a));
	check_alike(near_pixel(&both_b, &b), near_pixel(&alone_b, &b));
	voe_render_device_destroy(scene.device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(8 * 1024 * 1024);
	struct scene scene = { 0 };

	if (open_device(&scene, 0, arena)) {
		off_the_origin_the_wall_reddens(&scene, arena);
		the_bounce_never_darkens(&scene, arena);
		scrolling_keeps_both_claims(&scene, arena);
		voe_render_device_destroy(scene.device);
		two_views_in_one_frame(arena);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
