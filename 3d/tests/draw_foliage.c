// A frame gathers its foliage into groups (0442 point 5), in the world and
// store of draw_foliage_world.inc.
//
// 1. Inside a frame, the groups' counts sum to three times the model's opaque
//    or cut-out parts, none is `rest` (a model without levels never
//    dissolves), and each group's range lies within what was written.
// 2. The row switched off gathers none, and true.
// 3. A store without the `.glb` gathers none, and true.
// 4. A device with `instances` 2 refuses the room: false, with none.
//
// A layer grows the kind too (0476 points 5 and 6), the store growing.
//
// 10. The groups' counts sum to the hand and grown copies times the parts,
//     each group casting, and some copies were grown.
// 11. With the kind's look shadowless, every group is seen and not casting,
//     the copies counted drawn are those gathered, and drawn and skipped
//     together are every copy.
//
// A blending kind blends with the landscape it stands on (0481 point 3): the
// same landscape, painted by its layer.
//
// 12. The kind's look set blending at 120 cm, every group has a ground and
//     reach 1.2; set back to the default look, every group's ground is NULL.
// 16. KIND a species of two `.glb`s at KIND#v (0515 point 3): none loaded, no
//     group; both, groups drawing their parts, three copies' worth.
//
// The view and the sun drawing them are draw_foliage_view.c's; a copy's LOD
// memory draw_foliage_lods.c's.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_model_fade.c does.
#include "draw_foliage_world.inc"

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
	const uint32_t parts = opaque_parts(models, KIND);

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

// 10 and 11, from the camera of 5.
static void grown_copies_gather(voe_render_device *device,
				voe_base_arena *arena, bool shadowless)
{
	voe_3d_models *models =
		a_store_growing(device, arena, true, COPIES, true, shadowless);
	const voe_3d_models_grown grown = voe_3d_models_landscape_grown(
		models, voe_3d_models_find(models, FOREST));
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	voe_3d_frame frame;
	uint32_t sum = 0;
	const uint32_t parts = opaque_parts(models, KIND);

	printf("%u grown, shadowless %d\n", grown.count, shadowless);
	VOE_TEST_CHECK(grown.count > 0);
	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK(frame.foliage_count > 0);
	for (uint32_t i = 0; i < frame.foliage_count; i++) {
		VOE_TEST_CHECK(frame.foliage[i].casts == !shadowless);
		VOE_TEST_CHECK(!shadowless ||
			       frame.foliage[i].tier == VOE_3D_FOLIAGE_SEEN);
		sum += frame.foliage[i].count;
	}
	VOE_TEST_CHECK_INT(frame.counts.drawn + frame.counts.skipped,
			   COPIES + grown.count);
	if (shadowless)
		VOE_TEST_CHECK_INT(sum, frame.counts.drawn * parts);
	else
		VOE_TEST_CHECK_INT(sum, (COPIES + grown.count) * parts);
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// The groups gathered with kind 0's look set to `look`: true when every one
// has a ground and reach `reach`, or, with `reach` 0, none has a ground.
static bool groups_blend(voe_render_device *device, voe_base_arena *arena,
			 voe_3d_models *models, voe_assets_landscape_look look,
			 float reach)
{
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	voe_3d_frame frame;
	bool all = true;

	voe_3d_models_landscape_look_set(models, FOREST, 0, look);
	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK(frame.foliage_count > 0);
	for (uint32_t i = 0; i < frame.foliage_count; i++) {
		const voe_3d_foliage_group group = frame.foliage[i];

		printf("group %u: ground %d, reach %.3f\n", i,
		       group.ground != NULL, group.ground_reach);
		all = all && (group.ground != NULL) == (reach > 0.0f) &&
		      group.ground_reach == reach;
	}
	return all;
}

// 12, from the camera of 5.
static void blending_kind_names_its_ground(voe_render_device *device,
					   voe_base_arena *arena)
{
	voe_3d_models *models =
		a_store_growing(device, arena, true, COPIES, true, false);
	voe_assets_landscape_look look = voe_assets_landscape_look_default();

	look.blends = true;
	look.blend_height = 120;
	VOE_TEST_CHECK(groups_blend(device, arena, models, look, 1.2f));
	VOE_TEST_CHECK(groups_blend(device, arena, models,
				    voe_assets_landscape_look_default(), 0.0f));
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// `group` draws a part of one of the `count` models at `paths`.
static bool draws_one_of(const voe_3d_models *models, const char *const *paths,
			 uint32_t count, voe_3d_foliage_group group)
{
	for (uint32_t v = 0; v < count; v++) {
		const voe_3d_model_entry *entry =
			voe_3d_models_find(models, paths[v]);

		for (uint32_t p = 0; entry != NULL && p < entry->part_count; p++)
			if (entry->parts[p].geometry.index == group.geometry.index &&
			    entry->parts[p].geometry.generation ==
				    group.geometry.generation)
				return true;
	}
	return false;
}

// 16, from the camera of 5.
static void a_species_draws_its_variations(voe_render_device *device,
					   voe_base_arena *arena)
{
	static const char *const paths[] = { KIND "#0", KIND "#1" };
	voe_3d_models *models = a_store(device, arena, false, COPIES);
	voe_ecs_entity land = { 0 };
	voe_ecs_world *world = a_world(arena, &land);
	voe_base_error error = VOE_BASE_OK;
	voe_3d_frame frame;
	uint32_t sum = 0;

	VOE_TEST_CHECK(voe_3d_models_species_set(models, KIND, 2));
	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK_INT(frame.foliage_count, 0);
	for (uint32_t v = 0; v < 2; v++)
		VOE_TEST_CHECK(voe_3d_models_load(models, device, paths[v], 1,
						  TWO_PRIMITIVES_GLB,
						  sizeof(TWO_PRIMITIVES_GLB),
						  &error));
	VOE_TEST_CHECK(voe_3d_models_species_set(models, KIND, 2));
	VOE_TEST_CHECK(gather(world, device, arena, models, &frame));
	VOE_TEST_CHECK(frame.foliage_count > 0);
	for (uint32_t i = 0; i < frame.foliage_count; i++) {
		VOE_TEST_CHECK(draws_one_of(models, paths, 2, frame.foliage[i]));
		sum += frame.foliage[i].count;
	}
	VOE_TEST_CHECK_INT(sum, COPIES * opaque_parts(models, paths[0]));
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
	voe_render_device *device = a_device(arena);

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	copies_gather_into_groups(device, arena);
	no_kind_gathers_none(device, arena);
	grown_copies_gather(device, arena, false);
	grown_copies_gather(device, arena, true);
	blending_kind_names_its_ground(device, arena);
	a_species_draws_its_variations(device, arena);
	voe_render_device_destroy(device);
	no_room_gathers_none(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
