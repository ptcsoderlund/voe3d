// A scene with no light is framed unshaded, and one light is framed as itself
// (ADR-0238). Needs no graphics card: voe_3d_draw_system_frame and
// voe_3d_draw_system_light take no device.
//
// It claims three things. A world with a camera, its transform and a light
// table with no rows frames without asserting, not blind, with `unshaded` set;
// voe_3d_draw_system_light on that world says the same; and once one light is
// added `unshaded` is zero and direction, intensity and colour are the light's.
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

static void no_light_frames_unshaded(voe_ecs_world *world)
{
	voe_3d_frame frame = voe_3d_draw_system_frame(
		world, (voe_platform_size){ 640, 480 }, 0.0f);
	voe_render_light light = voe_3d_draw_system_light(world);

	VOE_TEST_CHECK(frame.light.unshaded != 0);
	VOE_TEST_CHECK(!frame.blind);
	VOE_TEST_CHECK(light.unshaded != 0);
}

static void one_light_frames_as_itself(voe_ecs_world *world)
{
	voe_scene_light sun = {
		.direction = { 0.0f, -1.0f, 0.0f },
		.colour = { 1.0f, 0.5f, 0.25f },
		.intensity = 2.0f,
	};
	voe_ecs_entity entity;
	voe_3d_frame frame;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_light_add(world, entity, sun));
	frame = voe_3d_draw_system_frame(world, (voe_platform_size){ 640, 480 }, 0.0f);

	VOE_TEST_CHECK_INT(frame.light.unshaded, 0);
	VOE_TEST_CHECK_FLOAT(frame.light.direction.y, -1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.direction.x, 0.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.intensity, 2.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.colour.x, 1.0f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.colour.y, 0.5f, 1e-6f);
	VOE_TEST_CHECK_FLOAT(frame.light.colour.z, 0.25f, 1e-6f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_ecs_world *world = a_world_with_a_camera(arena);

	no_light_frames_unshaded(world);
	one_light_frames_as_itself(world);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
