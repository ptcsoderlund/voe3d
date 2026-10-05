// A frame carries every directional light (0357 point 1), through
// voe_3d_draw_system_frame, voe_3d_draw_system_lights and
// voe_3d_draw_system_camera. Needs no graphics card: none of them takes a
// device. Its worlds are built as 3d/tests/light_blockers.c builds its own.
//
// It claims five things. One light frames `more_count` 0 and the light as
// voe_3d_draw_system_light gives it. A sun then a moon: `light` is the sun's,
// and `more_lights[0]` the moon's direction, colour, intensity, fill colour
// times fill intensity, bounces and bounce strength, its shadow zeroed. Five
// lights keep three after the first, the 2nd to the 4th in table order. A
// Room, an All blocker, about the moon gives the moon's `blockers` its bit
// while the sun, standing outside it, has none in `sun`. And the pass camera
// from voe_3d_draw_system_camera carries `more` and the rest of the frame.
#include <3d/draw_system.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_blocker_component.h>
#include <scene/light_blocker_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define SCRATCH (1024 * 1024)
#define LIGHTS 5

static const voe_scene_transform UNMOVED = { .rotation = { 0.0f, 0.0f, 0.0f,
							   1.0f },
					     .scale = { 1.0f, 1.0f, 1.0f } };

// A world with a camera at the origin and room for five lights and a blocker.
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
	voe_ecs_entity camera = { 0 };

	voe_scene_transform_register(world, 8);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, LIGHTS);
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

// A light of `row` at `at`, shining `toward`.
static void light_at(voe_ecs_world *world, voe_scene_light row,
		     voe_math_double3 at, voe_math_float3 toward)
{
	voe_ecs_entity light = { 0 };
	voe_scene_transform where = UNMOVED;

	where.position = at;
	where.rotation = voe_scene_light_facing(toward);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &light));
	VOE_TEST_CHECK(voe_scene_transform_add(world, light, where));
	VOE_TEST_CHECK(voe_scene_light_add(world, light, row));
}

static const voe_scene_light SUN = { .colour = { 1.0f, 1.0f, 1.0f },
				     .intensity = 3.0f };

static const voe_scene_light MOON = { .colour = { 0.5f, 0.6f, 1.0f },
				      .intensity = 0.5f,
				      .fill_colour = { 1.0f, 0.5f, 0.0f },
				      .fill_intensity = 0.4f,
				      .bounces = 2,
				      .bounce_strength = 0.75f };

// The frame of `world`, its blockers and its further lights filled.
static voe_3d_frame framed(voe_ecs_world *world, voe_base_arena *arena)
{
	voe_3d_frame frame = voe_3d_draw_system_frame(
		world, (voe_platform_size){ 640, 480 }, 0.0f);

	VOE_TEST_CHECK(frame.more_lights == NULL);
	VOE_TEST_CHECK_INT(frame.more_count, 0);
	VOE_TEST_CHECK(voe_3d_draw_system_light_blockers(world, &frame, arena));
	VOE_TEST_CHECK(voe_3d_draw_system_lights(world, &frame, arena));
	return frame;
}

static void one_light_has_none_more(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_3d_frame frame;
	voe_render_light first;

	light_at(world, SUN, (voe_math_double3){ 0.0, 50.0, 0.0 },
		 (voe_math_float3){ 0.0f, -1.0f, 0.0f });
	frame = framed(world, arena);
	first = voe_3d_draw_system_light(world);

	VOE_TEST_CHECK_INT(frame.more_count, 0);
	VOE_TEST_CHECK(frame.more_lights == NULL);
	VOE_TEST_CHECK_FLOAT(frame.light.direction.y, -1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(frame.light.direction.y, first.direction.y, 0.0f);
	VOE_TEST_CHECK_FLOAT(frame.light.intensity, 3.0f, 0.0f);
}

static void a_moon_rides_after_the_sun(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_3d_frame frame;

	light_at(world, SUN, (voe_math_double3){ 0.0, 50.0, 0.0 },
		 (voe_math_float3){ 0.0f, -1.0f, 0.0f });
	light_at(world, MOON, (voe_math_double3){ 0.0, 50.0, 0.0 },
		 (voe_math_float3){ 1.0f, 0.0f, 0.0f });
	frame = framed(world, arena);

	VOE_TEST_CHECK_FLOAT(frame.light.direction.y, -1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(frame.light.intensity, 3.0f, 0.0f);
	VOE_TEST_CHECK_INT(frame.more_count, 1);
	if (frame.more_count != 1)
		return;

	voe_render_directional_light moon = frame.more_lights[0];

	VOE_TEST_CHECK_FLOAT(moon.light.direction.x, 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(moon.light.direction.y, 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(moon.light.direction.z, 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(moon.light.colour.x, 0.5f, 0.0f);
	VOE_TEST_CHECK_FLOAT(moon.light.colour.y, 0.6f, 0.0f);
	VOE_TEST_CHECK_FLOAT(moon.light.colour.z, 1.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(moon.light.intensity, 0.5f, 0.0f);
	VOE_TEST_CHECK_FLOAT(moon.light.fill.x, 0.4f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(moon.light.fill.y, 0.2f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(moon.light.fill.z, 0.0f, 1e-6f);
	VOE_TEST_CHECK_INT(moon.light.unshaded, 0);
	VOE_TEST_CHECK_INT(moon.bounces, 2);
	VOE_TEST_CHECK_FLOAT(moon.bounce_strength, 0.75f, 0.0f);
	VOE_TEST_CHECK_INT(moon.shadow.count, 0);
	VOE_TEST_CHECK_INT(moon.blockers, 0u);
}

// Five lights of intensities 1 to 5: the 2nd to the 4th ride, the 5th is out.
static void five_lights_keep_three(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_3d_frame frame;

	for (int i = 1; i <= LIGHTS; i++) {
		voe_scene_light row = SUN;

		row.intensity = (float)i;
		light_at(world, row, (voe_math_double3){ 0.0, 50.0, 0.0 },
			 (voe_math_float3){ 0.0f, -1.0f, 0.0f });
	}
	frame = framed(world, arena);

	VOE_TEST_CHECK_FLOAT(frame.light.intensity, 1.0f, 0.0f);
	VOE_TEST_CHECK_INT(frame.more_count, 3);
	if (frame.more_count != 3)
		return;
	for (uint32_t i = 0; i < 3; i++)
		VOE_TEST_CHECK_FLOAT(frame.more_lights[i].light.intensity,
				     (float)(i + 2), 0.0f);
}

// A 2 m Room about the moon at (10, 0, 0); the sun stands at (0, 50, 0).
static void a_room_holds_the_moon_only(voe_base_arena *arena)
{
	voe_ecs_world *world = world_of(arena);
	voe_ecs_entity room = { 0 };
	voe_scene_transform where = UNMOVED;
	voe_3d_frame frame;

	light_at(world, SUN, (voe_math_double3){ 0.0, 50.0, 0.0 },
		 (voe_math_float3){ 0.0f, -1.0f, 0.0f });
	light_at(world, MOON, (voe_math_double3){ 10.0, 0.5, 0.0 },
		 (voe_math_float3){ 1.0f, 0.0f, 0.0f });
	where.position = (voe_math_double3){ 10.0, 0.0, 0.0 };
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &room));
	VOE_TEST_CHECK(voe_scene_transform_add(world, room, where));
	VOE_TEST_CHECK(voe_scene_light_blocker_add(
		world, room,
		(voe_scene_light_blocker){ .size = { 2.0f, 2.0f, 2.0f },
					   .block = VOE_SCENE_LIGHT_BLOCKER_ALL }));
	frame = framed(world, arena);

	VOE_TEST_CHECK_INT(frame.blockers.count, 1);
	VOE_TEST_CHECK_INT(frame.blockers.sun, 0u);
	VOE_TEST_CHECK_INT(frame.more_count, 1);
	if (frame.more_count == 1)
		VOE_TEST_CHECK_INT(frame.more_lights[0].blockers, 1u);

	// The pass camera carries the moon beside the rest of the frame.
	voe_render_pass_camera camera = voe_3d_draw_system_camera(&frame);

	VOE_TEST_CHECK(camera.more.lights == frame.more_lights);
	VOE_TEST_CHECK_INT(camera.more.count, 1);
	VOE_TEST_CHECK(camera.blockers.blockers == frame.blockers.blockers);
	VOE_TEST_CHECK_INT(camera.blockers.count, 1);
	VOE_TEST_CHECK_FLOAT(camera.light.intensity, 3.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(camera.view.projection.m[0][0],
			     frame.view.projection.m[0][0], 0.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	one_light_has_none_more(arena);
	a_moon_rides_after_the_sun(arena);
	five_lights_keep_three(arena);
	a_room_holds_the_moon_only(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
