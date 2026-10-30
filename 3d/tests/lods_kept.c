// THE CLAIMS, on a hand-filled entry (radius 1, bounds infinite, 0.2, 0.05) on
// the −z axis of a 60° projection, placed where its size s is asked, none
// needing a graphics card:
//
// 1. First sight at s 0.1 takes level 1 whole, at 0.01 level 2 whole.
// 2. Kept at level 1 from s 0.19, then at 0.21 and 0.215: level 1 still, whole.
// 3. Then at 0.23, past 0.2 ÷ 0.9: level 0, from 1, shown 0.
// 4. A swap at 1 s to level 1 from 0, chosen at 1.0625 and 1.125 s: shown
//    0.25 and 0.5, from 0.
// 5. At 1.25 s and after: whole, `from` the level.
// 6. Kept at level 1, a frame begun and none chosen, then at 0.21: level 0
//    whole, as first sight.
// 7. A key chosen twice in its swap's frame, the second time placed elsewhere:
//    one answer, and _find gives it; the next frame's _find, before a choose,
//    gives last frame's; a key never chosen is not found.
// 8. A NULL memory at s 0.19, with level 1 kept before in a memory: level 1
//    whole, voe_3d_model_level's level, every time.
// 9. A row's key has the top bit clear, a copy's set; the same copy gives the
//    same key, another x, z, kind or landscape another.
#include <3d/lods_kept.h>
#include <3d/projection.h>

#include <base/arena.h>

#include <testing/test.h>

#include <math.h>

static voe_3d_model_entry model;
static voe_render_view view;

static voe_scene_transform placed_at(float s)
{
	return (voe_scene_transform){
		.position = { 0.0, 0.0, -view.projection.m[1][1] / s },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
}

static voe_3d_model_level_choice choose(voe_3d_lods_kept *kept, uint64_t key,
					float s)
{
	const voe_scene_transform placed = placed_at(s);

	return voe_3d_lods_kept_choose(kept, key, &model, &placed,
				       (voe_math_double3){ 0 }, &view);
}

static void check_choice(voe_3d_model_level_choice choice, uint32_t level,
			 uint32_t from, float shown)
{
	VOE_TEST_CHECK_INT(choice.level, level);
	VOE_TEST_CHECK_INT(choice.from, from);
	VOE_TEST_CHECK_FLOAT(choice.shown, shown, 1e-4);
}

// 1
static void first_sight_takes_the_distance_level(voe_3d_lods_kept *kept)
{
	voe_3d_lods_kept_begin(kept, 0.0);
	check_choice(choose(kept, 1, 0.1f), 1, 1, 1.0f);
	check_choice(choose(kept, 2, 0.01f), 2, 2, 1.0f);
}

// 2
static void creeping_back_keeps_the_coarser_level(voe_3d_lods_kept *kept)
{
	voe_3d_lods_kept_begin(kept, 0.0);
	check_choice(choose(kept, 1, 0.19f), 1, 1, 1.0f);
	voe_3d_lods_kept_begin(kept, 1.0);
	check_choice(choose(kept, 1, 0.21f), 1, 1, 1.0f);
	voe_3d_lods_kept_begin(kept, 2.0);
	check_choice(choose(kept, 1, 0.215f), 1, 1, 1.0f);
}

// 3
static void ten_percent_nearer_swaps_back(voe_3d_lods_kept *kept)
{
	voe_3d_lods_kept_begin(kept, 0.0);
	check_choice(choose(kept, 1, 0.19f), 1, 1, 1.0f);
	voe_3d_lods_kept_begin(kept, 1.0);
	check_choice(choose(kept, 1, 0.21f), 1, 1, 1.0f);
	voe_3d_lods_kept_begin(kept, 2.0);
	check_choice(choose(kept, 1, 0.23f), 0, 1, 0.0f);
}

// 4, 5
static void a_swap_dissolves_over_a_quarter_second(voe_3d_lods_kept *kept)
{
	voe_3d_lods_kept_begin(kept, 0.0);
	check_choice(choose(kept, 1, 0.3f), 0, 0, 1.0f);
	voe_3d_lods_kept_begin(kept, 1.0);
	check_choice(choose(kept, 1, 0.19f), 1, 0, 0.0f);
	voe_3d_lods_kept_begin(kept, 1.0625);
	check_choice(choose(kept, 1, 0.19f), 1, 0, 0.25f);
	voe_3d_lods_kept_begin(kept, 1.125);
	check_choice(choose(kept, 1, 0.19f), 1, 0, 0.5f);
}

// 5
static void settled_is_whole(voe_3d_lods_kept *kept)
{
	a_swap_dissolves_over_a_quarter_second(kept);
	voe_3d_lods_kept_begin(kept, 1.25);
	check_choice(choose(kept, 1, 0.19f), 1, 1, 1.0f);
	voe_3d_lods_kept_begin(kept, 5.0);
	check_choice(choose(kept, 1, 0.19f), 1, 1, 1.0f);
}

// 6
static void unseen_a_frame_is_first_sight(voe_3d_lods_kept *kept)
{
	voe_3d_lods_kept_begin(kept, 0.0);
	check_choice(choose(kept, 1, 0.19f), 1, 1, 1.0f);
	voe_3d_lods_kept_begin(kept, 1.0);
	voe_3d_lods_kept_begin(kept, 2.0);
	check_choice(choose(kept, 1, 0.21f), 0, 0, 1.0f);
}

// 7
static void two_chooses_in_a_frame_agree(voe_3d_lods_kept *kept)
{
	voe_3d_model_level_choice found;

	voe_3d_lods_kept_begin(kept, 0.0);
	check_choice(choose(kept, 1, 0.3f), 0, 0, 1.0f);
	voe_3d_lods_kept_begin(kept, 1.0);
	check_choice(choose(kept, 1, 0.19f), 1, 0, 0.0f);
	check_choice(choose(kept, 1, 0.01f), 1, 0, 0.0f);
	VOE_TEST_CHECK(voe_3d_lods_kept_find(kept, 1, &found));
	check_choice(found, 1, 0, 0.0f);
	voe_3d_lods_kept_begin(kept, 1.125);
	VOE_TEST_CHECK(voe_3d_lods_kept_find(kept, 1, &found));
	check_choice(found, 1, 0, 0.0f);
	VOE_TEST_CHECK(!voe_3d_lods_kept_find(kept, 2, &found));
}

// 8
static void null_memory_is_stateless(voe_3d_lods_kept *kept)
{
	const voe_scene_transform placed = placed_at(0.19f);
	const uint32_t level = voe_3d_model_level(
		&model, &placed, (voe_math_double3){ 0 }, &view);

	voe_3d_lods_kept_begin(kept, 0.0);
	check_choice(choose(kept, 1, 0.3f), 0, 0, 1.0f);
	VOE_TEST_CHECK_INT(level, 1);
	for (int i = 0; i < 3; i++)
		check_choice(choose(NULL, 1, 0.19f), level, level, 1.0f);
}

// 9
static void row_and_copy_keys_differ(void)
{
	const voe_ecs_entity row = { .index = 7, .generation = 3 };
	const voe_ecs_entity other = { .index = 8, .generation = 3 };
	const uint64_t copy = voe_3d_lods_kept_copy_key(row, 2, -40, 90);

	VOE_TEST_CHECK((voe_3d_lods_kept_row_key(row) >> 63) == 0);
	VOE_TEST_CHECK((copy >> 63) == 1);
	VOE_TEST_CHECK(voe_3d_lods_kept_row_key(row) !=
		       voe_3d_lods_kept_row_key(other));
	VOE_TEST_CHECK(copy == voe_3d_lods_kept_copy_key(row, 2, -40, 90));
	VOE_TEST_CHECK(copy != voe_3d_lods_kept_copy_key(row, 2, -41, 90));
	VOE_TEST_CHECK(copy != voe_3d_lods_kept_copy_key(row, 2, -40, 91));
	VOE_TEST_CHECK(copy != voe_3d_lods_kept_copy_key(row, 3, -40, 90));
	VOE_TEST_CHECK(copy != voe_3d_lods_kept_copy_key(other, 2, -40, 90));
}

int main(void)
{
	static void (*const tests[])(voe_3d_lods_kept *) = {
		first_sight_takes_the_distance_level,
		creeping_back_keeps_the_coarser_level,
		ten_percent_nearer_swaps_back,
		a_swap_dissolves_over_a_quarter_second,
		settled_is_whole,
		unseen_a_frame_is_first_sight,
		two_chooses_in_a_frame_agree,
		null_memory_is_stateless,
	};
	const voe_scene_camera lens = {
		.fov_y = 1.0471976f, // 60°
		.near_plane = 0.1f,
		.far_plane = 1000.0f,
	};
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	view.view = voe_math_float4x4_identity();
	view.projection = voe_3d_projection(lens, 1.0f);
	model.radius = 1.0f;
	model.level_count = 3;
	model.levels[0].bound = INFINITY;
	model.levels[1].bound = 0.2f;
	model.levels[2].bound = 0.05f;

	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		const struct voe_base_arena_mark mark = voe_base_arena_mark(arena);

		tests[i](voe_3d_lods_kept_create(arena));
		voe_base_arena_rewind(arena, mark);
	}
	row_and_copy_keys_differ();
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
