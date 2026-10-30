// Two game frames draw a world through its own camera on a headless device
// opened with VOE_GAME_CAPACITIES, and what was drawn comes out as a PNG. The
// world is built through the typed creation calls — a camera, a light and one
// cube — not through voe_game_scene_build, which only a cooked scene.c
// defines (game/scene.h).
//
// TWO CASES: that world, and the same world without the light, its light
// table still registered by voe_game_world_new. The second is bug 01 of 024:
// a lightless scene asserted in voe_3d_draw_system_frame; it no longer
// asserts and now draws black (0287).
//
// TWO FRAMES each, because the shape system gives a fresh shape its mesh and
// material in the first and the second draws a world that already has them.
// Both at lag 0, the current transforms. Between them a replace intent for the
// cube's collider is submitted, and the second frame's world step drains it.
//
// THE SHADOW CASE: a capsule set over the cube casts onto it. The light, the
// cube and the capsule all say cast_shadows (literals cast nothing, 0324), so
// both shapes are drawn into every cascade and the window pass; with
// VOE_GAME_CAPACITIES every pass and object fits and the frame comes back
// true, lit or not (0258).
//
// THE SUN AND MOON CASE: the shadow case's cube and capsule under a sun and a
// moon from opposite sides, both casting and bouncing once. Three frames, each
// true with VOE_GAME_CAPACITIES: the first asks for the second light's maps,
// the second draws with them, and the third relights (0357 points 3 and 4).
//
// THE LAMP CASE: a cube, a floor under it and a point light with
// cast_shadows beside them; the lamp takes slot 1, the point-shadow pass
// opens after the sun's, and both frames come back true (0325).
//
// THE LANDSCAPE CASE: a 64-cell, 512 m flat landscape table loaded with
// voe_game_models_landscapes and a row wearing it under the sun; its nodes
// are drawn into every pass and both frames come back true (0396 point 4).
// THE INTERFACE CASE: a label over the world: true, fitting VOE_GAME_CAPACITIES.
//
// THE BLOCKER CASE: a camera four metres over a grey ground under a sun and a
// 1 m light blocker, grown to 2 m between the frames and read back: the game
// draws nothing for a blocker (0347).
//
// THE DIRECT CASE: the same ground under a low sun with a fill and a Direct
// blocker over it (0350, 0352), two frames, both true, read back.
//
// THE SKY CASE: the lit cube with a sky row of 0406's defaults and a picture
// row, read back: the corner reads blue above red, not the clear colour; an
// exposure replace of -4 on the picture row between the frames darkens the
// cube's centre; neither row leaves the clear colour; a picture row at -4 and
// no sky darkens the cube against no row (0405, 0406).
//
// THE MODEL CASE: a thing whose model an empty store lacks draws as nothing;
// glb_build.h's triangle drawn twice through a window's LOD memory is in it.
// THE COUNTS CASE: a cube before the camera and one behind: drawn 1, skipped 1.
//
// THE HAZE, CLOUDS AND FOG CASES: a Haze row's replace reaches the sky record
// with no sky drawn, its top above the origin's eye; a Volumetric Clouds row at
// 1500 reaches the clouds record, its bottom 1500 over that eye; a fog volume of
// thickness 1 is one record of density 0.1, none when off. No device.
//
// Each draw case writes one file into the working directory, checks it and
// removes it, pass or fail. A machine with no usable Vulkan skips and says so.
#include <game/frame.h>
#include <game/interface.h>
#include <game/landscapes.h>
#include <game/models.h>
#include <game/world.h>

#include <app/app.h>

#include <base/arena.h>
#include <base/error.h>

#include <ecs/component.h>

#include <math/quat.h>

#include <physics/collider_system.h>

#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/clouds_system.h>
#include <scene/fog_volume_system.h>
#include <scene/haze_system.h>
#include <scene/identity_system.h>
#include <scene/light_blocker_system.h>
#include <scene/light_system.h>
#include <scene/picture_system.h>
#include <scene/point_light_system.h>
#include <scene/sky_system.h>
#include <scene/transform_system.h>

#include <3d/fog_volume.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/shape_component.h>

#include <ui/widgets.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "glb_build.h"

#define WIDTH 128
#define HEIGHT 72
#define CAPTURE_PATH "game_frame_test.png"
// A channel at most this is black: a driver's rounding into an sRGB target.
#define TOLERANCE 3

// An unrotated pose at `position`, scale one.
static voe_scene_transform placed(float x, float y, float z)
{
	return (voe_scene_transform){
		.position = { x, y, z },
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, 0.0f),
		.scale = { 1.0f, 1.0f, 1.0f },
	};
}

// A camera back along Z with the default lens, a light turned by its transform
// and with no fill when `lit`, and a cube
// at the origin with a collider of 1. Returns the cube.
static voe_ecs_entity build(voe_ecs_world *world, bool lit)
{
	voe_ecs_entity camera, light, cube;
	const voe_scene_camera *lens = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));

	VOE_TEST_CHECK(
		voe_scene_transform_add(world, camera, placed(0.0f, 0.0f, 5.0f)));
	VOE_TEST_CHECK(voe_scene_camera_add(world, camera, *lens));
	if (lit) {
		voe_scene_transform sun = placed(0.0f, 0.0f, 0.0f);

		sun.rotation = voe_scene_light_facing(
			(voe_math_float3){ -0.4f, -1.0f, -0.6f });
		VOE_TEST_CHECK(voe_ecs_entity_create(world, &light));
		VOE_TEST_CHECK(voe_scene_transform_add(world, light, sun));
		VOE_TEST_CHECK(voe_scene_light_add(
			world, light,
			(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
					   .intensity = 3.0f,
					   .cast_shadows = true }));
	}
	VOE_TEST_CHECK(
		voe_scene_transform_add(world, cube, placed(0.0f, 0.0f, 0.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY,
				.cast_shadows = true }));
	VOE_TEST_CHECK(voe_physics_collider_add(
		world, cube,
		(voe_physics_collider){ .kind = VOE_PHYSICS_COLLIDER_BOX,
					.size = { 1.0f, 1.0f, 1.0f } }));
	return cube;
}

// A fresh world built `lit` or not, two frames of it, captured, checked on
// disk and removed.
static void draw_case(voe_app *app, voe_base_arena *arena,
		      voe_base_arena *scratch, const voe_3d_shapes *shapes,
		      bool lit)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_base_error error = VOE_BASE_OK;
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_ecs_entity cube = build(world, lit);
	const voe_physics_collider *collider;
	FILE *file;

	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_physics_collider_submit(
		world, (voe_physics_collider_intent){
			       .entity = cube,
			       .collider = { .kind = VOE_PHYSICS_COLLIDER_BOX,
					     .size = { 2.0f, 1.0f, 1.0f } } }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	collider = voe_physics_collider_get(world, cube);
	VOE_TEST_CHECK(collider != NULL && collider->size.x == 2.0f);

	VOE_TEST_CHECK(voe_app_capture_png(app, VOE_RENDER_TARGET_WINDOW,
					   scratch, CAPTURE_PATH, &error));

	// The MSVC runtime deprecates fopen and -Werror makes that an error;
	// the file is read by something that is not the platform that wrote it.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	file = fopen(CAPTURE_PATH, "rb");
#pragma clang diagnostic pop
	VOE_TEST_CHECK(file != NULL);
	if (file != NULL)
		fclose(file);
	remove(CAPTURE_PATH);
}

// A fresh world built `lit` or not with a capsule two metres over the cube,
// two frames of it, both true: the shadow passes and their casters fit.
static void shadow_case(voe_app *app, voe_base_arena *arena,
			voe_base_arena *scratch, const voe_3d_shapes *shapes,
			bool lit)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_ecs_entity capsule;

	(void)build(world, lit);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &capsule));
	VOE_TEST_CHECK(voe_scene_transform_add(world, capsule,
					       placed(0.0f, 2.0f, 0.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, capsule,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CAPSULE,
				.colour = VOE_3D_SHAPE_GREY,
				.cast_shadows = true }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
}

// A light at the origin facing `toward`, casting and bouncing once.
static void add_bouncing_light(voe_ecs_world *world, voe_math_float3 toward)
{
	voe_scene_transform pose = placed(0.0f, 0.0f, 0.0f);
	voe_ecs_entity light;

	pose.rotation = voe_scene_light_facing(toward);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &light));
	VOE_TEST_CHECK(voe_scene_transform_add(world, light, pose));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, light,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .cast_shadows = true,
				   .bounces = 1,
				   .bounce_strength = 1.0f }));
}

// An unlit world with a capsule over the cube, a sun and a moon from opposite
// sides added, three frames, each true: every light's passes fit.
static void sun_and_moon_case(voe_app *app, voe_base_arena *arena,
			      voe_base_arena *scratch,
			      const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_ecs_entity capsule;

	(void)build(world, false);
	add_bouncing_light(world, (voe_math_float3){ -0.4f, -1.0f, -0.6f });
	add_bouncing_light(world, (voe_math_float3){ 0.4f, -1.0f, 0.6f });
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &capsule));
	VOE_TEST_CHECK(voe_scene_transform_add(world, capsule,
					       placed(0.0f, 2.0f, 0.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, capsule,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CAPSULE,
				.colour = VOE_3D_SHAPE_GREY,
				.cast_shadows = true }));
	for (int frame = 0; frame < 3; frame++)
		VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch,
					      size, 0.0f, NULL, NULL, NULL, NULL));
}

// A fresh lit world with a floor under the cube and a casting lamp beside
// them, two frames, both true: the point-shadow pass and its casters fit.
static void lamp_case(voe_app *app, voe_base_arena *arena,
		      voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_scene_transform flat = placed(0.0f, -1.0f, 0.0f);
	voe_ecs_entity floor, lamp;

	(void)build(world, true);
	flat.scale = (voe_math_float3){ 6.0f, 0.1f, 6.0f };
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &floor));
	VOE_TEST_CHECK(voe_scene_transform_add(world, floor, flat));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, floor,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY,
				.cast_shadows = true }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &lamp));
	VOE_TEST_CHECK(voe_scene_transform_add(world, lamp,
					       placed(1.5f, 0.5f, 0.0f)));
	VOE_TEST_CHECK(voe_scene_point_light_add(
		world, lamp,
		(voe_scene_point_light){ .colour = { 1.0f, 0.8f, 0.6f },
					 .intensity = 2.0f,
					 .range = 5.0f,
					 .falloff = 1.0f,
					 .cast_shadows = true }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
}

// A flat 64-cell landscape, (64 + 1)² heights of nought.
#define LAND_CELLS 64
#define LAND_PATH "Terrain/Flat.landscape"
static const int32_t FLAT[(LAND_CELLS + 1) * (LAND_CELLS + 1)];

// A fresh lit world with a row wearing a 512 m landscape loaded from a table,
// two frames, both true: its nodes in every pass fit VOE_GAME_CAPACITIES.
static void landscape_case(voe_app *app, voe_base_arena *arena,
			   voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	const voe_game_landscapes table = {
		(const voe_game_landscape[]){
			{ LAND_PATH, 512.0f, LAND_CELLS, FLAT, NULL, 0, NULL,
			  NULL, 0, NULL, 0, NULL, NULL } },
		1
	};
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_3d_models *models = voe_3d_models_new();
	voe_ecs_entity land;
	voe_game_models_failures failures;

	failures = voe_game_models_landscapes(models, voe_app_device(app),
					      &table, scratch);
	VOE_TEST_CHECK_INT(failures.count, 0);
	(void)build(world, true);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &land));
	VOE_TEST_CHECK(voe_scene_transform_add(world, land,
					       placed(-256.0f, -1.0f, -256.0f)));
	VOE_TEST_CHECK(voe_3d_model_add(world, land,
					(voe_3d_model){ .path = LAND_PATH,
							.cast_shadows = true }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, models, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, models, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	voe_3d_models_clear(models, voe_app_device(app));
	voe_3d_models_destroy(models);
}

// The camera four metres up looking down, `light_row` shining `toward`, a ground
// whose top is at y = -0.95 and `row` at `at`. The light stands at y = 50,
// outside every blocker, so the sun's mask is 0 (0350 point 2). Returns the
// blocker.
static voe_ecs_entity build_blocked(voe_ecs_world *world,
				    voe_math_float3 toward,
				    voe_scene_light light_row,
				    voe_scene_light_blocker row, float at)
{
	voe_ecs_entity camera, light, ground, blocker;
	voe_scene_transform eye = placed(0.0f, 4.0f, 0.0f);
	voe_scene_transform sun = placed(0.0f, 50.0f, 0.0f);
	voe_scene_transform flat = placed(0.0f, -1.0f, 0.0f);
	const voe_scene_camera *lens = voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_camera_key));

	eye.rotation = (voe_math_quat){ -0.70710678f, 0.0f, 0.0f, 0.70710678f };
	sun.rotation = voe_scene_light_facing(toward);
	flat.scale = (voe_math_float3){ 20.0f, 0.1f, 20.0f };
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_scene_transform_add(world, camera, eye));
	VOE_TEST_CHECK(voe_scene_camera_add(world, camera, *lens));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &light));
	VOE_TEST_CHECK(voe_scene_transform_add(world, light, sun));
	VOE_TEST_CHECK(voe_scene_light_add(world, light, light_row));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &ground));
	VOE_TEST_CHECK(voe_scene_transform_add(world, ground, flat));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, ground,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &blocker));
	VOE_TEST_CHECK(voe_scene_transform_add(world, blocker,
					       placed(0.0f, at, 0.0f)));
	VOE_TEST_CHECK(voe_scene_light_blocker_add(world, blocker, row));
	return blocker;
}

// The window of the last frame, read back; no pixels when it could not be.
static voe_render_picture read_window(voe_app *app, voe_base_arena *scratch)
{
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(voe_render_target_read(voe_app_device(app),
					      VOE_RENDER_TARGET_WINDOW, scratch,
					      &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	return picture;
}

static const uint8_t *pixel_at(voe_render_picture picture, uint32_t column,
			       uint32_t row)
{
	const uint8_t *pixel =
		picture.pixels + ((size_t)row * picture.width + column) * 4;

	printf("pixel %u %u: %u %u %u\n", column, row, pixel[0], pixel[1],
	       pixel[2]);
	return pixel;
}

// The blocked world, two frames with a 2 m replace between them, drained by
// the second's step, read back: black at the centre and half a metre out from
// it, lit at a corner, and plain grey where the box's top edge stands.
static void blocker_case(voe_app *app, voe_base_arena *arena,
			 voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_ecs_entity blocker = build_blocked(
		world, (voe_math_float3){ 0.4f, -1.0f, 0.3f },
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f },
		(voe_scene_light_blocker){ .size = { 1.0f, 1.0f, 1.0f } },
		-1.0f);
	voe_render_picture picture;
	const voe_scene_light_blocker *row;

	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_scene_light_blocker_submit(
		world, (voe_scene_light_blocker_intent){
			       .entity = blocker,
			       .blocker = { .size = { 2.0f, 2.0f, 2.0f } } }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	row = voe_scene_light_blocker_get(world, blocker);
	VOE_TEST_CHECK(row != NULL && row->size.x == 2.0f);

	picture = read_window(app, scratch);
	if (picture.pixels == NULL)
		return;

	// About 12.6 pixels a metre on the ground, 15.6 at the box's top, y = 0:
	// column +9 is 0.7 m out, inside the grown box and outside the first;
	// column -16 is where its top edge at x = -1 stands, over lit ground. Not
	// +16: a Room stops the sun it is crossed by (0350 point 3), and the
	// ground past x = 1 lies in the box's shadow along (0.4, -1, 0.3).
	const uint8_t *centre = pixel_at(picture, WIDTH / 2, HEIGHT / 2);
	const uint8_t *grown = pixel_at(picture, WIDTH / 2 + 9, HEIGHT / 2);
	const uint8_t *corner = pixel_at(picture, 0, 0);
	const uint8_t *edge = pixel_at(picture, WIDTH / 2 - 16, HEIGHT / 2);

	for (int channel = 0; channel < 3; channel++) {
		VOE_TEST_CHECK(centre[channel] <= TOLERANCE);
		VOE_TEST_CHECK(grown[channel] <= TOLERANCE);
		VOE_TEST_CHECK(corner[channel] > 40);
		VOE_TEST_CHECK(edge[channel] > 40);
		VOE_TEST_CHECK(edge[channel] == edge[0]);
	}
}

// A 2 × 0.5 × 2 Direct blocker floating at y = 1 under a sun along (1, -1, 0) with a
// fill, two frames, read back. Its shadow on the ground runs x 0.7 to 3.2,
// z -1 to 1: column +24 sees x ≈ 1.9 in it, column 20 x ≈ -3.5 clear of it,
// and column 41 is where its top edge at x = -1, y = 1.25 stands, about 22.7
// pixels a metre there, over ground the sun reaches. The ground in its shadow
// reads the fill, not black and below the sunlit ground, and its edge is
// plain grey.
static void direct_case(voe_app *app, voe_base_arena *arena,
		      voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_render_picture picture;

	(void)build_blocked(
		world, (voe_math_float3){ 1.0f, -1.0f, 0.0f },
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .fill_colour = { 1.0f, 1.0f, 1.0f },
				   .fill_intensity = 0.3f },
		(voe_scene_light_blocker){ .size = { 2.0f, 0.5f, 2.0f },
					   .block = VOE_SCENE_LIGHT_BLOCKER_DIRECT },
		1.0f);
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	picture = read_window(app, scratch);
	if (picture.pixels == NULL)
		return;

	const uint8_t *shadowed = pixel_at(picture, WIDTH / 2 + 24, HEIGHT / 2);
	const uint8_t *sunlit = pixel_at(picture, 20, HEIGHT / 2);
	const uint8_t *edge = pixel_at(picture, 41, HEIGHT / 2);

	for (int channel = 0; channel < 3; channel++) {
		VOE_TEST_CHECK(shadowed[channel] > TOLERANCE);
		VOE_TEST_CHECK(shadowed[channel] + 40 < sunlit[channel]);
		VOE_TEST_CHECK(edge[channel] > 40);
		VOE_TEST_CHECK(edge[channel] == edge[0]);
	}
}

// The clear colour render/src/pass.c clears to, #17171A as drawn.
#define CLEAR_RED 0x17
#define CLEAR_BLUE 0x1A

// The corner's and the cube centre's channels of one sky case's last frame.
typedef struct {
	uint8_t corner[3];
	uint8_t centre[3];
} sky_read;

// A fresh lit world, with a sky row of 0406's defaults when `sky` and a Soft
// picture row at exposure 0 when `lit`, two frames with a picture replace of
// `exposure` between them when it is not 0, read back.
static sky_read sky_case(voe_app *app, voe_base_arena *arena,
			 voe_base_arena *scratch, const voe_3d_shapes *shapes,
			 bool sky, bool lit, float exposure)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	const voe_scene_sky row = {
		.zenith = { 0.15f, 0.35f, 0.80f },
		.horizon = { 0.60f, 0.75f, 0.90f },
		.ground = { 0.20f, 0.18f, 0.16f },
		.horizon_height = 0.25f,
		.strength = 1.0f,
	};
	const voe_scene_picture look = { .exposure = 0.0f,
					 .tone = VOE_SCENE_PICTURE_TONE_SOFT };
	voe_ecs_entity settings;
	voe_render_picture picture;
	sky_read read = { 0 };

	(void)build(world, true);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	if (sky)
		VOE_TEST_CHECK(voe_scene_sky_add(world, settings, row));
	if (lit)
		VOE_TEST_CHECK(voe_scene_picture_add(world, settings, look));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	if (lit && exposure != 0.0f) {
		voe_scene_picture darker = look;

		darker.exposure = exposure;
		VOE_TEST_CHECK(voe_scene_picture_submit(
			world, (voe_scene_picture_intent){ .entity = settings,
							   .picture = darker }));
	}
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	if (lit) {
		const voe_scene_picture *got =
			voe_scene_picture_get(world, settings);

		VOE_TEST_CHECK(got != NULL && got->exposure == exposure);
	}
	picture = read_window(app, scratch);
	if (picture.pixels == NULL)
		return read;
	memcpy(read.corner, pixel_at(picture, 0, 0), 3);
	memcpy(read.centre, pixel_at(picture, WIDTH / 2, HEIGHT / 2), 3);
	return read;
}

// A sky behind the cube, the exposure replace darkening the cube with and
// without a sky, and neither row leaving the clear colour.
static void sky_cases(voe_app *app, voe_base_arena *arena,
		      voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	const sky_read lit =
		sky_case(app, arena, scratch, shapes, true, true, 0.0f);
	const sky_read dark =
		sky_case(app, arena, scratch, shapes, true, true, -4.0f);
	const sky_read none =
		sky_case(app, arena, scratch, shapes, false, false, 0.0f);
	const sky_read bare =
		sky_case(app, arena, scratch, shapes, false, true, -4.0f);

	VOE_TEST_CHECK(lit.corner[2] > lit.corner[0]);
	VOE_TEST_CHECK(lit.corner[2] > CLEAR_BLUE + TOLERANCE);
	for (int channel = 0; channel < 3; channel++) {
		VOE_TEST_CHECK(dark.centre[channel] < lit.centre[channel]);
		VOE_TEST_CHECK(bare.centre[channel] < none.centre[channel]);
	}
	VOE_TEST_CHECK(none.corner[0] >= CLEAR_RED - TOLERANCE &&
		       none.corner[0] <= CLEAR_RED + TOLERANCE);
	VOE_TEST_CHECK(none.corner[2] >= CLEAR_BLUE - TOLERANCE &&
		       none.corner[2] <= CLEAR_BLUE + TOLERANCE);
}

// A fresh lit world with a thing wearing a path an empty store lacks, two
// frames drawn with that store from voe_3d_models_new, both true, the thing
// drawing as nothing. The other cases pass no store.
static void model_case(voe_app *app, voe_base_arena *arena,
		       voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_3d_models *models = voe_3d_models_new();
	voe_ecs_entity hull;

	(void)build(world, true);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &hull));
	VOE_TEST_CHECK(voe_scene_transform_add(world, hull,
					       placed(0.0f, 2.0f, 0.0f)));
	VOE_TEST_CHECK(voe_3d_model_add(
		world, hull, (voe_3d_model){ .path = "Assets/hull.glb" }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, models, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, models, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_3d_models_count(models) == 0);
	voe_3d_models_destroy(models);
}

// A fresh lit world with a thing at the origin wearing glb_build.h's triangle,
// loaded into a store, two frames drawn through one memory as run.c keeps for
// its window: the thing's row is chosen, so its key is in the memory (0501).
static void a_window_memory_keeps_the_row_lod(voe_app *app,
					      voe_base_arena *arena,
					      voe_base_arena *scratch,
					      const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_3d_models *models = voe_3d_models_new();
	voe_3d_lods_kept *lods = voe_3d_lods_kept_create(arena);
	voe_base_error error = VOE_BASE_OK;
	voe_3d_model_level_choice choice;
	voe_3d_model row = { 0 };
	voe_ecs_entity thing;
	uint8_t glb[512];
	uint32_t length = glb_build(glb, sizeof(glb));

	VOE_TEST_CHECK(voe_3d_models_load(models, voe_app_device(app),
					  "Assets/triangle.glb", 1, glb, length,
					  &error));
	(void)build(world, true);
	thing = place(world);
	snprintf(row.path, sizeof row.path, "%s", "Assets/triangle.glb");
	VOE_TEST_CHECK(voe_3d_model_add(world, thing, row));
	for (int frame = 0; frame < 2; frame++)
		VOE_TEST_CHECK(voe_game_frame(app, world, shapes, models, scratch,
					      size, 0.0f, NULL, lods, NULL,
					      NULL));
	VOE_TEST_CHECK(voe_3d_lods_kept_find(
		lods, voe_3d_lods_kept_row_key(thing), &choice));
	voe_3d_models_clear(models, voe_app_device(app));
	voe_3d_models_destroy(models);
}

// A world with only a Haze row of 1000, height 1000, a replace of 500 drained
// by one world step: voe_3d_draw_system_sky, its eye the origin, reads haze
// 500, haze_top 1000 and `drawn` 0 (0417 point 1, 0419 point 2).
// No shape row, so the zeroed shapes are never read.
static void haze_case(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_ecs_world *world = voe_game_world_new(arena);
	const voe_3d_shapes shapes = { 0 };
	voe_ecs_entity settings;
	voe_render_sky sky;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_haze_add(
		world, settings,
		(voe_scene_haze){
			.haze = 1000.0f, .start = 100.0f, .height = 1000.0f }));
	VOE_TEST_CHECK(voe_scene_haze_submit(
		world, (voe_scene_haze_intent){ .entity = settings,
						.haze = { .haze = 500.0f,
							  .start = 100.0f,
							  .height = 1000.0f } }));
	voe_game_world_step(world, &shapes);
	sky = voe_3d_draw_system_sky(world, (voe_math_double3){ 0.0, 0.0, 0.0 });
	VOE_TEST_CHECK(sky.haze == 500.0f);
	VOE_TEST_CHECK(sky.haze_start == 100.0f);
	VOE_TEST_CHECK(sky.haze_top == 1000.0f);
	VOE_TEST_CHECK(sky.drawn == 0);

	voe_base_arena_destroy(arena);
}

// A world with only a Volumetric Clouds row at height 1500:
// voe_3d_draw_system_clouds, its eye the origin, reads volumetric and bottom
// 1500 (0492 point 3).
static void clouds_case(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_scene_clouds row = *(const voe_scene_clouds *)voe_ecs_component_default(
		world, voe_ecs_component_type(world, &voe_scene_clouds_key));
	voe_ecs_entity settings;
	voe_render_clouds clouds;

	row.kind = VOE_SCENE_CLOUDS_VOLUMETRIC;
	row.height = 1500.0f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &settings));
	VOE_TEST_CHECK(voe_scene_clouds_add(world, settings, row));
	clouds = voe_3d_draw_system_clouds(world,
					   (voe_math_double3){ 0.0, 0.0, 0.0 });
	VOE_TEST_CHECK_INT(clouds.kind, VOE_RENDER_CLOUDS_VOLUMETRIC);
	VOE_TEST_CHECK(clouds.bottom == 1500.0f);

	voe_base_arena_destroy(arena);
}

// A world with only a fog volume of thickness 1 on an identified, placed
// entity: voe_3d_fog_volumes, its eye the origin, reads one record of density
// 0.1; with the entity switched off through its identity, none (0493 point 3).
static void fog_case(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 20);
	voe_ecs_world *world = voe_game_world_new(arena);
	const voe_math_double3 origin = { 0.0, 0.0, 0.0 };
	voe_scene_fog_volume row =
		*(const voe_scene_fog_volume *)voe_ecs_component_default(
			world,
			voe_ecs_component_type(world, &voe_scene_fog_volume_key));
	voe_scene_identity identity = { 0 };
	voe_ecs_entity valley;
	voe_render_fog fog;

	row.thickness = 1.0f;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &valley));
	identity.id = valley.index + 1;
	VOE_TEST_CHECK(voe_scene_identity_add(world, valley, identity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, valley,
					       placed(0.0f, 0.0f, 0.0f)));
	VOE_TEST_CHECK(voe_scene_fog_volume_add(world, valley, row));
	fog = voe_3d_fog_volumes(world, origin, 0.0f);
	VOE_TEST_CHECK_INT(fog.count, 1);
	VOE_TEST_CHECK_FLOAT(fog.volumes[0].density, 0.1f, 1e-6f);

	identity.off = true;
	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){ .entity = valley,
						    .identity = identity }));
	voe_scene_identity_system_run(world);
	fog = voe_3d_fog_volumes(world, origin, 0.0f);
	VOE_TEST_CHECK_INT(fog.count, 0);

	voe_base_arena_destroy(arena);
}

// The unlit world with a second cube behind the camera, two frames, the second
// handed a counts record: the cube in front drawn, the one behind skipped.
static void counts_case(voe_app *app, voe_base_arena *arena,
			voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_3d_view_counts counts = { 0 };
	voe_ecs_entity behind;

	(void)build(world, false);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &behind));
	VOE_TEST_CHECK(voe_scene_transform_add(world, behind,
					       placed(0.0f, 0.0f, 10.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, behind,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, NULL));
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size,
				      0.0f, NULL, NULL, NULL, &counts));
	VOE_TEST_CHECK_INT(counts.drawn, 1);
	VOE_TEST_CHECK_INT(counts.skipped, 1);
}

// One label; the run goes on.
static bool labelled(const voe_game_project_frame *frame)
{
	voe_ui_column_begin(frame->ui, (voe_ui_container){ 0 });
	voe_ui_label(frame->ui, "Coins");
	voe_ui_end(frame->ui);
	VOE_TEST_CHECK(voe_ui_frame_end(frame->ui));
	return true;
}

// A fresh lit world with a label drawn over it: the frame comes back true.
static void interface_case(voe_app *app, voe_base_arena *arena,
			   voe_base_arena *scratch, const voe_3d_shapes *shapes)
{
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_world *world = voe_game_world_new(arena);
	voe_game_interface *interface =
		voe_game_interface_new(voe_app_device(app), arena);
	voe_game_project_asks asks = { 0 };

	VOE_TEST_CHECK(interface != NULL);
	if (interface == NULL)
		return;
	(void)build(world, true);
	VOE_TEST_CHECK(voe_game_interface_run(interface, scratch, world, NULL,
					      size, &asks, labelled));
	VOE_TEST_CHECK(voe_ui_element_count(
			       voe_game_interface_context(interface)) > 0);
	VOE_TEST_CHECK(voe_game_frame(app, world, shapes, NULL, scratch, size, 0.0f,
				      NULL, NULL,
				      voe_game_interface_context(interface), NULL));
	voe_game_interface_destroy(interface);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1 << 23);
	voe_base_arena *scratch = voe_base_arena_new(1 << 20);
	voe_app_settings settings = { .width = WIDTH,
				      .height = HEIGHT,
				      .capacities = VOE_GAME_CAPACITIES,
				      .longest_step = 0.25 };
	voe_base_error error = VOE_BASE_OK;
	voe_3d_shapes shapes;
	voe_app *app;

	haze_case();
	clouds_case();
	fog_case();
	app =voe_app_new_headless(arena, scratch, settings, &error);
	if (app == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(app != NULL);
		voe_base_arena_destroy(scratch);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	voe_base_arena_clear(scratch);

	VOE_TEST_CHECK(voe_3d_shapes_upload(voe_app_device(app), &shapes,
					    &error));

	draw_case(app, arena, scratch, &shapes, true);
	draw_case(app, arena, scratch, &shapes, false);
	shadow_case(app, arena, scratch, &shapes, true);
	shadow_case(app, arena, scratch, &shapes, false);
	sun_and_moon_case(app, arena, scratch, &shapes);
	lamp_case(app, arena, scratch, &shapes);
	landscape_case(app, arena, scratch, &shapes);
	blocker_case(app, arena, scratch, &shapes);
	direct_case(app, arena, scratch, &shapes);
	sky_cases(app, arena, scratch, &shapes);
	interface_case(app, arena, scratch, &shapes);
	model_case(app, arena, scratch, &shapes);
	a_window_memory_keeps_the_row_lod(app, arena, scratch, &shapes);
	counts_case(app, arena, scratch, &shapes);

	voe_app_destroy(app);
	voe_base_arena_destroy(scratch);
	voe_base_arena_destroy(arena);
	remove(CAPTURE_PATH);
	return voe_test_result();
}
