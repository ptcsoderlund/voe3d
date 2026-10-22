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
// The view and the sun draw them (0442 point 5): a casting sun shines along
// (-1, -1, -0.5), so the first copy's quad, standing in x = 1 from y 0 to 2,
// throws its shadow back over its own place, the origin under the picture's
// centre (its point y 1, z 0.5 lands there), and no fill lights the shadow.
//
// 5. The centre pixel with the copies differs from the same frame with none.
// 6. The whole frame's draws, shadows and view, with three copies equal those
//    with the first alone, one draw a group a pass, and exceed those with none.
//
// Each copy is gathered seen, cast or unseen (0464 point 4):
//
// 7. The camera turned to look straight up: the gather counts every copy
//    skipped, none drawn, and no group is seen.
// 8. The camera 1.5 m up at z = −5 looking along −Z 20° below level, every
//    copy behind it, and the sun low beyond them along (−0.5, −1, −4): every
//    copy counted skipped, yet the floor at column 17, row 28, in the first
//    copy's shadow three pixels wide there, reads darker than with none.
// 9. With the camera of 5, every copy is counted drawn.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_model_fade.c does.
#include <3d/draw_system.h>
#include <3d/mesh_component.h>
#include <3d/model_component.h>
#include <3d/models.h>
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

#define SCRATCH (4 * 1024 * 1024)
#define SIDE 32
#define KIND "Assets/two.glb"
#define FOREST "Assets/forest.landscape"
#define COPIES 3

// The model's parts and the landscape's grid, both with their twins; the
// ground's nodes and the groups drawn into the view and four cascades.
static const voe_render_capacities CAPACITIES = {
	.vertices = 33 * 33 + 6,
	.indices = 32 * 32 * 6 + 6,
	.geometries = 4,
	.objects = (1 + VOE_RENDER_SHADOW_CASCADES) * 64,
	.shadings = 8,
	.passes = 1 + VOE_RENDER_SHADOW_CASCADES,
	.shadow_size = VOE_3D_SHADOW_TEXELS,
	.heights_texels = VOE_3D_LANDSCAPE_WRITE_TEXELS,
	.instances = 64,
};

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
	VOE_TEST_CHECK(voe_scene_camera_add(
		world, entity,
		(voe_scene_camera){ .fov_y = 1.0471976f,
				    .near_plane = 0.1f,
				    .far_plane = 100.0f }));

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
static voe_ecs_world *a_world(voe_base_arena *arena, voe_ecs_entity *land)
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
// `count` of three copies, and, with `kind`, KIND itself.
static voe_3d_models *a_store(voe_render_device *device, voe_base_arena *arena,
			      bool kind, uint32_t count)
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
	voe_3d_models *models = a_store(device, arena, true, COPIES);
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
	voe_3d_models *models = a_store(device, arena, false, COPIES);
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	voe_3d_frame frame;

	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK_INT(frame.foliage_count, 0);
	VOE_TEST_CHECK(frame.foliage == NULL);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// One whole frame of `world` from a store of the first `count` copies, as the
// loop draws it: gathered, its shadows, the view's pass and the run. The
// frame's draws in `draws`, its counts after the shadows in `counts`; the
// pixel at `column`, `row`, rgb packed.
static uint32_t a_frame_at(voe_ecs_world *world, voe_render_device *device,
			   voe_base_arena *arena, uint32_t count,
			   uint32_t column, uint32_t row, uint32_t *draws,
			   voe_3d_view_counts *counts)
{
	voe_platform_size size = { SIDE, SIDE };
	voe_3d_models *models = a_store(device, arena, true, count);
	voe_3d_frame frame = voe_3d_draw_system_frame(world, size, 0.0f);
	voe_render_pass_camera camera;
	voe_render_picture picture = { 0 };
	voe_base_error error = VOE_BASE_OK;
	bool drawing = false;
	const uint8_t *pixel;
	uint32_t centre = 0;

	frame.models = models;
	VOE_TEST_CHECK(voe_render_frame_begin(device, size, &drawing));
	VOE_TEST_CHECK(drawing);
	if (drawing) {
		VOE_TEST_CHECK(voe_3d_draw_system_foliage(world, device, &frame,
							  arena));
		VOE_TEST_CHECK(voe_3d_draw_system_shadows(world, device, &frame));
		VOE_TEST_CHECK_INT(frame.shadow.count,
				   VOE_RENDER_SHADOW_CASCADES);
		*counts = frame.counts;
		camera = voe_3d_draw_system_camera(&frame);
		VOE_TEST_CHECK(voe_render_pass_begin(
			device, VOE_RENDER_TARGET_WINDOW, &camera));
		voe_3d_draw_system_run(world, device, arena, frame);
		voe_render_pass_end(device);
		*draws = voe_render_frame_draw_count(device);
		VOE_TEST_CHECK(voe_render_frame_end(device));
		VOE_TEST_CHECK(voe_render_target_read(
			device, VOE_RENDER_TARGET_WINDOW, arena, &picture, &error));
	}
	if (picture.pixels != NULL) {
		pixel = picture.pixels +
			((size_t)row * picture.width + column) * 4;
		centre = (uint32_t)pixel[0] << 16 | (uint32_t)pixel[1] << 8 |
			 pixel[2];
		printf("%u copies: %u groups, %u draws, %u drawn, %u skipped, "
		       "%u cast, %u cast skipped, (%u, %u) %06x\n",
		       count, frame.foliage_count, *draws, frame.counts.drawn,
		       frame.counts.skipped, frame.counts.cast,
		       frame.counts.cast_skipped, column, row, centre);
	}
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
	return centre;
}

// The frame of a_frame_at, its centre pixel.
static uint32_t a_frame(voe_ecs_world *world, voe_render_device *device,
			voe_base_arena *arena, uint32_t count, uint32_t *draws,
			voe_3d_view_counts *counts)
{
	return a_frame_at(world, device, arena, count, SIDE / 2, SIDE / 2,
			  draws, counts);
}

// The sum of a packed pixel's three channels.
static uint32_t brightness(uint32_t rgb)
{
	return (rgb >> 16 & 0xff) + (rgb >> 8 & 0xff) + (rgb & 0xff);
}

// 5, 6 and 9.
static void the_view_and_the_sun_draw_copies(voe_render_device *device,
					     voe_base_arena *arena)
{
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	uint32_t none_draws = 0, one_draws = 0, three_draws = 0;
	voe_3d_view_counts counts = { 0 };
	const uint32_t none =
		a_frame(world, device, arena, 0, &none_draws, &counts);
	const uint32_t three =
		a_frame(world, device, arena, COPIES, &three_draws, &counts);

	VOE_TEST_CHECK_INT(counts.drawn, COPIES);
	VOE_TEST_CHECK_INT(counts.skipped, 0);
	(void)a_frame(world, device, arena, 1, &one_draws, &counts);
	VOE_TEST_CHECK(three != none);
	VOE_TEST_CHECK_INT(three_draws, one_draws);
	VOE_TEST_CHECK(one_draws > none_draws);
}

// 7: the camera turned to look straight up from 5 m.
static void copies_behind_are_skipped(voe_render_device *device,
				      voe_base_arena *arena)
{
	voe_3d_models *models = a_store(device, arena, true, COPIES);
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world_from(
		arena, &land,
		(voe_scene_transform){
			.position = { 0.0, 5.0, 0.0 },
			.rotation = { 0.70710678f, 0.0f, 0.0f, 0.70710678f },
			.scale = { 1.0f, 1.0f, 1.0f } },
		(voe_math_float3){ -1.0f, -1.0f, -0.5f });
	voe_3d_frame frame;

	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK_INT(frame.counts.drawn, 0);
	VOE_TEST_CHECK_INT(frame.counts.skipped, COPIES);
	for (uint32_t i = 0; i < frame.foliage_count; i++)
		VOE_TEST_CHECK(frame.foliage[i].tier != VOE_3D_FOLIAGE_SEEN);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// 8: the camera 1.5 m up at z = −5, 20° below level, the sun beyond.
static void copies_behind_still_shadow(voe_render_device *device,
				       voe_base_arena *arena)
{
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world_from(
		arena, &land,
		(voe_scene_transform){
			.position = { 0.0, 1.5, -5.0 },
			.rotation = { -0.17364818f, 0.0f, 0.0f, 0.98480775f },
			.scale = { 1.0f, 1.0f, 1.0f } },
		(voe_math_float3){ -0.5f, -1.0f, -4.0f });
	uint32_t draws = 0;
	voe_3d_view_counts counts = { 0 };
	const uint32_t none = a_frame_at(world, device, arena, 0, 17, 28,
					 &draws, &counts);
	const uint32_t three = a_frame_at(world, device, arena, COPIES, 17, 28,
					  &draws, &counts);

	VOE_TEST_CHECK_INT(counts.drawn, 0);
	VOE_TEST_CHECK_INT(counts.skipped, COPIES);
	VOE_TEST_CHECK(brightness(three) < brightness(none));
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
	models = a_store(device, arena, true, COPIES);
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
	the_view_and_the_sun_draw_copies(device, arena);
	copies_behind_are_skipped(device, arena);
	copies_behind_still_shadow(device, arena);
	voe_render_device_destroy(device);
	no_room_gathers_none(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
