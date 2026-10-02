// A frame carries the world's point lights (0320 point 6), through
// voe_3d_draw_system_point_lights.
//
// THE TABLE INTO THE PASS'S LIGHTS, NO GRAPHICS CARD: a steady light at
// (10, 2, -3) seen from an eye at (4, 0, 1) is at (6, 2, -4) with its range, its
// falloff of 1 and its colour times its intensity; a light of intensity 0 is
// left out; a light of falloff 2.5 gives a record of falloff 2.5; a light with
// no transform is left out; a light 1 m over a parent moved to (20, 0, 0) is at
// (20, 1, 0) less the eye; a world with no point light table fills none and
// returns true; and an arena of 64-byte blocks still holds them all, because
// base's arena push gets a block of its own rather than fails (base/arena.h) —
// the false return the header names has no path today.
//
// AND ONE PICTURE: a camera four metres over a grey ground cube, a sun of
// intensity 0 and no fill, one white point light half a metre over the ground
// with a range of 2 m. Drawn through _frame, _point_lights and _run with
// `.points` in the pass camera, the centre is lit and a corner, 4 m off, black.
// It needs a graphics card and skips with a reason without one, as
// 3d/tests/draw_water.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/structure.h>
#include <ecs/world.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/parent_system.h>
#include <scene/point_light_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdio.h>

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32
// A channel at most this is black: a driver's rounding into an sRGB target.
#define TOLERANCE 3

static const voe_scene_transform UNMOVED = { .rotation = { 0.0f, 0.0f, 0.0f,
							   1.0f },
					     .scale = { 1.0f, 1.0f, 1.0f } };

static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 8);
	voe_scene_parent_register(world, 8);
	voe_scene_point_light_register(world, 8);
	return world;
}

// A lamp, placed at `at` unless `placed` is false.
static voe_ecs_entity lamp_of(voe_ecs_world *world, voe_scene_point_light light,
			      bool placed, voe_math_double3 at)
{
	voe_ecs_entity lamp = { 0 };
	voe_scene_transform where = UNMOVED;

	where.position = at;
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &lamp));
	if (placed)
		VOE_TEST_CHECK(voe_scene_transform_add(world, lamp, where));
	VOE_TEST_CHECK(voe_scene_point_light_add(world, lamp, light));
	return lamp;
}

static void check_light(voe_render_point_light light, voe_math_float3 position,
			float range, float falloff, voe_math_float3 colour)
{
	VOE_TEST_CHECK_FLOAT(light.falloff, falloff, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.position.x, position.x, 1e-5f);
	VOE_TEST_CHECK_FLOAT(light.position.y, position.y, 1e-5f);
	VOE_TEST_CHECK_FLOAT(light.position.z, position.z, 1e-5f);
	VOE_TEST_CHECK_FLOAT(light.range, range, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.colour.x, colour.x, 1e-6f);
	VOE_TEST_CHECK_FLOAT(light.colour.y, colour.y, 1e-6f);
	VOE_TEST_CHECK_FLOAT(light.colour.z, colour.z, 1e-6f);
}

// Steady, dark, soft, unplaced and parented lights, in that table order.
static void the_table_becomes_the_passs_lights(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_scene_point_light steady = { .colour = { 1.0f, 0.8f, 0.5f },
					 .intensity = 2.0f,
					 .range = 3.0f,
					 .falloff = 1.0f };
	voe_scene_point_light dark = steady;
	voe_scene_point_light soft = steady;
	voe_ecs_entity parent = { 0 };
	voe_ecs_entity child;
	voe_scene_transform moved = UNMOVED;
	voe_3d_frame frame = { .eye = { 4.0, 0.0, 1.0 } };
	voe_math_float3 lit = { 2.0f, 1.6f, 1.0f };

	dark.intensity = 0.0f;
	soft.falloff = 2.5f;
	(void)lamp_of(world, steady, true, (voe_math_double3){ 10.0, 2.0, -3.0 });
	(void)lamp_of(world, dark, true, (voe_math_double3){ 1.0, 0.0, 0.0 });
	(void)lamp_of(world, soft, true, (voe_math_double3){ 0.0, 0.0, 0.0 });
	(void)lamp_of(world, steady, false, (voe_math_double3){ 0 });

	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.points.count, 2);
	if (frame.points.count == 2) {
		check_light(frame.points.lights[0],
			    (voe_math_float3){ 6.0f, 2.0f, -4.0f }, 3.0f, 1.0f,
			    lit);
		check_light(frame.points.lights[1],
			    (voe_math_float3){ -4.0f, 0.0f, -1.0f }, 3.0f, 2.5f,
			    lit);
	}

	// A lamp 1 m over a parent at the origin, then the parent moved.
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &parent));
	VOE_TEST_CHECK(voe_scene_transform_add(world, parent, UNMOVED));
	child = lamp_of(world, steady, true, (voe_math_double3){ 0.0, 1.0, 0.0 });
	VOE_TEST_CHECK(voe_scene_parent_set(world, child, parent));
	voe_ecs_structure_apply(world);
	voe_scene_transform_system_run(world);
	moved.position = (voe_math_double3){ 20.0, 0.0, 0.0 };
	VOE_TEST_CHECK(voe_scene_transform_submit(
		world, (voe_scene_transform_intent){ .entity = parent,
						     .transform = moved }));
	voe_scene_transform_system_run(world);
	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.points.count, 3);
	if (frame.points.count == 3)
		check_light(frame.points.lights[2],
			    (voe_math_float3){ 16.0f, 1.0f, -1.0f }, 3.0f, 1.0f,
			    lit);
}

// No table: none, and true. A small-block arena: all of them, and true.
static void no_table_and_a_small_arena(voe_base_arena *arena)
{
	voe_ecs_limits limits = { .entities = 4,
				  .component_types = 4,
				  .intent_types = 4 };
	voe_ecs_world *bare = voe_ecs_world_new(arena, limits);
	voe_ecs_world *world = world_of(arena);
	voe_base_arena *small = voe_base_arena_new(64);
	voe_scene_point_light steady = { .colour = { 1.0f, 1.0f, 1.0f },
					 .intensity = 1.0f,
					 .range = 5.0f,
					 .falloff = 1.0f };
	voe_3d_frame frame = { 0 };

	voe_scene_transform_register(bare, 4);
	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(bare, &frame, arena));
	VOE_TEST_CHECK_INT(frame.points.count, 0);
	VOE_TEST_CHECK(frame.points.lights == NULL);

	for (int i = 0; i < 4; i++)
		(void)lamp_of(world, steady, true,
			      (voe_math_double3){ (double)i, 0.0, 0.0 });
	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, small));
	VOE_TEST_CHECK_INT(frame.points.count, 4);
	voe_base_arena_destroy(small);
}

// The camera four metres up looking down, a sun of nothing, the ground cube
// whose top is at y = -0.95, and a white lamp half a metre over it.
static voe_ecs_world *a_lit_world(voe_base_arena *arena,
				  const voe_3d_shapes *shapes)
{
	voe_ecs_limits limits = {
		.entities = 8,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };
	voe_scene_transform ground = { .position = { 0.0, -1.0, 0.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 20.0f, 0.1f, 20.0f } };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_scene_point_light_register(world, 1);
	voe_3d_mesh_register(world, 4);
	voe_3d_material_register(world, 4);
	voe_3d_panel_register(world, 1);
	voe_3d_shape_register(world, 4);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { 0.0, 4.0, 0.0 },
			.rotation = { -0.70710678f, 0.0f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.4f, -1.0f, 0.3f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f } }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, ground));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, entity,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = { 0.5f, 0.5f, 0.5f } }));
	voe_3d_shape_system_run(world, shapes);

	(void)lamp_of(world,
		      (voe_scene_point_light){ .colour = { 1.0f, 1.0f, 1.0f },
					       .intensity = 2.0f,
					       .range = 2.0f,
					       .falloff = 1.0f },
		      true, (voe_math_double3){ 0.0, -0.45, 0.0 });
	return world;
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

static void a_point_light_lights_the_ground(voe_base_arena *arena,
					    voe_render_device *device,
					    const voe_3d_shapes *shapes)
{
	voe_ecs_world *world = a_lit_world(arena, shapes);
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;

	VOE_TEST_CHECK_INT(frame.points.count, 0);
	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.points.count, 1);
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return;
	camera = (voe_render_pass_camera){ .view = frame.view,
					   .light = frame.light,
					   .points = frame.points };
	VOE_TEST_CHECK(voe_render_pass_begin(device, VOE_RENDER_TARGET_WINDOW,
					     &camera));
	voe_3d_draw_system_run(world, device, arena, frame);
	voe_render_pass_end(device);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	VOE_TEST_CHECK(voe_render_target_read(device, VOE_RENDER_TARGET_WINDOW,
					      arena, &picture, &error));
	VOE_TEST_CHECK(picture.pixels != NULL);
	if (picture.pixels == NULL)
		return;

	const uint8_t *under = pixel_at(picture, SIDE / 2, SIDE / 2);
	const uint8_t *corner = pixel_at(picture, 0, 0);

	for (int channel = 0; channel < 3; channel++) {
		VOE_TEST_CHECK(under[channel] > 40);
		VOE_TEST_CHECK(corner[channel] <= TOLERANCE);
	}
}

static const voe_render_capacities CAPACITIES = {
	.vertices = VOE_3D_SHAPES_VERTICES,
	.indices = VOE_3D_SHAPES_INDICES,
	.geometries = VOE_3D_SHAPES_GEOMETRIES,
	.objects = 1,
	.shadings = VOE_3D_SHAPES_SHADINGS,
	.passes = 1,
};

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	voe_3d_shapes shapes;

	the_table_becomes_the_passs_lights(arena);
	no_table_and_a_small_arena(arena);

	device = voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));
	a_point_light_lights_the_ground(arena, device, &shapes);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
