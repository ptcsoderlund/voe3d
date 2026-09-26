// The sun's shadows (ADR-0258): that a frame can draw depth into the sun's four
// cascades and then its picture, that it counts like any other pass, and that a
// lit surface reads them. Four claims; only the last reads a pixel.
//
// A DEVICE WITHOUT shadow_size DRAWS AS BEFORE. Nought is the default every
// existing caller has, and it still opens, draws a cube into the window and ends.
//
// WITH IT, FOUR SHADOW PASSES AND A WINDOW PASS ARE ONE FRAME. Each cascade gets
// a pass with the cube drawn into it, the window gets one more, the frame ends
// true and holds five draw commands — the shadow draws counted like any other.
//
// A SHADOW PASS PAST `passes` IS REFUSED AND THE FRAME STILL ENDS. Room for two:
// the third shadow pass returns false, leaves no pass open, and _end is true.
//
// A CUBE SHADOWS THE FLOOR UNDER IT. A flattened cube for a floor, a cube above
// it, the sun straight down: one shadow pass onto cascade 0 through a hand-built
// orthographic view over both, then a window pass with `count` 1 and a split
// past the scene. The floor under the cube reads darker than the floor beside
// it and the cube's top reads lit; with `count` 0 the two floor pixels read
// alike; with `unshaded` set both read the base colour. Pixels are found by
// projecting world points through the camera, with the engine's one Y flip.
//
// With the validation layer present, a wrong layout, barrier or attachment in
// any of these is a message on stderr; the frames ending true is the rest.
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

#define SIDE 16
#define SHADOW_SIDE 64

static const voe_render_capacities CAPACITIES = {
	.vertices = 8,
	.indices = 36,
	.geometries = 1,
	.objects = 5,
	.shadings = 1,
	.passes = 5,
};

static const voe_render_shading_values GREY = {
	.base_colour = { 0.5f, 0.5f, 0.5f, 1.0f },
	.roughness = 1.0f,
	.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
};

// A cube half a unit a side about the origin, well inside an identity camera's
// clip volume. Its normals do not matter: nothing here is looked at.
static bool upload_cube(voe_render_device *device, voe_render_geometry *out)
{
	static const voe_render_vertex vertices[8] = {
		{ { -0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { -0.25f, 0.25f, 0.25f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { -0.25f, -0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { -0.25f, 0.25f, 0.75f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
	};
	static const uint32_t indices[36] = {
		0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
		3, 6, 2, 3, 7, 6, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5,
	};
	voe_base_error error = VOE_BASE_OK;

	return voe_render_geometry_create(device, vertices, 8, indices, 36, out,
					  &error);
}

static voe_render_view identity_view(void)
{
	return (voe_render_view){
		.view = voe_math_float4x4_identity(),
		.projection = voe_math_float4x4_identity(),
	};
}

static voe_render_object object(voe_render_shading shading)
{
	return (voe_render_object){
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// A device of `room`, with the grey record and the cube, or NULL — and *skip set
// when the reason is that this machine has no Vulkan to test.
static voe_render_device *open_device(voe_base_arena *arena,
				      voe_render_capacities room,
				      voe_render_geometry *cube,
				      voe_render_shading *grey, bool *skip)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, room, &error);

	*skip = device == NULL && (error == VOE_BASE_ERROR_UNAVAILABLE ||
				   error == VOE_BASE_ERROR_UNSUPPORTED);
	if (*skip)
		printf("skip: %s\n", voe_base_error_string(error));
	if (device == NULL)
		return NULL;

	VOE_TEST_CHECK(voe_render_shading_create(device, GREY, grey, &error));
	VOE_TEST_CHECK(upload_cube(device, cube));
	return device;
}

static bool open_frame(voe_render_device *device)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	return drawing;
}

static void window_pass(voe_render_device *device, voe_render_geometry cube,
			voe_render_shading grey)
{
	voe_render_pass_camera camera = {
		.view = identity_view(),
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 1.0f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
	};

	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	VOE_TEST_CHECK(voe_render_frame_draw(device, cube, object(grey)));
	voe_render_pass_end(device);
}

static bool no_shadow_draws_as_before(voe_base_arena *arena)
{
	voe_render_geometry cube;
	voe_render_shading grey;
	bool skip;
	voe_render_device *device =
		open_device(arena, CAPACITIES, &cube, &grey, &skip);

	if (skip)
		return false;
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return true;

	if (open_frame(device)) {
		window_pass(device, cube, grey);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 1);
	}
	voe_render_device_destroy(device);
	return true;
}

static void four_cascades_then_the_window(voe_base_arena *arena)
{
	voe_render_capacities room = CAPACITIES;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_view light = identity_view();
	bool skip;
	voe_render_device *device;

	room.shadow_size = SHADOW_SIDE;
	device = open_device(arena, room, &cube, &grey, &skip);
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return;

	// Twice, so each frame slot's maps are drawn and handed back once.
	for (int frame = 0; frame < 2 && open_frame(device); frame++) {
		for (uint32_t i = 0; i < VOE_RENDER_SHADOW_CASCADES; i++) {
			VOE_TEST_CHECK(voe_render_shadow_pass_begin(device, i,
								    &light));
			VOE_TEST_CHECK(voe_render_frame_draw(device, cube,
							     object(grey)));
			voe_render_pass_end(device);
		}
		window_pass(device, cube, grey);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK_INT(voe_render_frame_draw_count(device), 5);
	}
	voe_render_device_destroy(device);
}

static void a_shadow_pass_past_passes_is_refused(voe_base_arena *arena)
{
	voe_render_capacities room = CAPACITIES;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_view light = identity_view();
	bool skip;
	voe_render_device *device;

	room.shadow_size = SHADOW_SIDE;
	room.passes = 2;
	device = open_device(arena, room, &cube, &grey, &skip);
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return;

	if (open_frame(device)) {
		for (uint32_t i = 0; i < 2; i++) {
			VOE_TEST_CHECK(voe_render_shadow_pass_begin(device, i,
								    &light));
			voe_render_pass_end(device);
		}
		VOE_TEST_CHECK(!voe_render_shadow_pass_begin(device, 2, &light));
		VOE_TEST_CHECK(!voe_render_pass_is_open(device));
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	voe_render_device_destroy(device);
}

// ---- A cube shadows the floor under it ----

#define SCENE_SIDE 64
#define SCENE_SHADOW 256
// The light's orthographic box: LIGHT_HALF metres either side of the origin,
// looking down from LIGHT_HEIGHT, depth from LIGHT_NEAR to LIGHT_FAR below it.
#define LIGHT_HALF 4.0f
#define LIGHT_HEIGHT 10.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 20.0f
#define EYE 5.0f
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define GREY_LEVEL 0.5f
// A driver's rounding into an sRGB target, and for two floor pixels seen from
// slightly different angles, the specular term's small difference as well.
#define TOLERANCE 3
#define ALIKE 8

// Counter-clockwise from outside, four vertices a face, a unit cube about the
// origin with real normals: the lit claim needs them where the others did not.
#define H 0.5f
static const voe_render_vertex LIT_VERTICES[24] = {
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

static const uint32_t LIT_INDICES[36] = {
	3, 2, 1, 3, 1, 0,	 7, 6, 5, 7, 5, 4,
	11, 10, 9, 11, 9, 8,	 15, 14, 13, 15, 13, 12,
	19, 18, 17, 19, 17, 16,	 23, 22, 21, 23, 21, 20,
};

// The floor, 6 m square with its top at y = 0, and the cube from y = 1 to 2.
static const voe_math_float3 FLOOR_SCALE = { 6.0f, 0.1f, 6.0f };
static const voe_math_float3 FLOOR_AT = { 0.0f, -0.05f, 0.0f };
static const voe_math_float3 CUBE_AT = { 0.0f, 1.5f, 0.0f };
// Where the claims look: the floor under the cube, the floor two metres
// beside it, and the middle of the cube's top.
static const voe_math_float3 UNDER = { 0.0f, 0.0f, 0.0f };
static const voe_math_float3 BESIDE = { 2.0f, 0.0f, 0.0f };
static const voe_math_float3 TOP = { 0.0f, 2.0f, 0.0f };

struct scene {
	voe_render_device *device;
	voe_render_geometry cube;
	voe_render_shading grey;
	voe_render_pass_camera camera;
	voe_render_view light;
};

// A camera at (0, EYE, EYE) looking at the origin, reverse-Z perspective: the
// floor under the cube is in view below it and the cube's top above.
static voe_render_view scene_camera(void)
{
	voe_math_float3 eye = { 0.0f, EYE, EYE };
	voe_math_float3 z = voe_math_float3_normalize(eye);
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x =
		voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_math_float3 y = voe_math_float3_cross(z, x);
	voe_math_float3 rows[3] = { x, y, z };
	voe_render_view view = { .eye = eye };
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float span = FAR_PLANE - NEAR_PLANE;

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = focal;
	view.projection.m[1][1] = focal;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The sun's view: from LIGHT_HEIGHT straight down, x to the right and -z up the
// map, orthographic and reverse-Z — depth one at LIGHT_NEAR, nought at LIGHT_FAR.
static voe_render_view scene_light(void)
{
	voe_render_view light = { .eye = { 0.0f, LIGHT_HEIGHT, 0.0f } };
	float span = LIGHT_FAR - LIGHT_NEAR;

	light.view.m[0][0] = 1.0f;
	light.view.m[1][2] = -1.0f;
	light.view.m[2][1] = 1.0f;
	light.view.m[2][3] = -LIGHT_HEIGHT;
	light.view.m[3][3] = 1.0f;
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
	voe_math_float3 one = { 1.0f, 1.0f, 1.0f };

	VOE_TEST_CHECK(voe_render_frame_draw(
		scene->device, scene->cube,
		placed(scene->grey, FLOOR_AT, FLOOR_SCALE)));
	VOE_TEST_CHECK(voe_render_frame_draw(
		scene->device, scene->cube, placed(scene->grey, CUBE_AT, one)));
}

// One frame: the scene into cascade 0, then into the window with `camera`, then
// the window read back into `arena`.
static voe_render_picture draw_frame(struct scene *scene,
				     voe_base_arena *arena)
{
	voe_platform_size size = { SCENE_SIDE, SCENE_SIDE };
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	VOE_TEST_CHECK(voe_render_shadow_pass_begin(scene->device, 0,
						    &scene->light));
	draw_scene(scene);
	voe_render_pass_end(scene->device);
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

// The red byte of the pixel `world` lands on: through the camera to clip, and
// clip +y is row 0 — the engine's one Y flip.
static int red_at(const voe_render_picture *picture,
		  const voe_render_view *camera, voe_math_float3 world)
{
	voe_math_float4 clip = voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(camera->projection, camera->view),
		(voe_math_float4){ world.x, world.y, world.z, 1.0f });
	float column = (clip.x / clip.w * 0.5f + 0.5f) * (float)picture->width;
	float row = (0.5f - clip.y / clip.w * 0.5f) * (float)picture->height;

	if (picture->pixels == NULL || column < 0.0f || row < 0.0f ||
	    column >= (float)picture->width || row >= (float)picture->height)
		return -1;
	return picture->pixels[((size_t)row * picture->width + (size_t)column) *
			       4];
}

// A linear value as the byte an sRGB target stores for it.
static int srgb_byte(float linear)
{
	float encoded = linear <= 0.0031308f ?
				linear * 12.92f :
				1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;

	return (int)lroundf(encoded * 255.0f);
}

static void a_cube_shadows_the_floor_under_it(voe_base_arena *arena)
{
	voe_render_capacities room = {
		.vertices = 24,
		.indices = 36,
		.geometries = 1,
		.objects = 4,
		.shadings = 1,
		.passes = 2,
		.shadow_size = SCENE_SHADOW,
	};
	voe_render_shading_values grey = GREY;
	voe_platform_size size = { SCENE_SIDE, SCENE_SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct scene scene = { .light = scene_light() };
	voe_render_picture shadowed;
	voe_render_picture plain;
	voe_render_picture unshaded;

	grey.base_colour =
		(voe_math_float4){ GREY_LEVEL, GREY_LEVEL, GREY_LEVEL, 1.0f };
	scene.device = voe_render_device_new_headless(arena, size, room, &error);
	VOE_TEST_CHECK(scene.device != NULL);
	if (scene.device == NULL)
		return;
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, grey,
						 &scene.grey, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(scene.device, LIT_VERTICES,
						  24, LIT_INDICES, 36,
						  &scene.cube, &error));

	scene.camera = (voe_render_pass_camera){
		.view = scene_camera(),
		.light = { .direction = { 0.0f, -1.0f, 0.0f },
			   .intensity = 3.14159265f,
			   .colour = { 1.0f, 1.0f, 1.0f } },
		.shadow = { .splits = { FAR_PLANE },
			    .texels = { 2.0f * LIGHT_HALF / SCENE_SHADOW },
			    .count = 1 },
	};
	scene.camera.shadow.cascades[0] = voe_math_float4x4_mul(
		scene.light.projection, scene.light.view);
	shadowed = draw_frame(&scene, arena);
	scene.camera.shadow.count = 0;
	plain = draw_frame(&scene, arena);
	scene.camera.shadow.count = 1;
	scene.camera.light.unshaded = 1;
	unshaded = draw_frame(&scene, arena);

	const voe_render_view *camera = &scene.camera.view;
	int under = red_at(&shadowed, camera, UNDER);
	int beside = red_at(&shadowed, camera, BESIDE);
	int top = red_at(&shadowed, camera, TOP);

	printf("shadowed: under %d, beside %d, top %d\n", under, beside, top);
	VOE_TEST_CHECK(under + 60 < beside);
	VOE_TEST_CHECK(top * 2 > beside);
	VOE_TEST_CHECK(abs(red_at(&plain, camera, UNDER) -
			   red_at(&plain, camera, BESIDE)) <= ALIKE);
	VOE_TEST_CHECK(abs(red_at(&unshaded, camera, UNDER) -
			   srgb_byte(GREY_LEVEL)) <= TOLERANCE);
	VOE_TEST_CHECK(abs(red_at(&unshaded, camera, BESIDE) -
			   srgb_byte(GREY_LEVEL)) <= TOLERANCE);
	voe_render_device_destroy(scene.device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	if (no_shadow_draws_as_before(arena)) {
		four_cascades_then_the_window(arena);
		a_shadow_pass_past_passes_is_refused(arena);
		a_cube_shadows_the_floor_under_it(arena);
	}

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
