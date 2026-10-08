// An entity tree's size and the distance that frames it: a scaled cube's box,
// a parent's box from its child and round a far child, nothing for a bare
// transform, a cube 100 km out to the millimetre, the framing distance for a
// square picture and a tall one, and a landscape's heights' box. Only the
// landscape needs a graphics card, and skips without one; the world is set up
// as 3d/tests/pick.c sets its own.
#include <3d/bounds.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>
#include <ecs/structure.h>
#include <ecs/world.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

// The three shapes' triangles and edges are a couple of hundred kilobytes
// (3d/shape_geometry.h).
#define SCRATCH (4 * 1024 * 1024)

static voe_scene_transform at(double x, double y, double z, float scale)
{
	return (voe_scene_transform){
		.position = { x, y, z },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { scale, scale, scale },
	};
}

static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 16,
		.component_types = 8,
		.intent_types = 8,
		.structure_requests = 8,
		.structure_bytes = 256,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);

	voe_scene_transform_register(world, 16);
	voe_scene_parent_register(world, 16);
	voe_3d_shape_register(world, 8);
	voe_3d_model_register(world, 4);
	return world;
}

// A grey cube, a metre across unscaled (ADR-0191), placed by `transform`.
static voe_ecs_entity add_a_cube(voe_ecs_world *world,
				 voe_scene_transform transform)
{
	voe_ecs_entity entity = { 0 };

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, transform));
	VOE_TEST_CHECK(voe_3d_shape_add(world, entity,
					(voe_3d_shape){
						.kind = VOE_3D_SHAPE_CUBE,
						.colour = VOE_3D_SHAPE_GREY }));
	return entity;
}

// A cube scaled 2 at (10, 0, 0) is two metres across: its centre is its
// position and its radius half the diagonal of a 2 m box, √3.
static void a_scaled_cube_gives_its_own_box(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity cube = add_a_cube(world, at(10.0, 0.0, 0.0, 2.0f));
	voe_math_double3 centre = { 0.0, 0.0, 0.0 };
	float radius = 0.0f;

	VOE_TEST_CHECK(voe_3d_bounds(world, geometries, NULL, cube, &centre,
				     &radius));
	VOE_TEST_CHECK_FLOAT((float)centre.x, 10.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT((float)centre.y, 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT((float)centre.z, 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(radius, sqrtf(3.0f), 1e-5f);
}

// A parent with no shape takes its child's box; a cube on the parent and a
// child 10 m along +X give the box round both, 11 m by 1 by 1.
static void a_parent_gives_its_children_s_box(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_math_double3 centre = { 0.0, 0.0, 0.0 };
	float radius = 0.0f;
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity parent = { 0 };
		voe_ecs_entity child =
			add_a_cube(world, at(3.0, 0.0, -10.0, 1.0f));

		VOE_TEST_CHECK(voe_ecs_entity_create(world, &parent));
		VOE_TEST_CHECK(voe_scene_transform_add(
			world, parent, at(0.0, 0.0, -10.0, 1.0f)));
		VOE_TEST_CHECK(voe_scene_parent_set(world, child, parent));
		voe_ecs_structure_apply(world);
		voe_scene_transform_system_run(world);

		VOE_TEST_CHECK(voe_3d_bounds(world, geometries, NULL, parent,
					     &centre, &radius));
		VOE_TEST_CHECK_FLOAT((float)centre.x, 3.0f, 1e-5f);
		VOE_TEST_CHECK_FLOAT((float)centre.z, -10.0f, 1e-5f);
		VOE_TEST_CHECK_FLOAT(radius, sqrtf(3.0f) * 0.5f, 1e-5f);
	}
	{
		voe_ecs_world *world = a_world(arena);
		voe_ecs_entity parent =
			add_a_cube(world, at(0.0, 0.0, 0.0, 1.0f));
		voe_ecs_entity child =
			add_a_cube(world, at(10.0, 0.0, 0.0, 1.0f));

		VOE_TEST_CHECK(voe_scene_parent_set(world, child, parent));
		voe_ecs_structure_apply(world);
		voe_scene_transform_system_run(world);

		VOE_TEST_CHECK(voe_3d_bounds(world, geometries, NULL, parent,
					     &centre, &radius));
		VOE_TEST_CHECK_FLOAT((float)centre.x, 5.0f, 1e-5f);
		VOE_TEST_CHECK_FLOAT(radius, 0.5f * sqrtf(121.0f + 1.0f + 1.0f),
				     1e-4f);
	}
}

// A bare transform has no size: false, and the outputs are left alone.
static void a_bare_transform_has_no_size(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity place = { 0 };
	voe_math_double3 centre = { 7.0, 7.0, 7.0 };
	float radius = -1.0f;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &place));
	VOE_TEST_CHECK(voe_scene_transform_add(world, place,
					       at(1.0, 2.0, 3.0, 1.0f)));
	VOE_TEST_CHECK(!voe_3d_bounds(world, geometries, NULL, place, &centre,
				      &radius));
	VOE_TEST_CHECK_FLOAT((float)centre.x, 7.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(radius, -1.0f, 0.0f);
}

// The same scaled cube 100 km out keeps its centre to a millimetre (ADR-0250).
static void a_far_cube_keeps_its_centre(
	voe_base_arena *arena, const voe_3d_shape_geometries *geometries)
{
	voe_ecs_world *world = a_world(arena);
	voe_ecs_entity cube =
		add_a_cube(world, at(100000.123, -100000.456, 100000.789, 2.0f));
	voe_math_double3 centre = { 0.0, 0.0, 0.0 };
	float radius = 0.0f;

	VOE_TEST_CHECK(voe_3d_bounds(world, geometries, NULL, cube, &centre,
				     &radius));
	VOE_TEST_CHECK(fabs(centre.x - 100000.123) < 1e-3);
	VOE_TEST_CHECK(fabs(centre.y + 100000.456) < 1e-3);
	VOE_TEST_CHECK(fabs(centre.z - 100000.789) < 1e-3);
	VOE_TEST_CHECK_FLOAT(radius, sqrtf(3.0f), 1e-5f);
}

// A landscape's shape is empty, so its heights' box is its size (0379 point 2):
// 16 m of 8 cells, flat but for a 4 m peak in the middle, at (10, 0, 0) is the
// box 16 by 4 by 16 about (10, 2, 0). Loading uploads, so this skips with no
// card.
static void bounds_hold_a_landscape(voe_base_arena *arena,
				    const voe_3d_shape_geometries *geometries)
{
	const voe_render_capacities capacities = {
		.vertices = 16 * 9,
		.indices = 16 * 24,
		.geometries = 16,
		.objects = 1,
		.shadings = 2,
		.passes = 1,
	};
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device = voe_render_device_new_headless(
		arena, (voe_platform_size){ 4, 4 }, capacities, &error);
	voe_assets_landscape land;
	voe_3d_models *models;
	voe_ecs_world *world;
	voe_ecs_entity thing = { 0 };
	voe_3d_model model = { 0 };
	voe_math_double3 centre = { 0.0, 0.0, 0.0 };
	float radius = 0.0f;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		return;
	}
	land = voe_assets_landscape_flat(16.0f, 8, arena);
	land.heights[4 * 9 + 4] = 4.0f;
	models = voe_3d_models_new();
	VOE_TEST_CHECK(voe_3d_models_load_landscape(
		models, device, "Assets/hill.landscape", 1, &land, &error));

	world = a_world(arena);
	snprintf(model.path, sizeof(model.path), "%s", "Assets/hill.landscape");
	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing,
					       at(10.0, 0.0, 0.0, 1.0f)));
	VOE_TEST_CHECK(voe_3d_model_add(world, thing, model));

	VOE_TEST_CHECK(voe_3d_bounds(world, geometries, models, thing, &centre,
				     &radius));
	VOE_TEST_CHECK_FLOAT((float)centre.x, 10.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT((float)centre.y, 2.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT((float)centre.z, 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(radius, 0.5f * sqrtf(16.0f * 16.0f * 2.0f + 16.0f),
			     1e-4f);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
}

// A unit sphere filling a square picture seen at 90 degrees stands √2 away; a
// tall picture's narrower width puts the eye further back.
static void the_distance_frames_the_narrower_side(void)
{
	float square = voe_3d_bounds_distance(1.0f, 1.5707964f, 1.0f,
					      1.0f);

	VOE_TEST_CHECK_FLOAT(square, sqrtf(2.0f), 1e-5f);
	VOE_TEST_CHECK(voe_3d_bounds_distance(1.0f, 1.5707964f, 0.5f,
					      1.0f) > square);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shape_geometries geometries;

	voe_3d_shape_geometries_create(arena, &geometries);

	a_scaled_cube_gives_its_own_box(arena, &geometries);
	a_parent_gives_its_children_s_box(arena, &geometries);
	a_bare_transform_has_no_size(arena, &geometries);
	a_far_cube_keeps_its_centre(arena, &geometries);
	the_distance_frames_the_narrower_side();
	bounds_hold_a_landscape(arena, &geometries);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
