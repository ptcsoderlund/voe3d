// A frame gathers its foliage into groups (0442 point 5): a camera 5 m up
// looks straight down on a model row wearing an 8-cell flat landscape whose
// kind 0 is the hand-built `.glb` of model_data.inc, loaded as
// 3d/tests/draw_model_fade.c loads it, with three copies near the centre.
//
// 1. Inside a frame, the groups' counts sum to three times the model's opaque
//    or cut-out parts, none is `rest` (a model without levels never
//    dissolves), and each group's range lies within what was written.
// 2. The row switched off gathers none, and true.
// 3. A store without the `.glb` gathers none, and true.
// 4. A device with `instances` 2 refuses the room: false, with none.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_model_fade.c does.
#include <3d/draw_system.h>
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
#include <scene/identity_system.h>
#include <scene/light_system.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "model_data.inc"

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32
#define KIND "Assets/two.glb"
#define FOREST "Assets/forest.landscape"
#define COPIES 3

// The model's parts and the landscape's grid, both with their twins; nothing
// is drawn, so one object and one pass are spare.
static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33 + 6,
	.indices = 32 * 32 * 6 + 6,
	.geometries = 4,
	.objects = 4,
	.shadings = 8,
	.passes = 1,
	.heights_texels = VOE_3D_LANDSCAPE_WRITE_TEXELS,
	.instances = 64,
};

// The camera 5 m up looking down, and the row wearing FOREST, into `land`.
static voe_ecs_world *a_world(voe_base_arena *arena, voe_ecs_entity *land)
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
	voe_3d_model row = { .cast_shadows = true };

	voe_scene_transform_register(world, 4);
	voe_scene_parent_register(world, 4);
	voe_scene_identity_register(world, 4);
	voe_scene_camera_register(world, 1);
	voe_scene_light_register(world, 1);
	voe_3d_mesh_register(world, 1);
	voe_3d_panel_register(world, 1);
	voe_3d_model_register(world, 1);

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.position = { 0.0, 5.0, 0.0 },
			.rotation = { -0.70710678f, 0.0f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, land));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, *land,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_identity_add(
		world, *land, (voe_scene_identity){ .id = 1, .name = "Forest" }));
	snprintf(row.path, sizeof(row.path), "%s", FOREST);
	VOE_TEST_CHECK(voe_3d_model_add(world, *land, row));
	return world;
}

// A store holding FOREST, 16 m of 8 cells naming KIND as kind 0 with three
// copies, and, with `kind`, KIND itself.
static voe_3d_models *a_store(voe_render_device *device, voe_base_arena *arena,
			      bool kind)
{
	voe_3d_models *models = voe_3d_models_new();
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 8, arena);
	voe_assets_landscape_copy copies[COPIES] = {
		{ .x = 0, .z = 0, .scale = 1000 },
		{ .x = 2000, .z = 0, .turn = 16384, .scale = 1200 },
		{ .x = -2000, .z = 2000, .scale = 800 },
	};
	voe_base_error error = VOE_BASE_OK;
	voe_assets_landscape_text text;

	snprintf(land.kinds[0], sizeof(land.kinds[0]), "%s", KIND);
	land.kind_count = 1;
	land.copies = copies;
	land.copy_count = COPIES;
	text = voe_assets_landscape_write(&land, arena);
	VOE_TEST_CHECK(voe_3d_models_load(models, device, FOREST, 1,
					  (const uint8_t *)text.text, text.size,
					  &error));
	if (kind)
		VOE_TEST_CHECK(voe_3d_models_load(models, device, KIND, 1,
						  TWO_PRIMITIVES_GLB,
						  sizeof(TWO_PRIMITIVES_GLB),
						  &error));
	return models;
}

// One frame of `world` from `models` that only gathers: the call's answer, the
// frame it filled in `out`.
static bool gather(voe_ecs_world *world, voe_render_device *device,
		   voe_base_arena *arena, const voe_3d_models *models,
		   voe_3d_frame *out)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;
	bool gathered = false;

	*out = voe_3d_draw_system_frame(world, size, 0.0f);
	out->models = models;
	VOE_TEST_CHECK(out->foliage == NULL && out->foliage_count == 0);
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (!drawing)
		return false;
	gathered = voe_3d_draw_system_foliage(world, device, out, arena);
	VOE_TEST_CHECK(voe_render_frame_end(device));
	printf("gathered %d: %u groups\n", gathered, out->foliage_count);
	return gathered;
}

// The model's parts drawn as foliage: neither blended.
static uint32_t opaque_parts(const voe_3d_models *models)
{
	const voe_3d_model_entry *kind = voe_3d_models_find(models, KIND);
	uint32_t parts = 0;

	VOE_TEST_CHECK(kind != NULL && kind->loaded);
	if (kind == NULL)
		return 0;
	for (uint32_t p = 0; p < kind->part_count; p++)
		parts += kind->parts[p].material.alpha_mode !=
			 VOE_RENDER_ALPHA_BLENDED;
	return parts;
}

// 1 and 2.
static void copies_gather_into_groups(voe_render_device *device,
				      voe_base_arena *arena)
{
	voe_3d_models *models = a_store(device, arena, true);
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	voe_scene_identity identity;
	voe_3d_frame frame;
	uint32_t sum = 0;
	const uint32_t parts = opaque_parts(models);

	VOE_TEST_CHECK(parts > 0);
	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK(frame.foliage_count > 0);
	for (uint32_t i = 0; i < frame.foliage_count; i++) {
		const voe_3d_foliage_group group = frame.foliage[i];

		VOE_TEST_CHECK(!group.rest);
		VOE_TEST_CHECK(group.count > 0);
		VOE_TEST_CHECK(group.part != NULL);
		VOE_TEST_CHECK(group.geometry.generation != 0);
		VOE_TEST_CHECK(group.first + group.count <=
			       CAPACITIES.instances);
		sum += group.count;
	}
	VOE_TEST_CHECK_INT(sum, COPIES * parts);
	// The groups are contiguous from the first written.
	for (uint32_t i = 1; i < frame.foliage_count; i++)
		VOE_TEST_CHECK_INT(frame.foliage[i].first,
				   frame.foliage[i - 1].first +
					   frame.foliage[i - 1].count);

	identity = *voe_scene_identity_get(world, land);
	identity.off = true;
	VOE_TEST_CHECK(voe_scene_identity_submit(
		world, (voe_scene_identity_intent){ .entity = land,
						    .identity = identity }));
	voe_scene_identity_system_run(world);
	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK_INT(frame.foliage_count, 0);
	VOE_TEST_CHECK(frame.foliage == NULL);

	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// 3.
static void no_kind_gathers_none(voe_render_device *device,
				 voe_base_arena *arena)
{
	voe_3d_models *models = a_store(device, arena, false);
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	voe_3d_frame frame;

	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK_INT(frame.foliage_count, 0);
	VOE_TEST_CHECK(frame.foliage == NULL);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// 4.
static void no_room_gathers_none(voe_base_arena *arena)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_render_capacities capacities = CAPACITIES;
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device;
	voe_3d_models *models;
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world;
	voe_3d_frame frame;

	capacities.instances = 2;
	device = voe_render_device_new_headless(arena, size, capacities, &error);
	VOE_TEST_CHECK(device != NULL);
	if (device == NULL)
		return;
	models = a_store(device, arena, true);
	world = a_world(arena, &land);
	VOE_TEST_CHECK(!gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK_INT(frame.foliage_count, 0);
	VOE_TEST_CHECK(frame.foliage == NULL);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	voe_render_device_destroy(device);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	voe_render_device *device =
		voe_render_device_new_headless(arena, size, CAPACITIES, &error);

	if (device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	copies_gather_into_groups(device, arena);
	no_kind_gathers_none(device, arena);
	voe_render_device_destroy(device);
	no_room_gathers_none(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
