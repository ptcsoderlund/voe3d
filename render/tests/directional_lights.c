// EVERY DIRECTIONAL LIGHT LIGHTS THE SURFACE (ADR-0357 point 2). A grey ground
// quad seen straight down from 10 m and a box caster on it, 2 m square and 3 m
// tall about x = 0, under a sun from +x at N·L 0.8 with fill 0.1, its cascade on
// shadow slot 0. Six pictures:
//
// 1. `more` empty: the reference;
// 2. a zeroed `more` array with a count of 0: byte for byte the reference;
// 3. a faint blue moon from −x, fill 0.05, unshadowed: every pixel at least the
//    reference, blue up, red unchanged in the box's shadow under the sun;
// 4. both shadowed, the moon on slot 1 once voe_render_shadow_lights_ready(2)
//    has had a frame: the sun's patch, west of the box, without the sun's red,
//    the moon's, east of it, without the moon's blue;
// 5. a Room blocker over the left half, the sun's mask 0 and the moon's its
//    bit: the left lit by the moon alone, the right by the sun alone;
// 6. that blocker a Fill box (ADR-0361 point 1), the sun's mask 0 and the
//    moon's its bit, the moon unshadowed and level, N·L 0 on the ground so its
//    fill is whole: the sun's patch, in the box, has the moon's blue fill and
//    no red; the right has the sun's light as in case 1 and the moon's fill.
//
// Built as blocked_light.c builds its device, reading the window back through
// voe_render_target_read as RGBA8 as shadow.c does. A cascade is a hand-built
// orthographic view along its light over the whole ground.
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
#define SHADOW_SIDE 256
#define CASES 6

#define RED 0
#define GREEN 1
#define BLUE 2
#define TOLERANCE 3

// 32 pixels are 10 m on the ground. Columns 8 and 56 are 7.3 m either side
// of the box, lit by both lights; 24 and 39 are 2.3 m west and east of it, in
// the sun's shadow and the moon's; row 32 runs through the box's middle.
#define WEST_LIT 8
#define EAST_LIT 56
#define SUN_PATCH 24
#define MOON_PATCH 39
#define MIDDLE 32

#define EYE_Y 10.0f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f
#define GREY 0.5f
// The lights' orthographic boxes: LIGHT_HALF metres about the origin, from
// LIGHT_DISTANCE back along the light, depth from LIGHT_NEAR to LIGHT_FAR.
#define LIGHT_HALF 12.0f
#define LIGHT_DISTANCE 20.0f
#define LIGHT_NEAR 1.0f
#define LIGHT_FAR 40.0f

static const voe_render_capacities CAPACITIES = {
	.vertices = 28,
	.indices = 42,
	.geometries = 2,
	.objects = 4,
	.shadings = 1,
	.passes = 3,
	.shadow_size = SHADOW_SIDE,
};

// A 20 m square at y = 0 facing up, counter-clockwise seen from above.
#define G 10.0f
static const voe_render_vertex GROUND_VERTICES[4] = {
	{ { -G, 0, G }, { 0, 1, 0 }, { 0, 0 } },
	{ { G, 0, G }, { 0, 1, 0 }, { 0, 0 } },
	{ { G, 0, -G }, { 0, 1, 0 }, { 0, 0 } },
	{ { -G, 0, -G }, { 0, 1, 0 }, { 0, 0 } },
};

static const uint32_t GROUND_INDICES[6] = { 0, 1, 2, 0, 2, 3 };

// A unit cube about the origin, four vertices a face with real normals,
// counter-clockwise from outside: shadow.c's.
#define H 0.5f
static const voe_render_vertex BOX_VERTICES[24] = {
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

static const uint32_t BOX_INDICES[36] = {
	3, 2, 1, 3, 1, 0,	 7, 6, 5, 7, 5, 4,
	11, 10, 9, 11, 9, 8,	 15, 14, 13, 15, 13, 12,
	19, 18, 17, 19, 17, 16,	 23, 22, 21, 23, 21, 20,
};

static const voe_math_float3 BOX_AT = { 0.0f, 1.5f, 0.0f };
static const voe_math_float3 BOX_SCALE = { 2.0f, 3.0f, 2.0f };

struct scene {
	voe_render_device *device;
	voe_render_geometry ground;
	voe_render_geometry box;
	voe_render_shading grey;
	voe_base_arena *arena;
};

// Straight down from (0, EYE_Y, 0), world +X to the right and −Z up the
// picture, a 90° field of view, reverse-Z: blocked_light.c's camera.
static voe_render_view the_camera(void)
{
	voe_math_float3 eye = { 0.0f, EYE_Y, 0.0f };
	voe_math_float3 rows[3] = {
		{ 1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, -1.0f },
		{ 0.0f, 1.0f, 0.0f },
	};
	voe_render_view view = { .eye = eye };
	float span = FAR_PLANE - NEAR_PLANE;

	for (int r = 0; r < 3; r++) {
		view.view.m[r][0] = rows[r].x;
		view.view.m[r][1] = rows[r].y;
		view.view.m[r][2] = rows[r].z;
		view.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	view.view.m[3][3] = 1.0f;
	view.projection.m[0][0] = 1.0f;
	view.projection.m[1][1] = 1.0f;
	view.projection.m[2][2] = NEAR_PLANE / span;
	view.projection.m[2][3] = NEAR_PLANE * FAR_PLANE / span;
	view.projection.m[3][2] = -1.0f;
	return view;
}

// The view of a light travelling `direction`, from LIGHT_DISTANCE back along
// it, orthographic and reverse-Z: depth one at LIGHT_NEAR, nought at LIGHT_FAR.
static voe_render_view light_view(voe_math_float3 direction)
{
	voe_math_float3 z = { -direction.x, -direction.y, -direction.z };
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };
	voe_math_float3 x =
		voe_math_float3_normalize(voe_math_float3_cross(up, z));
	voe_math_float3 y = voe_math_float3_cross(z, x);
	voe_math_float3 rows[3] = { x, y, z };
	voe_math_float3 eye = { z.x * LIGHT_DISTANCE, z.y * LIGHT_DISTANCE,
				z.z * LIGHT_DISTANCE };
	voe_render_view light = { .eye = eye };
	float span = LIGHT_FAR - LIGHT_NEAR;

	for (int r = 0; r < 3; r++) {
		light.view.m[r][0] = rows[r].x;
		light.view.m[r][1] = rows[r].y;
		light.view.m[r][2] = rows[r].z;
		light.view.m[r][3] = -voe_math_float3_dot(rows[r], eye);
	}
	light.view.m[3][3] = 1.0f;
	light.projection.m[0][0] = 1.0f / LIGHT_HALF;
	light.projection.m[1][1] = 1.0f / LIGHT_HALF;
	light.projection.m[2][2] = 1.0f / span;
	light.projection.m[2][3] = LIGHT_FAR / span;
	light.projection.m[3][3] = 1.0f;
	return light;
}

// One cascade over the whole picture from `light`, read from `slot`.
static voe_render_shadow one_cascade(const voe_render_view *light, uint32_t slot)
{
	voe_render_shadow shadow = {
		.splits = { FAR_PLANE },
		.texels = { 2.0f * LIGHT_HALF / SHADOW_SIDE },
		.count = 1,
		.slot = slot,
	};

	shadow.cascades[0] =
		voe_math_float4x4_mul(light->projection, light->view);
	return shadow;
}

static voe_render_object ground_object(voe_render_shading shading)
{
	return (voe_render_object){
		.world = voe_math_float4x4_identity(),
		.normal = voe_math_float4x4_identity(),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

static voe_render_object box_object(voe_render_shading shading)
{
	voe_math_float3 inverse = { 1.0f / BOX_SCALE.x, 1.0f / BOX_SCALE.y,
				    1.0f / BOX_SCALE.z };

	return (voe_render_object){
		.world = voe_math_float4x4_mul(
			voe_math_float4x4_from_translation(BOX_AT),
			voe_math_float4x4_from_scale(BOX_SCALE)),
		.normal = voe_math_float4x4_from_scale(inverse),
		.shading = shading.index,
		.colour = { 1.0f, 1.0f, 1.0f, 1.0f },
	};
}

// The box into layer `layer` from `light`.
static void shadow_pass(struct scene *scene, uint32_t layer,
			const voe_render_view *light)
{
	VOE_TEST_CHECK(voe_render_shadow_pass_begin(scene->device, layer, light));
	VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->box,
					     box_object(scene->grey)));
	voe_render_pass_end(scene->device);
}

// One frame: the sun's cascade into layer 0, the moon's into layer 4 when
// `moon` is set, then the ground and box through `camera`, read back.
static voe_render_picture draw_case(struct scene *scene,
				    const voe_render_pass_camera *camera,
				    const voe_render_view *sun,
				    const voe_render_view *moon)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK(voe_render_frame_begin(scene->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return picture;
	shadow_pass(scene, 0, sun);
	if (moon != NULL)
		shadow_pass(scene, VOE_RENDER_SHADOW_CASCADES, moon);
	VOE_TEST_CHECK(voe_render_pass_begin(scene->device,
					     VOE_RENDER_TARGET_WINDOW, camera));
	VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->ground,
					     ground_object(scene->grey)));
	VOE_TEST_CHECK(voe_render_frame_draw(scene->device, scene->box,
					     box_object(scene->grey)));
	voe_render_pass_end(scene->device);
	VOE_TEST_CHECK(voe_render_frame_end(scene->device));
	VOE_TEST_CHECK(voe_render_target_read(scene->device,
					      VOE_RENDER_TARGET_WINDOW,
					      scene->arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL && picture.width == SIDE &&
		       picture.height == SIDE);
	return picture;
}

static const unsigned char *pixel_at(const voe_render_picture *picture,
				     int column, int row)
{
	return picture->pixels + ((size_t)row * SIDE + (size_t)column) * 4;
}

// Every channel of every pixel of `picture` at least the reference's.
static void check_at_least(const voe_render_picture *picture,
			   const voe_render_picture *reference)
{
	for (size_t i = 0; i < (size_t)SIDE * SIDE * 4; i++)
		if (picture->pixels[i] < reference->pixels[i]) {
			VOE_TEST_CHECK(picture->pixels[i] >= reference->pixels[i]);
			return;
		}
}

static void check_moon_adds_blue(const voe_render_picture *moonlit,
				 const voe_render_picture *reference)
{
	const int columns[3] = { WEST_LIT, SUN_PATCH, EAST_LIT };

	check_at_least(moonlit, reference);
	for (int i = 0; i < 3; i++)
		VOE_TEST_CHECK(pixel_at(moonlit, columns[i], MIDDLE)[BLUE] >
			       pixel_at(reference, columns[i], MIDDLE)[BLUE] +
				       TOLERANCE * 4);
	for (int row = MIDDLE - 2; row <= MIDDLE + 2; row++)
		VOE_TEST_CHECK_INT(pixel_at(moonlit, SUN_PATCH, row)[RED],
				   pixel_at(reference, SUN_PATCH, row)[RED]);
	// The sun's shadow is there in the reference: its patch is the fill.
	VOE_TEST_CHECK(pixel_at(reference, SUN_PATCH, MIDDLE)[RED] + 40 <
		       pixel_at(reference, WEST_LIT, MIDDLE)[RED]);
}

// The sun's patch without the sun's red, the moon's without the moon's blue,
// each against the ground lit by both on its own side.
static void check_two_patches(const voe_render_picture *both)
{
	const unsigned char *sun_patch = pixel_at(both, SUN_PATCH, MIDDLE);
	const unsigned char *moon_patch = pixel_at(both, MOON_PATCH, MIDDLE);
	const unsigned char *west = pixel_at(both, WEST_LIT, MIDDLE);
	const unsigned char *east = pixel_at(both, EAST_LIT, MIDDLE);

	VOE_TEST_CHECK(sun_patch[RED] + 40 < west[RED]);
	VOE_TEST_CHECK(sun_patch[BLUE] > sun_patch[RED] + 20);
	VOE_TEST_CHECK(moon_patch[BLUE] + 10 < east[BLUE]);
	VOE_TEST_CHECK(abs(moon_patch[RED] - east[RED]) <= TOLERANCE * 2);
	VOE_TEST_CHECK(moon_patch[RED] > sun_patch[RED] + 40);
}

// The left lit by the moon alone, blue and nothing else; the right by the sun
// alone, white and lit.
static void check_rooms(const voe_render_picture *roomed)
{
	for (int row = 0; row < SIDE; row += 8) {
		const unsigned char *left = pixel_at(roomed, WEST_LIT, row);
		const unsigned char *right = pixel_at(roomed, EAST_LIT, row);

		VOE_TEST_CHECK(left[RED] <= TOLERANCE);
		VOE_TEST_CHECK(left[GREEN] <= TOLERANCE);
		VOE_TEST_CHECK(left[BLUE] > 40);
		VOE_TEST_CHECK(right[RED] > 40);
		VOE_TEST_CHECK(abs(right[BLUE] - right[RED]) <= TOLERANCE);
	}
}

// In the Fill box the sun's patch has the moon's fill and none of the sun's;
// outside it the sun lights as in `reference` and the moon still fills.
static void check_fill_box(const voe_render_picture *filled,
			   const voe_render_picture *reference)
{
	const unsigned char *patch = pixel_at(filled, SUN_PATCH, MIDDLE);
	const unsigned char *east = pixel_at(filled, EAST_LIT, MIDDLE);
	const unsigned char *east_before = pixel_at(reference, EAST_LIT, MIDDLE);

	VOE_TEST_CHECK(patch[RED] <= TOLERANCE);
	VOE_TEST_CHECK(patch[GREEN] <= TOLERANCE);
	VOE_TEST_CHECK(patch[BLUE] > TOLERANCE * 4);
	VOE_TEST_CHECK(abs(east[RED] - east_before[RED]) <= TOLERANCE);
	VOE_TEST_CHECK(east[BLUE] > east[RED]);
}

// An unturned box of centre `c` and half sizes `h`: rows (a_i / h_i,
// −a_i·c / h_i) and a sphere of radius |h|.
static voe_render_light_blocker box_blocker(voe_math_float3 c, voe_math_float3 h)
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

static void draw_and_check(struct scene *scene)
{
	const voe_math_float3 sun_direction = { -0.6f, -0.8f, 0.0f };
	const voe_math_float3 moon_direction = { 0.6f, -0.8f, 0.0f };
	const voe_render_view sun_view = light_view(sun_direction);
	const voe_render_view moon_view = light_view(moon_direction);
	const voe_render_directional_light zeroed[VOE_RENDER_DIRECTIONAL_LIGHTS - 1] = { 0 };
	const voe_render_light_blocker room[1] = {
		box_blocker((voe_math_float3){ -5.5f, 0.0f, 0.0f },
			    (voe_math_float3){ 5.5f, 1.0f, 11.0f }),
	};
	voe_render_directional_light moon[1] = { {
		.light = { .direction = moon_direction,
			   .intensity = 1.0f,
			   .colour = { 0.0f, 0.0f, 1.0f },
			   .fill = { 0.0f, 0.0f, 0.05f } },
	} };
	voe_render_pass_camera camera = {
		.view = the_camera(),
		.light = { .direction = sun_direction,
			   .intensity = 3.0f,
			   .colour = { 1.0f, 1.0f, 1.0f },
			   .fill = { 0.1f, 0.1f, 0.1f } },
		.shadow = one_cascade(&sun_view, 0),
	};
	voe_render_picture pictures[CASES];

	pictures[0] = draw_case(scene, &camera, &sun_view, NULL);
	camera.more = (voe_render_directional_lights){ zeroed, 0 };
	pictures[1] = draw_case(scene, &camera, &sun_view, NULL);
	camera.more = (voe_render_directional_lights){ moon, 1 };
	pictures[2] = draw_case(scene, &camera, &sun_view, NULL);
	moon[0].shadow = one_cascade(&moon_view, 1);
	pictures[3] = draw_case(scene, &camera, &sun_view, &moon_view);
	moon[0].shadow = (voe_render_shadow){ 0 };
	moon[0].blockers = 1u;
	camera.blockers = (voe_render_light_blockers){ .blockers = room,
						       .count = 1 };
	pictures[4] = draw_case(scene, &camera, &sun_view, NULL);
	moon[0].light.direction = (voe_math_float3){ 1.0f, 0.0f, 0.0f };
	camera.blockers.indoors = 1u;
	pictures[5] = draw_case(scene, &camera, &sun_view, NULL);

	for (int i = 0; i < CASES; i++)
		if (pictures[i].pixels == NULL)
			return;
	VOE_TEST_CHECK(memcmp(pictures[1].pixels, pictures[0].pixels,
			      (size_t)SIDE * SIDE * 4) == 0);
	check_moon_adds_blue(&pictures[2], &pictures[0]);
	check_two_patches(&pictures[3]);
	check_rooms(&pictures[4]);
	check_fill_box(&pictures[5], &pictures[0]);
	printf("fill box patch %d/%d\n",
	       pixel_at(&pictures[5], SUN_PATCH, MIDDLE)[RED],
	       pixel_at(&pictures[5], SUN_PATCH, MIDDLE)[BLUE]);
	printf("sun patch %d/%d, moon patch %d/%d, lit west %d/%d, east %d/%d (red/blue)\n",
	       pixel_at(&pictures[3], SUN_PATCH, MIDDLE)[RED],
	       pixel_at(&pictures[3], SUN_PATCH, MIDDLE)[BLUE],
	       pixel_at(&pictures[3], MOON_PATCH, MIDDLE)[RED],
	       pixel_at(&pictures[3], MOON_PATCH, MIDDLE)[BLUE],
	       pixel_at(&pictures[3], WEST_LIT, MIDDLE)[RED],
	       pixel_at(&pictures[3], WEST_LIT, MIDDLE)[BLUE],
	       pixel_at(&pictures[3], EAST_LIT, MIDDLE)[RED],
	       pixel_at(&pictures[3], EAST_LIT, MIDDLE)[BLUE]);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size = { SIDE, SIDE };
	voe_render_shading_values values = {
		.base_colour = { GREY, GREY, GREY, 1.0f },
		.roughness = 1.0f,
		.base_colour_uv_rect = { 0.0f, 0.0f, 1.0f, 1.0f },
	};
	struct scene scene = { .arena = arena };
	voe_platform_size frame = { SIDE, SIDE };
	bool drawing = false;

	scene.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						      &error);
	if (scene.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(scene.device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}

	VOE_TEST_CHECK(voe_render_geometry_create(scene.device, GROUND_VERTICES,
						  4, GROUND_INDICES, 6,
						  &scene.ground, &error));
	VOE_TEST_CHECK(voe_render_geometry_create(scene.device, BOX_VERTICES, 24,
						  BOX_INDICES, 36, &scene.box,
						  &error));
	VOE_TEST_CHECK(voe_render_shading_create(scene.device, values,
						 &scene.grey, &error));

	// The moon's slot: wanted, then a frame for the array to grow in.
	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(scene.device, 2), 1);
	VOE_TEST_CHECK(voe_render_frame_begin(scene.device, frame, &drawing));
	if (drawing)
		VOE_TEST_CHECK(voe_render_frame_end(scene.device));
	VOE_TEST_CHECK_INT(voe_render_shadow_lights_ready(scene.device, 2), 2);
	if (voe_render_shadow_lights_ready(scene.device, 2) == 2)
		draw_and_check(&scene);

	voe_render_device_destroy(scene.device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
