// Lit surfaces read the bounce (ADR-0308 point 6): that a camera pass on a
// target whose grid was updated this frame adds the bounce to its lit surfaces,
// that one with no update reads none, that an unshaded pass ignores it, and that
// the fill still holds as a floor. Four claims, each reading pixels.
//
// THE SCENE: grey ground (a 40 m flat box, base 0.5) and a red wall (base 1, 0,
// 0) standing on it at x = 0, its red face to +x; a sun from that side at 45°,
// fill 0; four cascades and a bounce pass through one orthographic view along
// the sun with both casters drawn; the window's grid updated about the wall
// (lowest cell −16, corner −32 m); then a camera pass looking down at the ground
// beside the wall. Pixels are found by projecting world points through the
// camera, with the engine's one Y flip.
//
// THE RED WALL REDDENS THE GROUND NEAR IT. With the update, the ground 1 m from
// the wall has red over green greater than the ground 10 m away, and greater
// than the same pixel in a frame with no update.
//
// NO UPDATE IS NO BOUNCE: the ground is grey, red within 2/255 of green.
//
// AN UNSHADED PASS IS THE SAME PICTURE with the update and without.
//
// THE FILL IS A FLOOR: at 0.5 with no update, the ground in the wall's shadow
// reads the fill × base colour, as it did before the bounce.
//
// A MACHINE WITH NO USABLE VULKAN SKIPS AND SAYS SO.
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

#define SIDE 128
#define SHADOW_SIDE 512
#define PASSES (VOE_RENDER_SHADOW_CASCADES + 2)
// The sun's orthographic box: LIGHT_HALF metres either side of LIGHT_AT across
// the sun, LIGHT_BACK metres back toward the sun, depth LIGHT_NEAR to LIGHT_FAR.
#define LIGHT_HALF 16.0f
#define LIGHT_BACK 30.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 60.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define FILL 0.5f
#define GREY_LEVEL 0.5f
// A driver's rounding into an sRGB target.
#define TOLERANCE 3
#define GREY_ALIKE 2

static const voe_render_shading_values GREY = {
	.base_colour = { GREY_LEVEL, GREY_LEVEL, GREY_LEVEL, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

static const voe_render_shading_values RED = {
	.base_colour = { 1.0f, 0.0f, 0.0f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// The grid's lowest cell and its corner: 64 m about the origin.
static const struct voe_render_bounce_update UPDATE = {
	.cell = { -16, -16, -16 },
	.corner = { -32.0f, -32.0f, -32.0f },
};

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

// The ground, 40 m square with its top at y = 0; the wall 0.2 m thick, 4 m
// tall and 6 m long, its red face at x = 0.1.
static const voe_math_float3 GROUND_SCALE = { 40.0f, 0.1f, 40.0f };
static const voe_math_float3 GROUND_AT = { 0.0f, -0.05f, 0.0f };
static const voe_math_float3 WALL_SCALE = { 0.2f, 4.0f, 6.0f };
static const voe_math_float3 WALL_AT = { 0.0f, 2.0f, 0.0f };
// Where the claims look: the ground 1 m and 10 m from the wall on its lit
// side, and in its shadow on the other.
static const voe_math_float3 NEAR = { 1.0f, 0.0f, 0.0f };
static const voe_math_float3 FAR = { 10.0f, 0.0f, 0.0f };
static const voe_math_float3 SHADED = { -1.5f, 0.0f, 1.0f };
// The camera, above the lit side, looking down at the ground beside the wall;
// the middle of the light's box.
static const voe_math_float3 EYE = { 4.0f, 10.0f, 14.0f };
static const voe_math_float3 LOOK_AT = { 4.0f, 0.0f, 0.0f };
static const voe_math_float3 LIGHT_AT = { 3.0f, 0.0f, 0.0f };

struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_shading red;
	voe_render_pass_camera camera;
	voe_render_view light;
};

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

// The camera at EYE looking at LOOK_AT, reverse-Z perspective.
static voe_render_view scene_camera(void)
{
	voe_math_float3 z = voe_math_float3_normalize(
		voe_math_float3_sub(EYE, LOOK_AT));
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x =
		voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_render_view view =
		basis_view(x, voe_math_float3_cross(z, x), z, EYE);
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The sun's view: looking along (−1, −1, 0) from LIGHT_BACK metres back from
// LIGHT_AT, orthographic and reverse-Z — depth one at LIGHT_NEAR, nought at
// LIGHT_FAR.
static voe_render_view scene_light(void)
{
	float r = 0.70710678f;
	voe_math_float3 x = { r, -r, 0.0f };
	voe_math_float3 y = { 0.0f, 0.0f, -1.0f };
	voe_math_float3 z = { r, r, 0.0f };
	voe_render_view light = basis_view(
		x, y, z,
		voe_math_float3_add(LIGHT_AT, voe_math_float3_scale(z, LIGHT_BACK)));
	float span = LIGHT_FAR - LIGHT_NEAR;

	light.projection.m[0][0] = 1.0f / LIGHT_HALF;
	light.projection.m[1][1] = 1.0f / LIGHT_HALF;
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
			voe_math_float4x4_from_translation(at),
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

// One frame: the scene into the four cascades and the bounce map, the window's
// grid updated when `update`, the camera pass, and the window read back.
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
							    &scene->light));
		draw_scene(scene);
		voe_render_pass_end(scene->device);
	}
	VOE_TEST_CHECK(voe_render_bounce_pass_begin(
		scene->device, &scene->light, &scene->camera.light));
	draw_scene(scene);
	if (update)
		VOE_TEST_CHECK(voe_render_bounce_update(
			scene->device, VOE_RENDER_TARGET_WINDOW, &UPDATE));
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

// The pixel `world` lands on: through the camera to clip, and clip +y is row 0
// — the engine's one Y flip. NULL off the picture.
static const uint8_t *pixel_at(const voe_render_picture *picture,
			       const voe_render_view *camera,
			       voe_math_float3 world)
{
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(camera->projection, camera->view),
		(voe_math_float4){ world.x, world.y, world.z, 1.0f });
	float column = (clip.x / clip.w * 0.5f + 0.5f) * (float)picture->width;
	float row = (0.5f - clip.y / clip.w * 0.5f) * (float)picture->height;

	if (picture->pixels == NULL || column < 0.0f || row < 0.0f ||
	    column >= (float)picture->width || row >= (float)picture->height)
		return NULL;
	return &picture->pixels[((size_t)row * picture->width +
				 (size_t)column) *
				4];
}

// Red over green at `world`, or −1 when it is off the picture or black.
static float redness(const voe_render_picture *picture,
		     const voe_render_view *camera, voe_math_float3 world)
{
	const uint8_t *p = pixel_at(picture, camera, world);

	if (p == NULL || p[1] == 0)
		return -1.0f;
	printf("  (%g, %g, %g): %d %d %d\n", world.x, world.y, world.z, p[0],
	       p[1], p[2]);
	return (float)p[0] / (float)p[1];
}

// Red and green within GREY_ALIKE at `world`.
static bool grey_at(const voe_render_picture *picture,
		    const voe_render_view *camera, voe_math_float3 world)
{
	const uint8_t *p = pixel_at(picture, camera, world);

	return p != NULL && abs((int)p[0] - (int)p[1]) <= GREY_ALIKE;
}

// A linear value as the byte an sRGB target stores for it.
static int srgb_byte(float linear)
{
	float encoded = linear <= 0.0031308f ?
				linear * 12.92f :
				1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

static bool same_picture(const voe_render_picture *a,
			 const voe_render_picture *b)
{
	return a->pixels != NULL && b->pixels != NULL &&
	       a->width == b->width && a->height == b->height &&
	       memcmp(a->pixels, b->pixels,
		      (size_t)a->width * a->height * 4) == 0;
}

static void the_wall_reddens_the_ground(struct scene *scene,
					voe_base_arena *arena)
{
	const voe_render_view *camera = &scene->camera.view;
	voe_render_picture bounced = draw_frame(scene, true, arena);
	voe_render_picture plain = draw_frame(scene, false, arena);

	printf("bounced:\n");
	float near = redness(&bounced, camera, NEAR);
	float far = redness(&bounced, camera, FAR);
	printf("plain:\n");
	float near_plain = redness(&plain, camera, NEAR);

	VOE_TEST_CHECK(near > far);
	VOE_TEST_CHECK(near > near_plain);
	VOE_TEST_CHECK(grey_at(&plain, camera, NEAR));
	VOE_TEST_CHECK(grey_at(&plain, camera, FAR));
}

static void unshaded_ignores_the_bounce(struct scene *scene,
					voe_base_arena *arena)
{
	voe_render_picture bounced;
	voe_render_picture plain;

	scene->camera.light.unshaded = 1;
	bounced = draw_frame(scene, true, arena);
	plain = draw_frame(scene, false, arena);
	scene->camera.light.unshaded = 0;
	VOE_TEST_CHECK(same_picture(&bounced, &plain));
}

static void the_fill_is_a_floor(struct scene *scene, voe_base_arena *arena)
{
	const voe_render_view *camera = &scene->camera.view;
	voe_render_picture filled;
	const uint8_t *p;

	scene->camera.light.fill = (voe_math_float3){ FILL, FILL, FILL };
	filled = draw_frame(scene, false, arena);
	scene->camera.light.fill = (voe_math_float3){ 0.0f, 0.0f, 0.0f };
	p = pixel_at(&filled, camera, SHADED);
	VOE_TEST_CHECK(p != NULL);
	if (p == NULL)
		return;
	printf("shaded: %d %d %d, fill %d\n", p[0], p[1], p[2],
	       srgb_byte(FILL * GREY_LEVEL));
	VOE_TEST_CHECK(abs((int)p[0] - srgb_byte(FILL * GREY_LEVEL)) <=
		       TOLERANCE);
	VOE_TEST_CHECK(abs((int)p[1] - srgb_byte(FILL * GREY_LEVEL)) <=
		       TOLERANCE);
}

// The device, both records, the cube and the cameras; false when it skipped.
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

	scene->light = scene_light();
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
			scene->light.projection, scene->light.view);
		scene->camera.shadow.texels[i] =
			2.0f * LIGHT_HALF / SHADOW_SIDE;
	}
	return true;
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4 * 1024 * 1024);
	struct scene scene = { 0 };

	if (open_scene(&scene, arena)) {
		the_wall_reddens_the_ground(&scene, arena);
		unshaded_ignores_the_bounce(&scene, arena);
		the_fill_is_a_floor(&scene, arena);
		voe_render_device_destroy(scene.device);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
