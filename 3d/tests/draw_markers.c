// A marked camera or sun is one more draw than none, and a zeroed marker is the
// same as none (0223, 0274).
//
// THE DRAW COUNT IS THE MEASUREMENT, BECAUSE A PICTURE SAYS LESS. What is
// observable is voe_render_frame_draw_count — every mesh drawn is one command —
// so "the marker was drawn" is exactly "one command more", and a marker drawn
// when it should not be, or not drawn when it should, is a number that does
// not match.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITHOUT ONE, for the reason
// 3d/tests/import.c gives at length: a box with no Vulkan is the box and not
// this engine.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <3d/shape_component.h>
#include <3d/shape_system.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

// The marker case's own picture and the room it needs beside a device.
#define GIZMO_SCRATCH (4 * 1024 * 1024)
#define GIZMO_SIDE 128

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 16,
		.component_types = 8,
		.intent_types = 8,
		// Only for the shaped case: the shape system queues removals
		// through it (3d/shape_system.h).
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_scene_camera_register(world, 2);
	voe_scene_light_register(world, 2);
	voe_3d_mesh_register(world, 16);
	voe_3d_material_register(world, 16);
	voe_3d_panel_register(world, 16);
	return world;
}

// At the origin, looking along its own -Z, so a thing at a more negative z is
// further away.
static void add_a_camera(voe_ecs_world *world)
{
	voe_ecs_entity eye = { 0 };
	voe_scene_transform pose = {
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_scene_camera lens = {
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &eye));
	VOE_TEST_CHECK(voe_scene_transform_add(world, eye, pose));
	VOE_TEST_CHECK(voe_scene_camera_add(world, eye, lens));
}

static void add_the_sun(voe_ecs_world *world)
{
	voe_ecs_entity sun = { 0 };
	voe_scene_light light = {
		.colour = { 1.0f, 1.0f, 1.0f },
		.intensity = 1.0f,
	};

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &sun));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, sun,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, light));
}

// A transform `back` metres down the camera's line of sight, unrotated and
// unscaled.
static voe_scene_transform at_depth(float back)
{
	voe_scene_transform transform = {
		.position = { 0.0f, 0.0f, -back },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	return transform;
}

// How many commands one frame of `world`, framed as `frame` with `marker` and
// `sun` set on it, comes to.
static uint32_t draws_with_a_marker(voe_ecs_world *world,
				    voe_render_device *device,
				    voe_base_arena *arena, voe_3d_frame frame,
				    voe_3d_camera_marked marker,
				    voe_3d_sun_marked sun)
{
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_render_pass_camera camera = { .view = frame.view,
					  .light = frame.light };
	bool drawing = false;
	uint32_t drawn = 0;

	VOE_TEST_CHECK_INT(frame.marker.entity.generation, 0);
	VOE_TEST_CHECK_INT(frame.sun.entity.generation, 0);
	frame.marker = marker;
	frame.sun = sun;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		drawn = voe_render_frame_draw_count(device);
		voe_render_pass_end(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
	}
	return drawn;
}

// A marker on a live camera, or on a live sun, is one more draw than none, and
// a marker on a zeroed entity is the same as none (0223, 0274). The marked
// camera and sun are second ones, added after framing, because framing wants
// exactly one camera and at most one light, and _run reads neither table.
static void a_marked_camera_is_one_more_draw(void)
{
	voe_base_arena *arena = voe_base_arena_new(GIZMO_SCRATCH);
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_base_error error = VOE_BASE_OK;
	// The shapes' pools, one object for the cube and one for the marker,
	// and one marker's worth of this frame's geometry (3d/draw_system.h) —
	// the sun's, which is the larger of the two.
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 2,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.transient_vertices = VOE_3D_SUN_MARKER_VERTICES,
		.transient_indices = VOE_3D_SUN_MARKER_INDICES,
		.transient_geometries = 1,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity cube = { 0 };
	voe_ecs_entity marked = { 0 };
	voe_ecs_entity shining = { 0 };
	voe_3d_frame frame;
	voe_3d_camera_marked marker;
	voe_3d_sun_marked sun;
	const voe_3d_camera_marked no_marker = { 0 };
	const voe_3d_sun_marked no_sun = { 0 };
	uint32_t without;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	world = a_world(arena);
	voe_3d_shape_register(world, 2);
	add_a_camera(world);
	add_the_sun(world);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, at_depth(6.0f)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
	voe_3d_shape_system_run(world, &shapes);
	frame = voe_3d_draw_system_frame(world, size, 0.0f);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &marked));
	VOE_TEST_CHECK(voe_scene_transform_add(world, marked, at_depth(3.0f)));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, marked,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

	marker = (voe_3d_camera_marked){
		.entity = marked,
		.material = shapes.outline,
		.colour = { 1.0f, 0.0f, 1.0f },
		.pixels = 2.0f,
		.size = size,
	};
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &shining));
	VOE_TEST_CHECK(voe_scene_transform_add(world, shining, at_depth(3.0f)));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, shining,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 1.0f }));
	sun = (voe_3d_sun_marked){
		.entity = shining,
		.material = shapes.outline,
		.colour = { 1.0f, 0.0f, 1.0f },
		.pixels = 2.0f,
		.size = size,
	};

	without = draws_with_a_marker(world, device, arena, frame, no_marker,
				      no_sun);
	VOE_TEST_CHECK_INT(without, 1);
	VOE_TEST_CHECK_INT(draws_with_a_marker(world, device, arena, frame,
					       marker, no_sun),
			   without + 1);
	VOE_TEST_CHECK_INT(draws_with_a_marker(world, device, arena, frame,
					       no_marker, sun),
			   without + 1);
	marker.entity = (voe_ecs_entity){ 0 };
	sun.entity = (voe_ecs_entity){ 0 };
	VOE_TEST_CHECK_INT(draws_with_a_marker(world, device, arena, frame,
					       marker, sun),
			   without);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	a_marked_camera_is_one_more_draw();
	return voe_test_result();
}
