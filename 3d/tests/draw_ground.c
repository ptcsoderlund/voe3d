// A painted landscape row is a ground a model row can blend with (ADR-0481
// points 2, 3 and 5): a 200 m flat landscape of 4 cells, one layer so it is
// painted, worn by a thing at (100, 0, 50), the camera at the origin.
//
// FOUR CLAIMS. The frame has one ground; it is under (100, 5, 50), over the
// landscape's centre, and nothing is under (400, 0, 0), beside it. A record
// filled with it names its heights and paint, its side is 200, and its rows
// take (100, 5, 50) about the eye to (0, 5, 0), the landscape's own centre
// 5 m up. A reach of 20 fills as 10, the most there is, and a reach of 0
// fills nothing.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_terrain.c does.
#include <3d/draw_system.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/panel_component.h>
#include <assets/landscape.h>
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

#include "../src/draw_ground.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define SCRATCH (16u * 1024u * 1024u)
#define SIDE 16
#define PATH "Assets/Ground.landscape"
#define METRES 200.0f

// The shared grid, the ground's record, a node per object, the one pass, and
// room for the paint.
static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33,
	.indices = 32 * 32 * 6,
	.geometries = 1,
	.objects = VOE_3D_LANDSCAPE_NODES,
	.shadings = 8,
	.passes = 1,
	.heights_texels = 64,
};

// The camera at the origin and the thing wearing PATH at (100, 0, 50).
static voe_ecs_world *a_world(voe_base_arena *arena)
{
	voe_ecs_limits limits = {
		.entities = 4,
		.component_types = 16,
		.intent_types = 16,
		.structure_requests = 16,
		.structure_bytes = 1024,
	};
	voe_ecs_world *world = voe_ecs_world_new(arena, limits);
	voe_ecs_entity entity = { 0 };
	voe_3d_model row = { 0 };

	voe_scene_transform_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 1);
	voe_3d_material_register(world, 1);
	voe_3d_panel_register(world, 1);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0f,
				    .near_plane = 0.1f,
				    .far_plane = 1000.0f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .position = { 100.0, 0.0, 50.0 },
				       .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	snprintf(row.path, sizeof(row.path), "%s", PATH);
	VOE_TEST_CHECK(voe_3d_model_add(world, entity, row));
	return world;
}

// Row `r` of `object`'s ground rows applied to `at`.
static float row_at(const voe_render_object *object, uint32_t r,
		    voe_math_float3 at)
{
	const voe_math_float4 row = object->ground_rows[r];

	return row.x * at.x + row.y * at.y + row.z * at.z + row.w;
}

static void the_ground_is_found_and_filled(voe_base_arena *arena,
					   voe_3d_models *models)
{
	voe_ecs_world *world = a_world(arena);
	voe_3d_frame frame =
		voe_3d_draw_system_frame(world, (voe_platform_size){ SIDE, SIDE },
					 0.0f);
	const voe_math_float3 over = { 100.0f, 5.0f, 50.0f };
	struct voe_3d_grounds grounds;
	const struct voe_3d_ground *under;
	voe_render_object object = { 0 };
	voe_render_object nothing = { 0 };
	voe_render_object untouched = { 0 };

	frame.models = models;
	grounds = voe_3d_draw_grounds(world, &frame, arena);
	VOE_TEST_CHECK_INT(grounds.count, 1);
	under = voe_3d_draw_ground_under(&grounds, over);
	VOE_TEST_CHECK(under != NULL);
	VOE_TEST_CHECK(voe_3d_draw_ground_under(
			       &grounds, (voe_math_float3){ 400.0f, 0.0f,
							    0.0f }) == NULL);
	if (under == NULL)
		return;

	voe_3d_draw_ground_fill(under, 1.0f, &object);
	VOE_TEST_CHECK(object.ground != 0);
	VOE_TEST_CHECK(object.paint[0] != 0);
	VOE_TEST_CHECK(object.terrain.w == METRES);
	VOE_TEST_CHECK(fabsf(row_at(&object, 0, over)) < 1e-3f);
	VOE_TEST_CHECK(fabsf(row_at(&object, 1, over) - 5.0f) < 1e-3f);
	VOE_TEST_CHECK(fabsf(row_at(&object, 2, over)) < 1e-3f);

	voe_3d_draw_ground_fill(under, 20.0f, &object);
	VOE_TEST_CHECK(object.ground_reach == 10.0f);
	voe_3d_draw_ground_fill(under, 0.0f, &nothing);
	VOE_TEST_CHECK(memcmp(&nothing, &untouched, sizeof(nothing)) == 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);
	voe_assets_landscape land;
	voe_3d_models *models;

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	land = voe_assets_landscape_flat(METRES, 4, arena);
	snprintf(land.layers[0], sizeof(land.layers[0]), "%s",
		 "Assets/none.material");
	land.layer_count = 1;
	models = voe_3d_models_new();
	VOE_TEST_CHECK(voe_3d_models_load_landscape(models, device, PATH, 1,
						    &land, &error));

	the_ground_is_found_and_filled(arena, models);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
