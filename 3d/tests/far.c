// Everything moved 100 km out answers as it did at the origin (ADR-0250): a
// pick, a gizmo grab and the matrix a cube is drawn with. At (100000, 0, 100000)
// a float keeps about 8 mm, so each claim below would miss by that much if any
// world position were narrowed before the eye or the ray's origin was taken off.
//
// It claims three things. A pick through the picture's centre hits the cube it
// hit at the origin, at the same distance within a tenth of a millimetre. A
// gizmo grab a millimetre along X reads back a millimetre within a hundredth of
// one. And a frame's `eye` is the camera's double position, and the cube's drawn
// matrix carries its offset from that eye as its translation. Needs no graphics
// card: the pick, the grab, voe_3d_draw_system_frame and the record a mesh is
// drawn with take no device.
#include "../src/draw_group.h"

#include <3d/draw_system.h>
#include <3d/gizmo.h>
#include <3d/material_component.h>
#include <3d/pick.h>
#include <3d/projection.h>
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>
#include <base/arena.h>
#include <ecs/world.h>
#include <math/double3.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#define WIDTH 640
#define HEIGHT 480
#define SCRATCH (4 * 1024 * 1024)

static const voe_math_double3 FAR = { 100000.0, 0.0, 100000.0 };
static const voe_math_double3 NEAR = { 0.0, 0.0, 0.0 };

// Five metres back along +Z from `where`, which the cube stands on.
static voe_math_double3 eye_of(voe_math_double3 where)
{
	return voe_math_double3_add(where, (voe_math_double3){ 0.0, 0.0, 5.0 });
}

static voe_scene_transform at(voe_math_double3 position)
{
	return (voe_scene_transform){
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
}

static voe_scene_camera the_lens(void)
{
	return (voe_scene_camera){
		.fov_y = 1.0471976f,
		.near_plane = 0.1f,
		.far_plane = 100.0f,
	};
}

static voe_render_view the_view(voe_math_double3 eye)
{
	voe_render_view view = { 0 };

	VOE_TEST_CHECK(voe_3d_view(at(eye), the_lens(),
				   (float)WIDTH / (float)HEIGHT, &view));
	return view;
}

// A world with a camera five metres back from a cube at `where`.
static voe_ecs_world *a_world(voe_base_arena *arena, voe_math_double3 where,
			      voe_ecs_entity *cube)
{
	voe_ecs_limits limits = {
		.entities = 4,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity camera;

	voe_scene_transform_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_shape_register(world, 4);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, *cube, at(where)));
	VOE_TEST_CHECK(voe_3d_shape_add(world, *cube,
					(voe_3d_shape){
						.kind = VOE_3D_SHAPE_CUBE,
						.colour = VOE_3D_SHAPE_GREY }));
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &camera));
	VOE_TEST_CHECK(
		voe_scene_transform_add(world, camera, at(eye_of(where))));
	VOE_TEST_CHECK(voe_scene_camera_add(world, camera, the_lens()));
	return world;
}

// The centre pixel's ray from the eye five metres back from `where`, picked
// against a world with only the cube's shape in it: the camera's own marker
// box would hold the ray's origin (3d/tests/pick.c says the same).
static float centre_pick(voe_base_arena *arena,
			 const voe_3d_shape_geometries *geometries,
			 voe_math_double3 where)
{
	voe_ecs_limits limits = { .entities = 2, .component_types = 4,
				  .intent_types = 4, .structure_requests = 4,
				  .structure_bytes = 128 };
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_platform_size size = { WIDTH, HEIGHT };
	voe_ecs_entity cube;
	voe_ecs_entity hit;
	float distance = -1.0f;

	voe_scene_transform_register(world, 2);
	voe_3d_shape_register(world, 2);
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &cube));
	VOE_TEST_CHECK(voe_scene_transform_add(world, cube, at(where)));
	VOE_TEST_CHECK(voe_3d_shape_add(world, cube,
					(voe_3d_shape){
						.kind = VOE_3D_SHAPE_CUBE,
						.colour = VOE_3D_SHAPE_GREY }));
	hit = voe_3d_pick(world, geometries,
			  voe_3d_pick_ray(the_view(eye_of(where)),
					  eye_of(where), size,
					  (voe_math_float2){ WIDTH / 2.0f,
							     HEIGHT / 2.0f }),
			  &distance);
	VOE_TEST_CHECK_INT(hit.index, cube.index);
	VOE_TEST_CHECK_INT(hit.generation, cube.generation);
	return distance;
}

static void a_far_pick_hits_at_the_same_distance(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	float near_distance = centre_pick(arena, geometries, NEAR);
	float far_distance = centre_pick(arena, geometries, FAR);

	VOE_TEST_CHECK_FLOAT(near_distance, 4.4f, 1e-3f);
	VOE_TEST_CHECK_FLOAT(far_distance, near_distance, 1e-4f);
}

// A ray straight down -Z at a point a millimetre along X from a far gizmo:
// the X grab lands a millimetre from the gizmo's origin.
static void a_far_grab_reads_back_a_millimetre(void)
{
	voe_math_double3 eye = eye_of(FAR);
	voe_3d_gizmo gizmo = voe_3d_gizmo_at(FAR, the_view(eye), eye,
					     (voe_platform_size){ WIDTH, HEIGHT },
					     90.0f);
	voe_3d_ray ray = {
		.origin = voe_math_double3_add(
			FAR, (voe_math_double3){ 0.001, 0.0, 5.0 }),
		.direction = { 0.0f, 0.0f, -1.0f },
	};
	voe_math_double3 grabbed = { 0.0, 0.0, 0.0 };

	VOE_TEST_CHECK(voe_3d_gizmo_grab(gizmo, VOE_3D_GIZMO_X, ray, &grabbed));
	VOE_TEST_CHECK_FLOAT(grabbed.x - FAR.x, 0.001, 1e-5);
	VOE_TEST_CHECK_FLOAT(grabbed.y - FAR.y, 0.0, 1e-5);
	VOE_TEST_CHECK_FLOAT(grabbed.z - FAR.z, 0.0, 1e-5);
}

// The frame's eye is the camera's own double position, and the matrix the
// cube is drawn with moves it by its offset from that eye and nothing more.
static void a_far_frame_is_about_the_camera(voe_base_arena *arena)
{
	voe_ecs_entity cube;
	voe_ecs_world *world = a_world(arena, FAR, &cube);
	voe_3d_frame frame = voe_3d_draw_system_frame(
		world, (voe_platform_size){ WIDTH, HEIGHT });
	voe_3d_material material = { 0 };
	voe_render_object object = voe_3d_draw_group_object_of(
		voe_scene_transform_get(world, cube), &material, NULL,
		frame.eye);

	VOE_TEST_CHECK(!frame.blind);
	VOE_TEST_CHECK_FLOAT(frame.eye.x, FAR.x, 0.0);
	VOE_TEST_CHECK_FLOAT(frame.eye.y, FAR.y, 0.0);
	VOE_TEST_CHECK_FLOAT(frame.eye.z, FAR.z + 5.0, 0.0);
	VOE_TEST_CHECK_FLOAT(object.world.m[0][3], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(object.world.m[1][3], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(object.world.m[2][3], -5.0f, 0.0f);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shape_geometries geometries;

	voe_3d_shape_geometries_create(arena, &geometries);

	a_far_pick_hits_at_the_same_distance(arena, &geometries);
	a_far_grab_reads_back_a_millimetre();
	a_far_frame_is_about_the_camera(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
