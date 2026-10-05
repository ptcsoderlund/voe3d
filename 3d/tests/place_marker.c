// The place marker: every edge built seen off-axis, nothing on a picture with
// no area, a ray meeting its cube head-on and missing beside it, and who wears
// it: a bare transform and a blocker do, a shape, a lamp, a camera and no
// transform do not, and an unregistered table counts as no row. Needs no
// graphics card: the marker is arithmetic into an arena.
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <3d/place_marker.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <3d/water_component.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <scene/camera_system.h>
#include <scene/light_blocker_system.h>
#include <scene/light_system.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define SCRATCH (1024 * 1024)

#define WIDTH 640
#define HEIGHT 480

// Where the_view's eye stands, which its quads are about.
static const voe_math_double3 EYE = { 0.0, 0.0, 5.0 };

static voe_scene_transform at(float x, float y, float z)
{
	return (voe_scene_transform){ .position = { x, y, z },
				      .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				      .scale = { 1.0f, 1.0f, 1.0f } };
}

// Five metres back along +Z, looking down -Z.
static voe_render_view the_view(void)
{
	voe_render_view view = { 0 };

	VOE_TEST_CHECK(voe_3d_view(at(0.0f, 0.0f, 5.0f),
				   (voe_scene_camera){ .fov_y = 1.0471976f,
						       .near_plane = 0.1f,
						       .far_plane = 100.0f },
				   (float)WIDTH / (float)HEIGHT, &view));
	return view;
}

static voe_3d_ray down_minus_z_from(double x)
{
	return (voe_3d_ray){ .origin = { x, 0.0, 5.0 },
			     .direction = { 0.0f, 0.0f, -1.0f } };
}

// A place at (1, 0.7, 0.3) sees no edge end-on, so every one is a quad.
static void a_place_off_axis_builds_every_edge(voe_base_arena *arena)
{
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_place_marker_quads(
		(voe_math_double3){ 1.0, 0.7, 0.3 }, the_view(), EYE,
		(voe_platform_size){ WIDTH, HEIGHT }, 2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, VOE_3D_PLACE_MARKER_VERTICES);
	VOE_TEST_CHECK_INT(mesh.index_count, VOE_3D_PLACE_MARKER_INDICES);
	for (uint32_t i = 0; i < mesh.index_count; i++)
		VOE_TEST_CHECK(mesh.indices[i] < mesh.vertex_count);
}

static void a_picture_with_no_area_builds_nothing(voe_base_arena *arena)
{
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(!voe_3d_place_marker_quads(
		(voe_math_double3){ 1.0, 0.7, 0.3 }, the_view(), EYE,
		(voe_platform_size){ 0, 0 }, 2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, 0);
}

// The cube's near face is 0.25 m short of the place; 0.3 m aside misses.
static void a_ray_hits_the_cube_head_on_and_misses_beside_it(void)
{
	float distance = 0.0f;

	VOE_TEST_CHECK(voe_3d_place_marker_hit((voe_math_double3){ 0.0, 0.0, 0.0 },
					       down_minus_z_from(0.0), &distance));
	VOE_TEST_CHECK_FLOAT(distance, 5.0f - 0.25f, 1e-4f);
	VOE_TEST_CHECK(!voe_3d_place_marker_hit(
		(voe_math_double3){ 0.0, 0.0, 0.0 }, down_minus_z_from(0.3),
		NULL));
}

// A world with every scene table, and 3d's water, mesh and panel only when
// `all` is true.
static voe_ecs_world *a_world(voe_base_arena *arena, bool all)
{
	voe_ecs_limits limits = { .entities = 8,
				  .component_types = 16,
				  .intent_types = 16,
				  .structure_requests = 8,
				  .structure_bytes = 256 };
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 8);
	voe_3d_shape_register(world, 8);
	voe_scene_camera_register(world, 8);
	voe_scene_light_register(world, 8);
	voe_scene_point_light_register(world, 8);
	voe_scene_light_blocker_register(world, 8);
	if (all) {
		voe_3d_water_register(world, 8);
		voe_3d_mesh_register(world, 8);
		voe_3d_panel_register(world, 8);
	}
	return world;
}

static voe_ecs_entity placed_entity(voe_ecs_world *world)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, at(0.0f, 0.0f, 0.0f)));
	return entity;
}

static void who_wears_the_marker(voe_base_arena *arena)
{
	voe_ecs_world *world = a_world(arena, true);
	voe_ecs_entity bare = placed_entity(world);
	voe_ecs_entity blocker = placed_entity(world);
	voe_ecs_entity shape = placed_entity(world);
	voe_ecs_entity lamp = placed_entity(world);
	voe_ecs_entity camera = placed_entity(world);
	voe_ecs_entity loose = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &loose));
	VOE_TEST_CHECK(voe_scene_light_blocker_add(
		world, blocker,
		(voe_scene_light_blocker){ .size = { 1.0f, 1.0f, 1.0f },
					   .block = VOE_SCENE_LIGHT_BLOCKER_ALL }));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, shape,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
	VOE_TEST_CHECK(voe_scene_point_light_add(
		world, lamp,
		(voe_scene_point_light){ .colour = { 1.0f, 1.0f, 1.0f },
					 .intensity = 1.0f,
					 .range = 5.0f,
					 .falloff = 1.0f }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, camera,
		(voe_scene_camera){ .fov_y = 1.0f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

	VOE_TEST_CHECK(voe_3d_place_marker_wanted(world, bare));
	VOE_TEST_CHECK(voe_3d_place_marker_wanted(world, blocker));
	VOE_TEST_CHECK(!voe_3d_place_marker_wanted(world, loose));
	VOE_TEST_CHECK(!voe_3d_place_marker_wanted(world, shape));
	VOE_TEST_CHECK(!voe_3d_place_marker_wanted(world, lamp));
	VOE_TEST_CHECK(!voe_3d_place_marker_wanted(world, camera));

	// No water, mesh or panel table: no row in any of them, and no assert.
	world = a_world(arena, false);
	bare = placed_entity(world);
	VOE_TEST_CHECK(voe_3d_place_marker_wanted(world, bare));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);

	a_place_off_axis_builds_every_edge(arena);
	a_picture_with_no_area_builds_nothing(arena);
	a_ray_hits_the_cube_head_on_and_misses_beside_it();
	who_wears_the_marker(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
