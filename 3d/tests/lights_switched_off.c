// A light whose owner is switched off gives no light (0428 point 4), through
// voe_3d_draw_system_light, _lights, _point_lights, _light_blockers and
// voe_3d_draw_light_casts. Needs no graphics card: none of them takes a
// device. Its worlds are built as 3d/tests/directional_lights.c builds its
// own, with the identity table, each light's entity given an identity.
//
// It claims four things. A sun then a moon, the sun off: the first light has
// intensity 0 and fill nought, its direction as ever, `more_lights[0]` is the
// moon's, and only row 1 casts. The sun on and the moon off: the sun as ever,
// `more_lights[0]` of intensity 0 and fill nought, `more_count` still 1, and
// row 1 not casting. Two point lamps, the first off: the frame's points hold
// one, the second's. A light blocker, off: the frame's blockers count 0;
// switched on again: 1.
#include "../src/draw_bounce.h"

#include <3d/draw_system.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/identity_component.h>
#include <scene/identity_system.h>
#include <scene/light_blocker_component.h>
#include <scene/light_blocker_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/point_light_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define SCRATCH (1024 * 1024)
#define ROOM 8

static const voe_scene_transform UNMOVED = { .rotation = { 0.0f, 0.0f, 0.0f,
							   1.0f },
					     .scale = { 1.0f, 1.0f, 1.0f } };

// A world with a camera at the origin and room for lights, lamps, a blocker
// and their identities.
static voe_ecs_world *world_of(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = ROOM,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity camera = { 0 };

	voe_scene_transform_register(world, ROOM);
	voe_scene_identity_register(world, ROOM);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 2);
	voe_scene_point_light_register(world, 2);
	voe_scene_light_blocker_register(world, 1);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_scene_transform_add(world, camera, UNMOVED));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, camera,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));
	return world;
}

// A new entity placed at `at` with identity `id`, switched `off`.
static voe_ecs_entity authored_at(voe_ecs_world *world, voe_scene_transform at,
				  uint64_t id, bool off)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at));
	VOE_TEST_CHECK(voe_scene_identity_add(
		world, entity, (voe_scene_identity){ .id = id, .off = off }));
	return entity;
}

// A light of `row` shining `toward`, switched `off`.
static void light_of(voe_ecs_world *world, voe_scene_light row,
		     voe_math_float3 toward, uint64_t id, bool off)
{
	voe_scene_transform where = UNMOVED;

	where.position = (voe_math_double3){ 0.0, 50.0, 0.0 };
	where.rotation = voe_scene_light_facing(toward);
	VOE_TEST_CHECK(voe_scene_light_add(world,
					   authored_at(world, where, id, off),
					   row));
}

// The entity's switch set to `off` through the identity intent.
static void switch_to(voe_ecs_world *world, voe_ecs_entity entity, bool off)
{
	const voe_scene_identity *read = voe_scene_identity_get(world, entity);
	voe_scene_identity row;

	VOE_TEST_CHECK(read != NULL);
	if (read == NULL)
		return;
	row = *read;
	row.off = off;
	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){ .entity = entity,
						    .identity = row }));
	voe_scene_identity_system_run(world);
}

static const voe_scene_light SUN = { .colour = { 1.0f, 1.0f, 1.0f },
				     .intensity = 3.0f,
				     .fill_colour = { 1.0f, 1.0f, 1.0f },
				     .fill_intensity = 0.2f,
				     .cast_shadows = true };

static const voe_scene_light MOON = { .colour = { 0.5f, 0.6f, 1.0f },
				      .intensity = 0.5f,
				      .fill_colour = { 1.0f, 0.5f, 0.0f },
				      .fill_intensity = 0.4f,
				      .cast_shadows = true };

// The frame of `world`, its blockers and its further lights filled.
static voe_3d_frame framed(voe_ecs_world *world, voe_base_arena *arena)
{
	voe_3d_frame frame = voe_3d_draw_system_frame(
		world, (voe_platform_size){ 640, 480 }, 0.0f);

	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame, arena));
	VOE_TEST_CHECK(voe_3d_draw_system_lights(world, &frame, arena));
	return frame;
}

static void check_dark(voe_render_light light)
{
	VOE_TEST_CHECK_FLOAT(light.intensity, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.fill.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.fill.y, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.fill.z, 0.0f, 0.0f);
}

static void an_off_sun_keeps_its_row(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_render_light sun;
	voe_3d_frame frame;

	light_of(world, SUN, (voe_math_float3){ 0.0f, -1.0f, 0.0f }, 1, true);
	light_of(world, MOON, (voe_math_float3){ 1.0f, 0.0f, 0.0f }, 2, false);
	sun = voe_3d_draw_system_light(world);
	frame = framed(world, arena);

	check_dark(sun);
	check_dark(frame.light);
	VOE_TEST_CHECK_FLOAT(sun.direction.y, -1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(sun.colour.x, 1.0f, 0.0f);
	VOE_TEST_CHECK_INT(frame.more_count, 1);
	if (frame.more_count == 1) {
		VOE_TEST_CHECK_FLOAT(frame.more_lights[0].light.intensity, 0.5f,
				     0.0f);
		VOE_TEST_CHECK_FLOAT(frame.more_lights[0].light.direction.x,
				     1.0f, 1e-5f);
		VOE_TEST_CHECK_FLOAT(frame.more_lights[0].light.fill.x, 0.4f,
				     1e-6f);
	}
	VOE_TEST_CHECK(!voe_3d_draw_light_casts(world, 0));
	VOE_TEST_CHECK(voe_3d_draw_light_casts(world, 1));
}

static void an_off_moon_keeps_its_place(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_3d_frame frame;

	light_of(world, SUN, (voe_math_float3){ 0.0f, -1.0f, 0.0f }, 1, false);
	light_of(world, MOON, (voe_math_float3){ 1.0f, 0.0f, 0.0f }, 2, true);
	frame = framed(world, arena);

	VOE_TEST_CHECK_FLOAT(frame.light.intensity, 3.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(frame.light.fill.x, 0.2f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.direction.y, -1.0f, 1e-5f);
	VOE_TEST_CHECK_INT(frame.more_count, 1);
	if (frame.more_count == 1) {
		check_dark(frame.more_lights[0].light);
		VOE_TEST_CHECK_FLOAT(frame.more_lights[0].light.direction.x,
				     1.0f, 1e-5f);
	}
	VOE_TEST_CHECK(voe_3d_draw_light_casts(world, 0));
	VOE_TEST_CHECK(!voe_3d_draw_light_casts(world, 1));
}

// Lamps at (1, 0, 0), off, and (5, 0, 0), about an eye at the origin.
static void an_off_lamp_is_left_out(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_scene_point_light lamp = { .colour = { 1.0f, 1.0f, 1.0f },
				       .intensity = 2.0f,
				       .range = 3.0f,
				       .falloff = 1.0f,
				       .cast_shadows = true };
	voe_scene_transform where = UNMOVED;
	voe_3d_frame frame = { 0 };

	where.position = (voe_math_double3){ 1.0, 0.0, 0.0 };
	VOE_TEST_CHECK(voe_scene_point_light_add(
		world, authored_at(world, where, 1, true), lamp));
	where.position = (voe_math_double3){ 5.0, 0.0, 0.0 };
	VOE_TEST_CHECK(voe_scene_point_light_add(
		world, authored_at(world, where, 2, false), lamp));

	VOE_TEST_CHECK(voe_3d_draw_system_point_lights(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.points.count, 1);
	if (frame.points.count == 1) {
		VOE_TEST_CHECK_FLOAT(frame.points.lights[0].position.x, 5.0f,
				     1e-5f);
		VOE_TEST_CHECK_INT(frame.points.lights[0].shadow, 1u);
	}
}

// A 2 m All at (10, 0, 0), off and then on.
static void an_off_blocker_is_left_out(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_scene_transform where = UNMOVED;
	voe_ecs_entity blocker;
	voe_3d_frame frame = { 0 };

	where.position = (voe_math_double3){ 10.0, 0.0, 0.0 };
	blocker = authored_at(world, where, 1, true);
	VOE_TEST_CHECK(voe_scene_light_blocker_add(
		world, blocker,
		(voe_scene_light_blocker){ .size = { 2.0f, 2.0f, 2.0f },
					   .block = VOE_SCENE_LIGHT_BLOCKER_ALL }));

	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.blockers.count, 0);

	switch_to(world, blocker, false);
	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame, arena));
	VOE_TEST_CHECK_INT(frame.blockers.count, 1);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	an_off_sun_keeps_its_row(arena);
	an_off_moon_keeps_its_place(arena);
	an_off_lamp_is_left_out(arena);
	an_off_blocker_is_left_out(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
