// A scene with no light is framed with the zeroed light, which draws lit
// surfaces black, and one light is framed as itself (ADR-0287). Needs no
// graphics card: voe_3d_draw_system_frame and voe_3d_draw_system_light take no
// device.
//
// It claims three things. A world with a camera, its transform and a light
// table with no rows frames without asserting, not blind, with `unshaded`,
// intensity, colour and fill all nought; voe_3d_draw_system_light on that
// world says the same; and once one light is
// added `unshaded` is zero and direction, intensity and colour are the light's:
// the direction its transform's -Z, so a turn of -pi/2 about X shines straight
// down, and `fill` its fill colour times its fill intensity (ADR-0273).
#include <3d/draw_system.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define SCRATCH (1024 * 1024)

static voe_ecs_world *a_world_with_a_camera(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 4,
		.component_types = 4,
		.intent_types = 4,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_scene_transform pose = {
		.position = { 0.0f, 0.0f, 5.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_scene_camera lens = {
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};
	voe_ecs_entity camera;

	voe_scene_transform_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 2);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(voe_scene_transform_add(world, camera, pose));
	VOE_TEST_CHECK(voe_scene_camera_add(world, camera, lens));
	return world;
}

// Every field a lit surface reads is nought, so it draws black.
static void check_zeroed(voe_render_light light)
{
	VOE_TEST_CHECK_INT(light.unshaded, 0);
	VOE_TEST_CHECK_FLOAT(light.intensity, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.colour.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.colour.y, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.colour.z, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.fill.x, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.fill.y, 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(light.fill.z, 0.0f, 0.0f);
}

static void no_light_frames_the_zeroed_light(voe_ecs_world *world)
{
	voe_3d_frame frame = voe_3d_draw_system_frame(
		world, (voe_platform_size){ 640, 480 }, 0.0f);

	check_zeroed(frame.light);
	VOE_TEST_CHECK(!frame.blind);
	check_zeroed(voe_3d_draw_system_light(world));
}

static void one_light_frames_as_itself(voe_ecs_world *world)
{
	voe_scene_transform turn = {
		.rotation = voe_scene_light_facing(
			(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_scene_light sun = {
		.colour = { 1.0f, 0.5f, 0.25f },
		.intensity = 2.0f,
	};
	voe_ecs_entity entity;
	voe_3d_frame frame;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, turn));
	VOE_TEST_CHECK(voe_scene_light_add(world, entity, sun));
	frame = voe_3d_draw_system_frame(world, (voe_platform_size){ 640, 480 }, 0.0f);

	VOE_TEST_CHECK_INT(frame.light.unshaded, 0);
	VOE_TEST_CHECK_FLOAT(frame.light.direction.y, -1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(frame.light.direction.x, 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(frame.light.intensity, 2.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.colour.x, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.colour.y, 0.5f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.colour.z, 0.25f, 1e-6f);
}

// A world of its own, because a world draws at most one light.
static void a_turned_light_frames_its_turn_and_fill(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world_with_a_camera(arena);
	voe_scene_transform turn = {
		.rotation = { -0.70710678f, 0.0f, 0.0f, 0.70710678f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_scene_light sun = {
		.colour = { 1.0f, 1.0f, 1.0f },
		.intensity = 1.0f,
		.fill_colour = { 1.0f, 0.5f, 0.0f },
		.fill_intensity = 0.4f,
	};
	voe_ecs_entity entity;
	voe_render_light light;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, turn));
	VOE_TEST_CHECK(voe_scene_light_add(world, entity, sun));
	light = voe_3d_draw_system_frame(world, (voe_platform_size){ 640, 480 },
					 0.0f).light;

	VOE_TEST_CHECK_FLOAT(light.direction.x, 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(light.direction.y, -1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(light.direction.z, 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(light.fill.x, 0.4f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(light.fill.y, 0.2f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(light.fill.z, 0.0f, 1e-6f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world_with_a_camera(arena);

	no_light_frames_the_zeroed_light(world);
	one_light_frames_as_itself(world);
	a_turned_light_frames_its_turn_and_fill(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
