// A marked camera or the suns shown is one more draw than none, and a zeroed
// marker is the same as none (0223, 0274). Two suns shown are one draw, one of
// them selected two, and a ray through each sun's place picks it (0360). Places
// shown mark the one bare transform, and not once it has a shape (0365).
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
#include <3d/pick.h>
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>
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

// A sun shining down from `position`, which only places its marker.
static voe_ecs_entity add_a_sun(voe_ecs_world *world, voe_math_double3 position)
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
			.position = position,
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(world, sun, light));
	return sun;
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
	VOE_TEST_CHECK(!frame.sun.shown);
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

// A marker on a live camera, or the suns shown, is one more draw than none, and
// a marker on a zeroed entity or suns not shown are the same as none (0223,
// 0274). The marked camera is a second one, added after framing, because
// framing wants exactly one camera, and _run reads no camera table.
static void a_marked_camera_is_one_more_draw(void)
{
	voe_base_arena *arena = voe_base_arena_new(GIZMO_SCRATCH);
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_base_error error = VOE_BASE_OK;
	// The shapes' pools, one object for the cube and one for the marker,
	// and one range of this frame's geometry (3d/draw_system.h) — the two
	// suns' lines together, which is more than the camera's.
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 2,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.transient_vertices = 2 * VOE_3D_SUN_MARKER_VERTICES,
		.transient_indices = 2 * VOE_3D_SUN_MARKER_INDICES,
		.transient_geometries = 1,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity cube = { 0 };
	voe_ecs_entity marked = { 0 };
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
	(void)add_a_sun(world, (voe_math_double3){ 0.0, 2.0, -4.0 });
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
	(void)add_a_sun(world, (voe_math_double3){ 0.0, 0.0, -3.0 });
	sun = (voe_3d_sun_marked){
		.shown = true,
		.material = shapes.outline,
		.colour = { 1.0f, 0.0f, 1.0f },
		.selected_colour = { 1.0f, 1.0f, 0.0f },
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
	sun.shown = false;
	VOE_TEST_CHECK_INT(draws_with_a_marker(world, device, arena, frame,
					       marker, sun),
			   without);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

// Two suns two metres either side of the line of sight and five down it: shown
// with neither selected they are one draw more than not shown, with one
// selected two (0360 point 1), and a ray down -Z through each one's place picks
// that one and not the other (0360 point 3). The ray starts past the camera so
// the camera's own box is behind it.
static void every_sun_is_marked_and_picked(void)
{
	voe_base_arena *arena = voe_base_arena_new(GIZMO_SCRATCH);
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_base_error error = VOE_BASE_OK;
	// Two draws, and room for both suns' lines in two ranges.
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 2,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.transient_vertices = 2 * VOE_3D_SUN_MARKER_VERTICES,
		.transient_indices = 2 * VOE_3D_SUN_MARKER_INDICES,
		.transient_geometries = 2,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	const voe_3d_camera_marked no_marker = { 0 };
	const voe_math_double3 places[2] = { { -2.0, 0.0, -5.0 },
					     { 2.0, 0.0, -5.0 } };
	voe_3d_shape_geometries geometries;
	voe_3d_shapes shapes;
	voe_ecs_entity suns[2];
	voe_ecs_world *world;
	voe_3d_frame frame;
	voe_3d_sun_marked marked;
	uint32_t hidden;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));
	voe_3d_shape_geometries_create(arena, &geometries);

	world = a_world(arena);
	voe_3d_shape_register(world, 2);
	add_a_camera(world);
	suns[0] = add_a_sun(world, places[0]);
	suns[1] = add_a_sun(world, places[1]);
	frame = voe_3d_draw_system_frame(world, size, 0.0f);
	marked = (voe_3d_sun_marked){
		.material = shapes.outline,
		.colour = { 1.0f, 0.0f, 1.0f },
		.selected_colour = { 1.0f, 1.0f, 0.0f },
		.pixels = 2.0f,
		.size = size,
	};

	hidden = draws_with_a_marker(world, device, arena, frame, no_marker,
				     marked);
	marked.shown = true;
	VOE_TEST_CHECK_INT(draws_with_a_marker(world, device, arena, frame,
					       no_marker, marked),
			   hidden + 1);
	marked.selected = suns[1];
	VOE_TEST_CHECK_INT(draws_with_a_marker(world, device, arena, frame,
					       no_marker, marked),
			   hidden + 2);

	for (uint32_t i = 0; i < 2; i++) {
		voe_3d_ray ray = {
			.origin = { places[i].x, places[i].y, -2.0 },
			.direction = { 0.0f, 0.0f, -1.0f },
		};
		voe_ecs_entity hit =
			voe_3d_pick(world, &geometries, NULL, ray, NULL);

		VOE_TEST_CHECK_INT(hit.index, suns[i].index);
		VOE_TEST_CHECK_INT(hit.generation, suns[i].generation);
	}

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

// How many commands one frame comes to with `places` set on it and no camera
// marker or sun shown: draws_with_a_marker's count.
static uint32_t draws_with_places(voe_ecs_world *world,
				  voe_render_device *device,
				  voe_base_arena *arena, voe_3d_frame frame,
				  voe_3d_rows_marked places)
{
	const voe_3d_camera_marked no_marker = { 0 };
	const voe_3d_sun_marked no_sun = { 0 };

	VOE_TEST_CHECK(!frame.places.shown);
	frame.places = places;
	return draws_with_a_marker(world, device, arena, frame, no_marker,
				   no_sun);
}

// A camera, a sun and one bare transform four metres down the line of sight:
// places shown is one draw more than zeroed, the bare one marked; selected it
// is still one more, because it is drawn alone and the rest, empty, draws none
// (0365 points 1-2); given a shape it wears no marker and is none more.
static void every_bare_place_is_marked(void)
{
	voe_base_arena *arena = voe_base_arena_new(GIZMO_SCRATCH);
	voe_platform_size size = { GIZMO_SIDE, GIZMO_SIDE };
	voe_base_error error = VOE_BASE_OK;
	// Two draws, and room for one diamond in two ranges.
	voe_render_capacities capacities = {
		.vertices = VOE_3D_SHAPES_VERTICES,
		.indices = VOE_3D_SHAPES_INDICES,
		.geometries = VOE_3D_SHAPES_GEOMETRIES,
		.objects = 2,
		.shadings = VOE_3D_SHAPES_SHADINGS,
		.transient_vertices = VOE_3D_PLACE_MARKER_VERTICES,
		.transient_indices = VOE_3D_PLACE_MARKER_INDICES,
		.transient_geometries = 2,
		.passes = 1,
	};
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, capacities, &error);
	const voe_3d_rows_marked no_places = { 0 };
	voe_3d_shapes shapes;
	voe_ecs_world *world;
	voe_ecs_entity bare = { 0 };
	voe_3d_frame frame;
	voe_3d_rows_marked places;
	uint32_t without;

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return;
	}
	VOE_TEST_CHECK(voe_3d_shapes_upload(device, &shapes, &error));

	world = a_world(arena);
	voe_3d_shape_register(world, 2);
	add_a_camera(world);
	(void)add_a_sun(world, (voe_math_double3){ 0.0, 2.0, -4.0 });
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &bare));
	VOE_TEST_CHECK(voe_scene_transform_add(world, bare, at_depth(4.0f)));
	frame = voe_3d_draw_system_frame(world, size, 0.0f);
	places = (voe_3d_rows_marked){
		.shown = true,
		.material = shapes.outline,
		.colour = { 1.0f, 0.0f, 1.0f },
		.selected_colour = { 1.0f, 1.0f, 0.0f },
		.pixels = 2.0f,
		.size = size,
	};

	without = draws_with_places(world, device, arena, frame, no_places);
	VOE_TEST_CHECK_INT(draws_with_places(world, device, arena, frame,
					     places),
			   without + 1);
	places.selected = bare;
	VOE_TEST_CHECK_INT(draws_with_places(world, device, arena, frame,
					     places),
			   without + 1);
	// The shape's row alone, never run into a mesh, so the entity draws
	// nothing itself and only the missing marker is counted.
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, bare,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
	VOE_TEST_CHECK_INT(draws_with_places(world, device, arena, frame,
					     places),
			   without);

	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	a_marked_camera_is_one_more_draw();
	every_sun_is_marked_and_picked();
	every_bare_place_is_marked();
	return voe_test_result();
}
