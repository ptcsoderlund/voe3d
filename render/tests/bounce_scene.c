// The bounce where the editor runs it (bug 01): bounce.c's wall and ground,
// but off the world origin, drawn about the eye the way 3d draws, under a
// bounce view as wide as 3d's fit, so a negative grid cell that is no multiple
// of 32 and coarse VPLs are what the stages see. Two claims, each reading
// pixels. Shares nothing with bounce.c by include: the few statics it needs
// are copied.
//
// THE SCENE: grey ground (a 40 m flat box, base 0.5) and a red wall (base 1,
// 0, 0) on it at world (−71, 0, −45), its red face to +x; the eye 10 m above
// at (−67, 10, −31), every position handed to render as world − eye. A sun
// from +x at 45°; four cascades through a 32 m orthographic box along it, the
// bounce pass through one 112 m across; the window's grid updated at lowest
// cell (−52, −8, −39), its corner that cell × 2 m − eye; then a camera pass
// looking down at the ground beside the wall. Pixels are found by projecting
// through the camera, with the engine's one Y flip.
//
// OFF THE ORIGIN THE WALL STILL REDDENS THE GROUND: sun π, fill 0, with the
// update the ground 1 m from the wall has red over green greater than the
// ground 10 m away by at least 0.05.
//
// THE BOUNCE NEVER DARKENS: sun 9, fill 0.09, at a 7 × 7 lattice of ground
// points across the wall's sunlit side and its shadow, every channel with the
// update is at least the same pixel with no update, less 1/255.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
#include <render/device.h>

#include <base/arena.h>
#include <base/error.h>
#include <math/float4x4.h>
#include <platform/window.h>

#include <testing/test.h>

#include <stdio.h>
#include <math.h>

#define SIDE 128
#define SHADOW_SIDE 512
#define PASSES (VOE_RENDER_SHADOW_CASCADES + 2)
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

struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_shading red;
	voe_render_pass_camera camera;
	voe_render_view shadow;
	voe_render_view bounce;
	struct voe_render_bounce_update update;
};

// An offset from SCENE as render sees it: about the eye.
static voe_math_float3 about_eye(voe_math_float3 offset)
{
	return voe_math_float3_sub(voe_math_float3_add(SCENE, offset), EYE);
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

// The camera at the eye, the origin of render's space, looking at LOOK_AT,
// reverse-Z perspective.
static voe_render_view scene_camera(void)
{
	voe_math_float3 eye = { 0.0f, 0.0f, 0.0f };
	voe_math_float3 z = voe_math_float3_normalize(
		voe_math_float3_sub(eye, about_eye(LOOK_AT)));
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x =
		voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_render_view view =
		basis_view(x, voe_math_float3_cross(z, x), z, eye);
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The sun's view `half` metres either side: along (−1, −1, 0) from LIGHT_BACK
// metres back from LIGHT_AT, orthographic and reverse-Z.
static voe_render_view scene_light(float half)
{
	float r = 0.70710678f;
	voe_math_float3 x = { r, -r, 0.0f };
	voe_math_float3 y = { 0.0f, 0.0f, -1.0f };
	voe_math_float3 z = { r, r, 0.0f };
	voe_render_view light = basis_view(
		x, y, z,
		voe_math_float3_add(about_eye(LIGHT_AT),
				    voe_math_float3_scale(z, LIGHT_BACK)));
	float span = LIGHT_FAR - LIGHT_NEAR;

	light.projection.m[0][0] = 1.0f / half;
	light.projection.m[1][1] = 1.0f / half;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = LIGHT_FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

static voe_render_object placed(voe_render_shading shading, voe_math_float3 at,
				voe_math_float3 scale)
{
	voe_math_float3 inverse = { 1.0f / scale.x, 1.0f / scale.y,
				    1.0f / scale.z };

	return (voe_render_object){
		.world = voe_math_float4x4_mul(
			voe_math_float4x4_from_translation(about_eye(at)),
			voe_math_float4x4_from_scale(scale)),
		.normal = voe_math_float4x4_from_scale(inverse),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

static void draw_scene(struct scene *scene)
{
	VOE_TEST_CHECK(voe_render_frame_draw(
		scene->device, scene->cube,
		placed(scene->grey, GROUND_AT, GROUND_SCALE)));
	VOE_TEST_CHECK(voe_render_frame_draw(
		scene->device, scene->cube,
		placed(scene->red, WALL_AT, WALL_SCALE)));
}

// One frame: the cascades, the bounce map, the window's grid updated when
// `update`, the camera pass, and the window read back.
static voe_render_picture draw_frame(struct scene *scene, bool update,
				     voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	for (uint32_t i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		VOE_TEST_CHECK(voe_render_shadow_pass_begin(scene->device, i,
							    &scene->shadow));
		draw_scene(scene);
		voe_render_pass_end(scene->device);
	}
	VOE_TEST_CHECK(voe_render_bounce_pass_begin(
		scene->device, &scene->bounce, &scene->camera.light));
	draw_scene(scene);
	if (update)
		VOE_TEST_CHECK(voe_render_bounce_update(
			scene->device, VOE_RENDER_TARGET_WINDOW, &scene->update));
	VOE_TEST_CHECK(voe_render_pass_begin(
		scene->device, VOE_RENDER_TARGET_WINDOW, &scene->camera));
	draw_scene(scene);
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	VOE_TEST_CHECK(voe_render_target_read(scene->device,
					      VOE_RENDER_TARGET_WINDOW, arena,
					      &picture, &error));
	return picture;
}

// The pixel the scene offset `at` lands on: through the camera to clip, and
// clip +y is row 0 — the engine's one Y flip. NULL off the picture.
static const uint8_t *pixel_at(const voe_render_picture *picture,
			       const voe_render_view *camera,
			       voe_math_float3 at)
{
	voe_math_float3 p = about_eye(at);
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
		     const voe_render_view *camera, voe_math_float3 at)
{
	const uint8_t *p = pixel_at(picture, camera, at);

	if (p == NULL || p[1] == 0)
		return -1.0f;
	printf("  (%g, %g, %g): %d %d %d\n", at.x, at.y, at.z, p[0], p[1],
	       p[2]);
	return (float)p[0] / (float)p[1];
}

static void off_the_origin_the_wall_reddens(struct scene *scene,
					    voe_base_arena *arena)
{
	const voe_render_view *camera = &scene->camera.view;
	voe_render_picture bounced = draw_frame(scene, true, arena);

	printf("off the origin:\n");
	float near = redness(&bounced, camera, NEAR);
	float far = redness(&bounced, camera, FAR);

	VOE_TEST_CHECK(far >= 0.0f);
	VOE_TEST_CHECK(near >= far + REDDER_BY);
}

static void the_bounce_never_darkens(struct scene *scene,
				     voe_base_arena *arena)
{
	const voe_render_view *camera = &scene->camera.view;
	voe_render_picture bounced;
	voe_render_picture plain;
	int darker = 0;

	scene->camera.light.intensity = BRIGHT_SUN;
	scene->camera.light.fill = (voe_math_float3){ DIM_FILL, DIM_FILL,
						      DIM_FILL };
	bounced = draw_frame(scene, true, arena);
	plain = draw_frame(scene, false, arena);
	for (int i = 0; i < LATTICE * LATTICE; i++) {
		// x across the shadow (−4..0) and the lit side, z along the wall.
		voe_math_float3 at = { -3.4f + 1.2f * (float)(i % LATTICE), 0.0f,
				       -2.5f + 5.0f / 6.0f * (float)(i / LATTICE) };
		const uint8_t *b = pixel_at(&bounced, camera, at);
		const uint8_t *p = pixel_at(&plain, camera, at);

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

// The device, both records, the cube, the views and the update; false when it
// skipped.
static bool open_scene(struct scene *scene, voe_base_arena *arena)
{
	voe_render_capacities room = {
		.vertices = 24,
		.indices = 36,
		.geometries = 1,
		.objects = 2 * PASSES,
		.shadings = 2,
		.passes = PASSES,
		.shadow_size = SHADOW_SIDE,
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

	scene->shadow = scene_light(SHADOW_HALF);
	scene->bounce = scene_light(BOUNCE_HALF);
	scene->camera = (voe_render_pass_camera){
		.view = scene_camera(),
		.light = { .direction = { -0.70710678f, -0.70710678f, 0.0f },
			   .intensity = 3.14159265f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
		.shadow = { .splits = { FAR_PLANE, FAR_PLANE, FAR_PLANE,
					FAR_PLANE },
			    .count = VOE_RENDER_SHADOW_CASCADES },
	};
	for (uint32_t i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
		scene->camera.shadow.cascades[i] = voe_math_float4x4_mul(
			scene->shadow.projection, scene->shadow.view);
		scene->camera.shadow.texels[i] =
			2.0f * SHADOW_HALF / SHADOW_SIDE;
	}
	for (int a = 0; a < 3; a++)
		scene->update.cell[a] = LOWEST_CELL[a];
	scene->update.corner = (voe_math_float3){
		(float)LOWEST_CELL[0] * VOE_RENDER_BOUNCE_SPACING - EYE.x,
		(float)LOWEST_CELL[1] * VOE_RENDER_BOUNCE_SPACING - EYE.y,
		(float)LOWEST_CELL[2] * VOE_RENDER_BOUNCE_SPACING - EYE.z,
	};
	return true;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4 * 1024 * 1024);
	struct scene scene = { 0 };

	if (open_scene(&scene, arena)) {
		off_the_origin_the_wall_reddens(&scene, arena);
		the_bounce_never_darkens(&scene, arena);
		voe_render_device_destroy(scene.device);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
