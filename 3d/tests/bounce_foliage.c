// The bounce sun maps draw the foliage, and a foliage change relights (0450,
// 0451). The world of 3d/tests/draw_foliage.c: a camera 5 m up looking down on
// a model row wearing an 8-cell flat landscape whose kind 0 is the hand-built
// `.glb` of model_data.inc, three copies near the centre; here its sun shines
// straight down, casting and bouncing 1, and the frame keeps a casters memory.
// Each frame gathers the foliage, then the shadows call; its draws are read by
// voe_render_frame_draw_count.
//
// SETTLED, COUNTED AS 3d/tests/bounce_world.inc COUNTS: by passes. The device
// has room for the cascades, VOE_RENDER_BOUNCE_CAPTURE_PASSES capture passes,
// a sun map per volume and one spare. A probing frame first opens DUMMIES
// empty passes, so a shadows call that would open a capture pass or a sun map
// returns false (render says so on a line; that noise is the measurement).
// Full and probing frames alternate until a probing one is true, within BOUND
// pairs, then FADE full frames more.
//
// 1. A kind set naming kind 0's own path makes the next frame draw d1 more
//    than the settled one: the store's count relit every volume.
// 2. Settled again, erasing one copy makes the next frame draw more.
// 3. The rest erased and settled, the same kind set draws d2 more. d1 − d2,
//    the sun maps' foliage, is a positive whole multiple of the first frame's
//    groups not `rest`.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_foliage.c does.
#include <3d/bounce_casters.h>
#include <3d/draw_system.h>
#include <3d/foliage.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
#include <3d/models_foliage.h>
#include <3d/panel_component.h>
#include <3d/shadow_cascades.h>
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/error.h>
#include <ecs/world.h>
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "model_data.inc"

#define SCRATCH (16 * 1024 * 1024)
#define SIDE 32
#define BOUND 320
// Places a new probe picture takes to full weight, render's VOE_RENDER_BOUNCE_FADE.
#define FADE 16
#define KIND "Assets/two.glb"
#define FOREST "Assets/forest.landscape"
#define COPIES 3
#define PASSES                                                          \
	(VOE_RENDER_SHADOW_CASCADES + VOE_RENDER_BOUNCE_CAPTURE_PASSES + \
	 VOE_RENDER_BOUNCE_VOLUMES + 1)
// Leaves the cascades' passes and not one more.
#define DUMMIES (PASSES - VOE_RENDER_SHADOW_CASCADES)

// The model's parts and the landscape's grid, both with their twins; the
// ground's nodes and the groups drawn into every pass.
static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33 + 6,
	.indices = 32 * 32 * 6 + 6,
	.geometries = 4,
	.objects = PASSES * 64,
	.shadings = 8,
	.passes = PASSES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
	.point_shadow_size = VOE_3D_POINT_SHADOW_TEXELS,
	.heights_texels = VOE_3D_LANDSCAPE_WRITE_TEXELS,
	.instances = 64,
};

// What every frame draws from.
struct bed {
	voe_ecs_world *world;
	voe_render_device *device;
	voe_3d_models *models;
	voe_base_arena *arena;
};

// The target's casters memory, zeroed is empty.
static voe_3d_bounce_casters memory;

// The camera 5 m up looking down, the sun straight down casting and bouncing
// 1, and the row wearing FOREST.
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

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){
			.rotation = voe_scene_light_facing(
				(voe_math_float3){ 0.0f, -1.0f, 0.0f }),
			.scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_light_add(
		world, entity,
		(voe_scene_light){ .colour = { 1.0f, 1.0f, 1.0f },
				   .intensity = 3.0f,
				   .bounces = 1,
				   .bounce_strength = 1.0f,
				   .cast_shadows = true }));

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &entity));
	VOE_TEST_CHECK(voe_scene_transform_add(
		world, entity,
		(voe_scene_transform){ .rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
				       .scale = { 1.0f, 1.0f, 1.0f } }));
	VOE_TEST_CHECK(voe_scene_identity_add(
		world, entity, (voe_scene_identity){ .id = 1, .name = "Forest" }));
	snprintf(row.path, sizeof(row.path), "%s", FOREST);
	VOE_TEST_CHECK(voe_3d_model_add(world, entity, row));
	return world;
}

// A store holding FOREST, 16 m of 8 cells naming KIND as kind 0 with three
// copies, and KIND itself.
static voe_3d_models *a_store(voe_render_device *device, voe_base_arena *arena)
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
	VOE_TEST_CHECK(voe_3d_models_load(models, device, KIND, 1,
					  TWO_PRIMITIVES_GLB,
					  sizeof(TWO_PRIMITIVES_GLB), &error));
	return models;
}

// One frame: the foliage gathered, then, `probing`, DUMMIES empty passes, then
// the shadows call. Whether that call was true; the frame's draws in `draws`
// and its groups not `rest` in `groups`. A full frame's call must be true.
static bool a_frame(const struct bed *bed, bool probing, uint32_t *draws,
		    uint32_t *groups)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(bed->arena);
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_frame frame = voe_3d_draw_system_frame(bed->world, size, 0.0f);
	bool drawing = false;
	bool shadows = false;

	frame.models = bed->models;
	frame.target = VOE_RENDER_TARGET_WINDOW;
	frame.casters = &memory;
	*draws = 0;
	*groups = 0;
	VOE_TEST_CHECK(voe_render_frame_begin(bed->device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_3d_draw_system_foliage(bed->world, bed->device,
							  &frame, bed->arena));
		for (uint32_t i = 0; i < frame.foliage_count; i++)
			*groups += !frame.foliage[i].rest;
		for (int i = 0; probing && i < DUMMIES; i++) {
			VOE_TEST_CHECK(voe_render_pass_begin(
				bed->device, VOE_RENDER_TARGET_WINDOW, NULL));
			voe_render_pass_end(bed->device);
		}
		shadows = voe_3d_draw_system_shadows(bed->world, bed->device,
						     &frame);
		VOE_TEST_CHECK(probing || shadows);
		*draws = voe_render_frame_draw_count(bed->device);
		VOE_TEST_CHECK(voe_render_frame_end(bed->device));
	}
	voe_base_arena_rewind(bed->arena, mark);
	return shadows;
}

// A full frame's draws.
static uint32_t draws_of_a_frame(const struct bed *bed)
{
	uint32_t draws = 0, groups = 0;

	(void)a_frame(bed, false, &draws, &groups);
	return draws;
}

// Full and probing frames until a probing one wants no pass, then FADE full
// frames. Whether that came within BOUND pairs.
static bool settles(const struct bed *bed)
{
	uint32_t draws = 0, groups = 0;

	for (int pair = 0; pair < BOUND; pair++) {
		(void)draws_of_a_frame(bed);
		if (a_frame(bed, true, &draws, &groups)) {
			printf("settled after %d pairs\n", pair + 1);
			for (int f = 0; f < FADE; f++)
				(void)draws_of_a_frame(bed);
			return true;
		}
	}
	return false;
}

// The settled frame's draws and the next's after a kind set naming kind 0's
// own path; their difference.
static uint32_t a_kind_set_relights(const struct bed *bed)
{
	uint32_t settled, relit;

	VOE_TEST_CHECK(settles(bed));
	settled = draws_of_a_frame(bed);
	voe_3d_models_landscape_kind_set(bed->models, FOREST, 0, KIND);
	relit = draws_of_a_frame(bed);
	printf("kind set: settled %u draws, relit %u\n", settled, relit);
	VOE_TEST_CHECK(relit > settled);
	return relit > settled ? relit - settled : 0;
}

// The copies of kind 0 within `radius` of (x, z) erased; how many.
static uint32_t erase(const struct bed *bed, float x, float z, float radius)
{
	const voe_3d_foliage_brush brush = { .radius = radius,
					     .softness = 0.0f,
					     .density = 4.0f,
					     .picked = 1u };
	voe_assets_landscape_copy removed[COPIES];

	return voe_3d_models_landscape_erase(bed->models, FOREST, &brush, x, z,
					     removed, COPIES);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_platform_size size = { SIDE, SIDE };
	voe_base_error error = VOE_BASE_OK;
	struct bed bed = { .arena = arena };
	uint32_t first = 0, groups = 0, settled, erased, d1, d2;

	bed.device = voe_render_device_new_headless(arena, size, CAPACITIES,
						    &error);
	if (bed.device == NULL) {
		if (error == VOE_BASE_ERROR_UNAVAILABLE ||
		    error == VOE_BASE_ERROR_UNSUPPORTED)
			printf("skip: %s\n", voe_base_error_string(error));
		else
			VOE_TEST_CHECK(bed.device != NULL);
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	bed.models = a_store(bed.device, arena);
	bed.world = a_world(arena);
	(void)a_frame(&bed, false, &first, &groups);
	printf("first frame: %u draws, %u groups\n", first, groups);
	VOE_TEST_CHECK(groups > 0);

	// 1.
	d1 = a_kind_set_relights(&bed);

	// 2.
	VOE_TEST_CHECK(settles(&bed));
	settled = draws_of_a_frame(&bed);
	VOE_TEST_CHECK_INT(erase(&bed, 0.0f, 0.0f, 0.5f), 1);
	erased = draws_of_a_frame(&bed);
	printf("erase: settled %u draws, next %u\n", settled, erased);
	VOE_TEST_CHECK(erased > settled);

	// 3.
	VOE_TEST_CHECK_INT(erase(&bed, 0.0f, 0.0f, 8.0f), COPIES - 1);
	d2 = a_kind_set_relights(&bed);
	printf("d1 %u, d2 %u, groups %u\n", d1, d2, groups);
	VOE_TEST_CHECK(d1 > d2);
	VOE_TEST_CHECK(groups > 0 && (d1 - d2) % groups == 0);

	voe_3d_models_clear(bed.models, bed.device);
	voe_3d_models_destroy(bed.models);
	voe_render_device_destroy(bed.device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
