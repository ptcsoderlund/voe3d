// A copy keeps its LOD in a memory (0500 point 1, 0501), in the world of
// draw_foliage_world.inc: one copy of a 64 × 32 sphere with its levels file
// (model_glb.inc), b its levels[1].bound.
//
// 13. Seen at s 1.25 b, level 0 groups and no `rest`, its key found; at 0.95 b
//     0.1 s later, level 1 and a `rest` group at level 0; 0.3 s on, no `rest`.
// 14. The copy 60 m behind the camera, the sun nearly overhead: every group
//     unseen, and its key not found in a new memory.
//
// IT NEEDS A GRAPHICS CARD AND SKIPS WITH A REASON WITHOUT ONE, as
// 3d/tests/draw_model_fade.c does.
#include "draw_foliage_world.inc"

#include "model_glb.inc"

// A store holding FOREST with its first copy alone, of KIND loaded as a
// 64 × 32 sphere with its levels file.
static voe_3d_models *a_levelled_store(voe_render_device *device,
				       voe_base_arena *arena)
{
	voe_3d_models *models = a_store(device, arena, false, 1);
	voe_base_error error = VOE_BASE_OK;
	size_t glb_size;
	size_t levels_size = 0;
	const uint8_t *glb = sphere_glb(arena, 32, 64, false, &glb_size);
	const uint8_t *levels = voe_3d_model_levels_make(
		arena, glb, glb_size, VOE_ASSETS_LEVELS_DEFAULT, &levels_size,
		&error);

	VOE_TEST_CHECK(levels != NULL);
	VOE_TEST_CHECK(voe_3d_models_load_levelled(models, device, KIND, 1, glb,
						   glb_size, levels, levels_size,
						   &error));
	return models;
}

// The world of a_world_from with its camera `distance` metres along +Z from
// `centre`, looking down −Z, under a sun shining nearly straight down.
static voe_ecs_world *a_world_facing(voe_base_arena *arena,
				     voe_ecs_entity *land,
				     voe_math_float3 centre, double distance)
{
	return a_world_from(
		arena, land,
		(voe_scene_transform){
			.position = { centre.x, centre.y, centre.z + distance },
			.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
			.scale = { 1.0f, 1.0f, 1.0f } },
		(voe_math_float3){ 0.1f, -1.0f, 0.1f });
}

// The world radius of `models`' one copy of KIND, its centre into `centre`;
// its row stands at the identity.
static float the_copy(const voe_3d_models *models, voe_math_float3 *centre)
{
	const voe_3d_model_entry *kind = voe_3d_models_find(models, KIND);
	const voe_scene_transform placed = voe_3d_models_landscape_placed(
		models, voe_3d_models_find(models, FOREST))[0];
	const voe_math_float3 scale = placed.scale;

	*centre = voe_math_float4x4_transform_point(
		voe_scene_transform_matrix(placed, (voe_math_double3){ 0 }),
		kind->centre);
	return kind->radius *
	       fmaxf(fabsf(scale.x), fmaxf(fabsf(scale.y), fabsf(scale.z)));
}

// `frame` holds a group of `kind`'s part 0 at `level`, `rest` as asked.
static bool holds(const voe_3d_frame *frame, const voe_3d_model_entry *kind,
		  uint32_t level, bool rest)
{
	const voe_render_geometry at = kind->levels[level].geometries[0];

	for (uint32_t i = 0; i < frame->foliage_count; i++)
		if (frame->foliage[i].rest == rest &&
		    frame->foliage[i].geometry.index == at.index &&
		    frame->foliage[i].geometry.generation == at.generation)
			return true;
	return false;
}

// `frame` holds a `rest` group.
static bool any_rest(const voe_3d_frame *frame)
{
	for (uint32_t i = 0; i < frame->foliage_count; i++)
		if (frame->foliage[i].rest)
			return true;
	return false;
}

// 13: s at a distance is radius × P[1][1] ÷ that distance.
static void a_copy_swap_dissolves(voe_render_device *device,
				  voe_base_arena *arena)
{
	voe_3d_models *models = a_levelled_store(device, arena);
	const voe_3d_model_entry *ball = voe_3d_models_find(models, KIND);
	const double focal = voe_3d_projection(LENS, 1.0f).m[1][1];
	voe_3d_lods_kept *kept = voe_3d_lods_kept_create(arena);
	voe_ecs_entity land = { 0 };
	voe_3d_model_level_choice found;
	voe_math_float3 centre;
	const float radius = the_copy(models, &centre);
	voe_3d_frame frame;
	double near, past;

	VOE_TEST_CHECK(ball != NULL && ball->level_count >= 2);
	if (ball != NULL && ball->level_count >= 2) {
		near = radius * focal / (1.25 * ball->levels[1].bound);
		past = radius * focal / (0.95 * ball->levels[1].bound);
		printf("b %.4f, radius %.3f: %.2f m, then %.2f m\n",
		       ball->levels[1].bound, radius, near, past);
		VOE_TEST_CHECK(gather_kept(a_world_facing(arena, &land, centre, near),
					   device, arena, models, kept, 0.0,
					   &frame));
		VOE_TEST_CHECK(holds(&frame, ball, 0, false));
		VOE_TEST_CHECK(!holds(&frame, ball, 1, false));
		VOE_TEST_CHECK(!any_rest(&frame));
		VOE_TEST_CHECK(voe_3d_lods_kept_find(
			kept, voe_3d_lods_kept_copy_key(land, 0, 0, 0), &found));

		VOE_TEST_CHECK(gather_kept(a_world_facing(arena, &land, centre, past),
					   device, arena, models, kept, 0.1,
					   &frame));
		VOE_TEST_CHECK(holds(&frame, ball, 1, false));
		VOE_TEST_CHECK(holds(&frame, ball, 0, true));
		VOE_TEST_CHECK(!holds(&frame, ball, 0, false));

		VOE_TEST_CHECK(gather_kept(a_world_facing(arena, &land, centre, past),
					   device, arena, models, kept, 0.4,
					   &frame));
		VOE_TEST_CHECK(holds(&frame, ball, 1, false));
		VOE_TEST_CHECK(!any_rest(&frame));
	}
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

// 14
static void unseen_copies_keep_no_memory(voe_render_device *device,
					 voe_base_arena *arena)
{
	voe_3d_models *models = a_levelled_store(device, arena);
	voe_3d_lods_kept *kept = voe_3d_lods_kept_create(arena);
	voe_ecs_entity land = { 0 };
	voe_3d_model_level_choice found;
	voe_math_float3 centre;
	voe_3d_frame frame;

	(void)the_copy(models, &centre);
	VOE_TEST_CHECK(gather_kept(a_world_facing(arena, &land, centre, -60.0),
				   device, arena, models, kept, 0.0, &frame));
	VOE_TEST_CHECK_INT(frame.counts.skipped, 1);
	VOE_TEST_CHECK(frame.foliage_count > 0);
	for (uint32_t i = 0; i < frame.foliage_count; i++)
		VOE_TEST_CHECK(frame.foliage[i].tier == VOE_3D_FOLIAGE_UNSEEN);
	VOE_TEST_CHECK(!voe_3d_lods_kept_find(
		kept, voe_3d_lods_kept_copy_key(land, 0, 0, 0), &found));
	voe_3d_models_clear(models, device);
	voe_3d_models_destroy(models);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_render_device *device = a_device(arena);

	if (device == NULL) {
		voe_base_arena_destroy(arena);
		return voe_test_result();
	}
	a_copy_swap_dissolves(device, arena);
	unseen_copies_keep_no_memory(device, arena);
	voe_render_device_destroy(device);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
