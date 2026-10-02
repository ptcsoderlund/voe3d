// The point light marker: every edge built seen from off every circle's plane,
// nothing on a picture with no area, a ray meeting its cube head-on and missing
// beside it, and voe_3d_pick answering a lamp under the ray and the nearer of a
// lamp and a cube. Needs no graphics card: the marker and the pick are
// arithmetic into an arena.
#include <3d/pick.h>
#include <3d/point_light_marker.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <scene/camera_component.h>
#include <scene/point_light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

// The shapes' geometry is a couple of hundred kilobytes (3d/shape_geometry.h).
#define SCRATCH (4 * 1024 * 1024)

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

// A lamp at (1, 0.7, 0) puts EYE in none of its three circles' planes, so no
// edge is seen end-on and every one is a quad.
static void a_lamp_off_axis_builds_every_edge(voe_base_arena *arena)
{
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(voe_3d_point_light_marker_quads(
		(voe_math_double3){ 1.0, 0.7, 0.0 }, the_view(), EYE,
		(voe_platform_size){ WIDTH, HEIGHT }, 2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(VOE_3D_POINT_LIGHT_MARKER_EDGES, 3 * 12);
	VOE_TEST_CHECK_INT(mesh.vertex_count,
			   VOE_3D_POINT_LIGHT_MARKER_EDGES * 4);
	VOE_TEST_CHECK_INT(mesh.index_count, VOE_3D_POINT_LIGHT_MARKER_INDICES);
	for (uint32_t i = 0; i < mesh.index_count; i++)
		VOE_TEST_CHECK(mesh.indices[i] < mesh.vertex_count);
}

static void a_picture_with_no_area_builds_nothing(voe_base_arena *arena)
{
	voe_3d_outline_mesh mesh = { 0 };

	VOE_TEST_CHECK(!voe_3d_point_light_marker_quads(
		(voe_math_double3){ 1.0, 0.7, 0.0 }, the_view(), EYE,
		(voe_platform_size){ 0, 0 }, 2.0f, arena, &mesh));
	VOE_TEST_CHECK_INT(mesh.vertex_count, 0);
}

// The cube's near face is 0.25 m short of the lamp; half a metre aside misses.
static void a_ray_hits_the_cube_head_on_and_misses_beside_it(void)
{
	float distance = 0.0f;

	VOE_TEST_CHECK(voe_3d_point_light_marker_hit(
		(voe_math_double3){ 0.0, 0.0, 0.0 }, down_minus_z_from(0.0),
		&distance));
	VOE_TEST_CHECK_FLOAT(distance, 5.0f - 0.25f, 1e-4f);
	VOE_TEST_CHECK(!voe_3d_point_light_marker_hit(
		(voe_math_double3){ 0.0, 0.0, 0.0 }, down_minus_z_from(0.5),
		NULL));
}

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = { .entities = 8,
				  .component_types = 8,
				  .intent_types = 8,
				  .structure_requests = 8,
				  .structure_bytes = 256 };
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 8);
	voe_3d_shape_register(world, 8);
	voe_scene_point_light_register(world, 8);
	return world;
}

static voe_ecs_entity add_a_lamp(voe_ecs_world *world, float z)
{
	voe_ecs_entity lamp = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &lamp));
	VOE_TEST_CHECK(voe_scene_transform_add(world, lamp, at(0.0f, 0.0f, z)));
	VOE_TEST_CHECK(voe_scene_point_light_add(
		world, lamp,
		(voe_scene_point_light){ .colour = { 1.0f, 1.0f, 1.0f },
					 .intensity = 1.0f,
					 .range = 5.0f }));
	return lamp;
}

static voe_ecs_entity add_a_cube(voe_ecs_world *world, float z)
{
	voe_ecs_entity cube = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, at(0.0f, 0.0f, z)));
	VOE_TEST_CHECK(voe_3d_shape_add(
		world, cube,
		(voe_3d_shape){ .kind = VOE_3D_SHAPE_CUBE,
				.colour = VOE_3D_SHAPE_GREY }));
	return cube;
}

// A lamp alone is picked on its cube; before a cube at the origin it wins at
// 5 - 2.25, and behind one it loses to the cube's face at 4.5.
static void the_pick_answers_the_nearer_lamp_or_cube(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	float distance = -1.0f;
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity lamp = add_a_lamp(world, 0.0f);
	voe_ecs_entity hit = voe_3d_pick(world, geometries, NULL,
					 down_minus_z_from(0.0), &distance);

	VOE_TEST_CHECK_INT(hit.index, lamp.index);
	VOE_TEST_CHECK_INT(hit.generation, lamp.generation);
	VOE_TEST_CHECK_FLOAT(distance, 4.75f, 1e-3f);

	world = a_world(arena);
	(void)add_a_cube(world, 0.0f);
	lamp = add_a_lamp(world, 2.0f);
	hit = voe_3d_pick(world, geometries, NULL, down_minus_z_from(0.0),
			  &distance);
	VOE_TEST_CHECK_INT(hit.index, lamp.index);
	VOE_TEST_CHECK_FLOAT(distance, 5.0f - 2.25f, 1e-3f);

	world = a_world(arena);
	(void)add_a_lamp(world, -2.0f);
	voe_ecs_entity cube = add_a_cube(world, 0.0f);
	hit = voe_3d_pick(world, geometries, NULL, down_minus_z_from(0.0),
			  &distance);
	VOE_TEST_CHECK_INT(hit.index, cube.index);
	VOE_TEST_CHECK_FLOAT(distance, 4.5f, 1e-3f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shape_geometries geometries;

	voe_3d_shape_geometries_create(arena, &geometries);

	a_lamp_off_axis_builds_every_edge(arena);
	a_picture_with_no_area_builds_nothing(arena);
	a_ray_hits_the_cube_head_on_and_misses_beside_it();
	the_pick_answers_the_nearer_lamp_or_cube(arena, &geometries);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
