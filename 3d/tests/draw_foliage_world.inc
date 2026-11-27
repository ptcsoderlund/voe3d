// The foliage draw tests' harness, compiled into each program that includes
// it: draw_foliage.c, draw_foliage_view.c and draw_foliage_lods.c. It holds
// the world, the store, one frame that only gathers, a model's opaque parts
// and the headless device each program opens; the cases stay in the
// programs. It brings its own includes, so a program needs only this line
// for them.
//
// THE WORLD (0442 point 5): a camera 5 m up looks straight down on a model row
// wearing an 8-cell flat landscape whose kind 0 is the hand-built `.glb` of
// model_data.inc, loaded as 3d/tests/draw_model_fade.c loads it, with three
// copies near the centre. A casting sun shines along (-1, -1, -0.5), and no
// fill lights its shadow. a_world_from takes another camera and sun.
//
// THE STORE: FOREST names KIND as kind 0 with the first `count` of the three
// copies. Growing, its one layer grows kind 0 at 50 tenths per 100 m², a copy
// every 5 m, beside the three (0476 points 5 and 6), and its look may be
// shadowless.
//
// IT NEEDS A GRAPHICS CARD: a_device prints the reason and gives NULL without
// one, as 3d/tests/draw_model_fade.c skips, and the program returns its result.
#include <3d/draw_system.h>
#include <3d/lods_kept.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/model_levels.h>
#include <3d/models.h>
#include <3d/models_foliage.h>
#include <3d/panel_component.h>
#include <3d/projection.h>
#include <3d/shadow_cascades.h>
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
#include <math/float4x4.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/camera_system.h>
#include <scene/identity_system.h>
#include <scene/light_component.h>
#include <scene/light_system.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>
#include <scene/transform_system.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "model_data.inc"

// Two memories of 12 MiB each among the rest, never rewound.
#define SCRATCH (64 * 1024 * 1024)
#define SIDE 32
#define KIND "Assets/two.glb"
#define FOREST "Assets/forest.landscape"
#define COPIES 3

// The model's parts and the landscape's grid, both with their twins, or the
// sphere's levels in their place; the ground's nodes and the groups drawn into
// the view and four cascades.
static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33 + 6 + 32 * 1024,
	.indices = 32 * 32 * 6 + 6 + 128 * 1024,
	.geometries = 4 + VOE_3D_MODEL_LEVELS,
	.objects = (1 + VOE_RENDER_SHADOW_CASCADES) * 64,
	.shadings = 8,
	.passes = 1 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
	.heights_texels = VOE_3D_LANDSCAPE_WRITE_TEXELS,
	.instances = 64,
};

static const voe_scene_camera LENS = { .fov_y = 1.0471976f,
				       .near_plane = 0.1f,
				       .far_plane = 100.0f };

// The camera at `camera`, a casting sun shining along `sun` with no fill, and
// the row wearing FOREST, into `land`.
static voe_ecs_world *a_world_from(voe_base_arena *arena, voe_ecs_entity *land,
				   voe_scene_transform camera,
				   voe_math_float3 sun)
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
	VOE_TEST_CHECK(voe_scene_transform_add(world, entity, camera));
	VOE_TEST_CHECK(voe_scene_camera_add(world, entity, LENS));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = voe_scene_light_facing(sun),
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .cast_shadows = true }));

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

// The camera 5 m up looking down and the sun of the header.
[[maybe_unused]] static voe_ecs_world *a_world(voe_base_arena *arena,
					       voe_ecs_entity *land)
{
	return a_world_from(
		arena, land,
		(voe_scene_transform){
			.position = { 0.0, 5.0, 0.0 },
			.rotation = { -0.70710678f, 0.0f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } },
		(voe_math_float3){ -1.0f, -1.0f, -0.5f });
}

// A store holding FOREST, 16 m of 8 cells naming KIND as kind 0 with the first
// `count` of three copies, and, with `kind`, KIND itself. With `grow`, one
// layer grows kind 0 at 50 tenths and its look is `shadowless`.
static voe_3d_models *a_store_growing(voe_render_device *device,
				      voe_base_arena *arena, bool kind,
				      uint32_t count, bool grow,
				      bool shadowless)
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
	land.copy_count = count;
	land.looks[0].shadowless = shadowless;
	if (grow) {
		snprintf(land.layers[0], sizeof(land.layers[0]), "%s",
			 "Assets/grass.material");
		land.layer_count = 1;
		land.grows[0][0] = 50;
	}
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

// The store of a_store_growing, growing nothing.
static voe_3d_models *a_store(voe_render_device *device, voe_base_arena *arena,
			      bool kind, uint32_t count)
{
	return a_store_growing(device, arena, kind, count, false, false);
}

// One frame of `world` from `models` that only gathers, through `kept` begun
// at `now` seconds, none with NULL: the call's answer, the frame it filled in
// `out`.
static bool gather_kept(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *arena, const voe_3d_models *models,
			voe_3d_lods_kept *kept, double now, voe_3d_frame *out)
{
	voe_platform_size size = { SIDE, SIDE };
	bool drawing = false;
	bool gathered = false;

	*out = voe_3d_draw_system_frame(world, size, 0.0f);
	out->models = models;
	if (kept != NULL)
		voe_3d_lods_kept_begin(kept, now);
	out->lods = kept;
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

// gather_kept with no memory.
[[maybe_unused]] static bool gather(voe_ecs_world *world,
				    voe_render_device *device,
				    voe_base_arena *arena,
				    const voe_3d_models *models,
				    voe_3d_frame *out)
{
	return gather_kept(world, device, arena, models, NULL, 0.0, out);
}

// The parts of the model at `path` drawn as foliage: neither blended.
[[maybe_unused]] static uint32_t opaque_parts(const voe_3d_models *models,
					      const char *path)
{
	const voe_3d_model_entry *kind = voe_3d_models_find(models, path);
	uint32_t parts = 0;

	VOE_TEST_CHECK(kind != NULL && kind->loaded);
	if (kind == NULL)
		return 0;
	for (uint32_t p = 0; p < kind->part_count; p++)
		parts += kind->parts[p].material.alpha_mode !=
			 VOE_RENDER_ALPHA_BLENDED;
	return parts;
}

// The headless device with CAPACITIES, or NULL: skipped with the reason
// printed when there is no card, a failed check otherwise.
static voe_render_device *a_device(voe_base_arena *arena)
{
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
	}
	return device;
}
