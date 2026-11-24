// A foliage kind's seed (0517 point 4): a file with three kinds and no seed
// keys reads seeds 0, 1 and 2 and writes no `seed` line; a look with seed 5 on
// kind 1 writes `seed1=5` and reads back 5; `seed0=99` is clamped to 15 by the
// lenient read and refused MALFORMED by the strict one; and a new kind's seed
// is the kind count on a fresh landscape, the least free seed when that one is
// taken. A kind removed (0516): of three, kind 1 goes with its copies and the
// old kind 2 moves down with its path, look, growth and seed 2, its copies in
// order; that landscape round-trips through the strict read; removing the last
// kind leaves the others untouched; and a new seed after a removal is free.
//
// The texts are written by hand at four cells, the smallest a file may have.
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// Three kinds; kind 0's look keys go in, then the heights follow.
static const char *const three_kinds =
	"[Landscape]\nsize=64\ncells=4\n[Foliage]\nkind0=\"a.glb\"\n"
	"kind1=\"b.glb\"\nkind2=\"c.glb\"\n%s\n[Heights]\nrow0=0 0 0 0 0\n"
	"row1=0 0 0 0 0\nrow2=0 0 0 0 0\nrow3=0 0 0 0 0\nrow4=0 0 0 0 0\n";

static bool read_kinds(voe_base_arena *arena, const char *look_keys,
		       bool strict, voe_assets_landscape *out,
		       voe_base_error *error)
{
	char text[512];
	const int length = snprintf(text, sizeof(text), three_kinds, look_keys);

	VOE_TEST_CHECK(length > 0 && (size_t)length < sizeof(text));
	return strict ? voe_assets_landscape_read_strict(text, (size_t)length,
							 arena, out, error) :
			voe_assets_landscape_read(text, (size_t)length, arena,
						  out, error);
}

static void missing_seeds_are_the_indices(voe_base_arena *arena)
{
	voe_assets_landscape out;
	voe_assets_landscape_text written;

	VOE_TEST_CHECK(read_kinds(arena, "", true, &out, NULL));
	VOE_TEST_CHECK_INT(out.kind_count, 3);
	for (uint32_t k = 0; k < 3; k++) {
		VOE_TEST_CHECK_INT(out.looks[k].seed, 0);
		VOE_TEST_CHECK_INT(voe_assets_landscape_kind_seed(&out, k), k);
	}
	written = voe_assets_landscape_write(&out, arena);
	VOE_TEST_CHECK(strstr(written.text, "seed") == NULL);
}

static void a_set_seed_round_trips(voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, 4, arena);
	voe_assets_landscape back;
	voe_assets_landscape_text written;

	memcpy(grid.kinds[0], "a.glb", sizeof("a.glb"));
	memcpy(grid.kinds[1], "b.glb", sizeof("b.glb"));
	grid.kind_count = 2;
	grid.looks[0] = voe_assets_landscape_look_default();
	grid.looks[1] = voe_assets_landscape_look_default();
	grid.looks[1].seed = 5 + 1;
	VOE_TEST_CHECK_INT(voe_assets_landscape_kind_seed(&grid, 1), 5);
	written = voe_assets_landscape_write(&grid, arena);
	VOE_TEST_CHECK(strstr(written.text, "seed1=5\n") != NULL);
	VOE_TEST_CHECK(strstr(written.text, "seed0") == NULL);
	VOE_TEST_CHECK(voe_assets_landscape_read_strict(
		written.text, written.size, arena, &back, NULL));
	VOE_TEST_CHECK_INT(voe_assets_landscape_kind_seed(&back, 0), 0);
	VOE_TEST_CHECK_INT(voe_assets_landscape_kind_seed(&back, 1), 5);
}

static void an_out_of_range_seed_is_clamped_or_refused(voe_base_arena *arena)
{
	voe_assets_landscape out = { .size = 1.0f };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(read_kinds(arena, "seed0=99\n", false, &out, NULL));
	VOE_TEST_CHECK_INT(voe_assets_landscape_kind_seed(&out, 0), 15);

	out = (voe_assets_landscape){ .size = 1.0f };
	VOE_TEST_CHECK(!read_kinds(arena, "seed0=99\n", true, &out, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
	VOE_TEST_CHECK(out.size == 1.0f && out.heights == NULL);
}

static void a_new_kind_takes_a_free_seed(voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, 4, arena);

	VOE_TEST_CHECK_INT(voe_assets_landscape_seed_new(&grid), 0);
	grid.kind_count = 3;
	VOE_TEST_CHECK_INT(voe_assets_landscape_seed_new(&grid), 3);
	// Kind 0 removed: the two left keep seeds 1 and 2, so 2 is taken.
	grid.looks[0].seed = 1 + 1;
	grid.looks[1].seed = 2 + 1;
	grid.kind_count = 2;
	VOE_TEST_CHECK_INT(voe_assets_landscape_seed_new(&grid), 0);
	// Seeds 0 and 2 taken: the least free is 1.
	grid.looks[0].seed = 0;
	VOE_TEST_CHECK_INT(voe_assets_landscape_seed_new(&grid), 1);
}

enum { COPIES_A_KIND = 2, THREE = 3, POINTS = 25 };

static bool same_look(voe_assets_landscape_look a, voe_assets_landscape_look b)
{
	return a.size == b.size && a.size_vary == b.size_vary &&
	       a.turn_vary == b.turn_vary && a.tilt == b.tilt &&
	       a.shadowless == b.shadowless && a.blends == b.blends &&
	       a.blend_height == b.blend_height && a.seed == b.seed;
}

static bool same_copy(voe_assets_landscape_copy a, voe_assets_landscape_copy b)
{
	return a.x == b.x && a.z == b.z && a.turn == b.turn &&
	       a.scale == b.scale && a.kind == b.kind && a.lean == b.lean;
}

// Three kinds on two layers, each look apart, layer 0 growing kinds 0 and 2
// and layer 1 kinds 1 and 2, and two copies of each kind, kinds interleaved.
static voe_assets_landscape three_grown(voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, 4, arena);
	static const char *const paths[THREE] = { "a.glb", "b.glb", "c.glb" };

	grid.layer_count = 2;
	memcpy(grid.layers[0], "a.material", sizeof("a.material"));
	memcpy(grid.layers[1], "b.material", sizeof("b.material"));
	grid.paint = voe_base_arena_push(arena, 8 * POINTS);
	memset(grid.paint, 0, 8 * POINTS);
	for (size_t p = 0; p < POINTS; p++)
		grid.paint[p * 4] = 255;
	grid.kind_count = THREE;
	for (uint32_t k = 0; k < THREE; k++) {
		memcpy(grid.kinds[k], paths[k], strlen(paths[k]) + 1);
		grid.looks[k] = voe_assets_landscape_look_default();
		// Off 1000, which writes nothing and reads back as 0.
		grid.looks[k].size = (uint16_t)(1500 + 500 * k);
		grid.looks[k].tilt = (uint8_t)(10 * k);
		grid.looks[k].blends = k == 2;
		grid.looks[k].blend_height = k == 2 ? 120 : 0;
	}
	grid.grows[0][0] = 10;
	grid.grows[0][2] = 30;
	grid.grows[1][1] = 20;
	grid.grows[1][2] = 40;
	grid.copy_count = THREE * COPIES_A_KIND;
	grid.copies = voe_base_arena_push(arena, grid.copy_count *
							 sizeof(*grid.copies));
	for (uint32_t i = 0; i < grid.copy_count; i++)
		grid.copies[i] = (voe_assets_landscape_copy){
			.x = (int32_t)(1000 * i), .z = -(int32_t)(500 * i),
			.turn = (uint16_t)(100 * i), .scale = (uint16_t)(900 + i),
			.kind = (uint8_t)(i % THREE), .lean = (uint8_t)(i * 7)
		};
	return grid;
}

static void removing_a_middle_kind_moves_the_later_down(voe_base_arena *arena)
{
	voe_assets_landscape grid = three_grown(arena);
	const voe_assets_landscape was = grid;
	voe_assets_landscape_copy before[THREE * COPIES_A_KIND];
	uint32_t c = 0;

	memcpy(before, grid.copies, sizeof(before));
	voe_assets_landscape_kind_remove(&grid, 1);
	VOE_TEST_CHECK_INT(grid.kind_count, 2);
	VOE_TEST_CHECK(strcmp(grid.kinds[0], "a.glb") == 0);
	VOE_TEST_CHECK(strcmp(grid.kinds[1], "c.glb") == 0);
	VOE_TEST_CHECK(grid.kinds[2][0] == '\0');
	VOE_TEST_CHECK(same_look(grid.looks[0], was.looks[0]));
	VOE_TEST_CHECK_INT(grid.looks[1].size, was.looks[2].size);
	VOE_TEST_CHECK_INT(grid.looks[1].tilt, was.looks[2].tilt);
	VOE_TEST_CHECK(grid.looks[1].blends);
	VOE_TEST_CHECK_INT(grid.looks[1].blend_height, 120);
	VOE_TEST_CHECK(same_look(grid.looks[2], (voe_assets_landscape_look){ 0 }));
	VOE_TEST_CHECK_INT(voe_assets_landscape_kind_seed(&grid, 0), 0);
	VOE_TEST_CHECK_INT(voe_assets_landscape_kind_seed(&grid, 1), 2);
	for (uint32_t l = 0; l < 2; l++) {
		VOE_TEST_CHECK_INT(grid.grows[l][0], was.grows[l][0]);
		VOE_TEST_CHECK_INT(grid.grows[l][1], was.grows[l][2]);
		VOE_TEST_CHECK_INT(grid.grows[l][2], 0);
	}
	VOE_TEST_CHECK_INT(grid.copy_count, 2 * COPIES_A_KIND);
	for (uint32_t i = 0; i < THREE * COPIES_A_KIND; i++) {
		if (before[i].kind == 1)
			continue;
		voe_assets_landscape_copy moved = before[i];

		moved.kind = moved.kind == 2 ? 1 : 0;
		VOE_TEST_CHECK(c < grid.copy_count &&
			       same_copy(grid.copies[c], moved));
		c++;
	}
	VOE_TEST_CHECK_INT(c, grid.copy_count);
}

static void a_removal_round_trips_strictly(voe_base_arena *arena)
{
	voe_assets_landscape grid = three_grown(arena);
	voe_assets_landscape back;
	voe_assets_landscape_text written;

	voe_assets_landscape_kind_remove(&grid, 1);
	written = voe_assets_landscape_write(&grid, arena);
	VOE_TEST_CHECK(strstr(written.text, "seed1=2\n") != NULL);
	VOE_TEST_CHECK(voe_assets_landscape_read_strict(
		written.text, written.size, arena, &back, NULL));
	VOE_TEST_CHECK(back.size == grid.size);
	VOE_TEST_CHECK_INT(back.cells, grid.cells);
	VOE_TEST_CHECK(memcmp(back.heights, grid.heights,
			      POINTS * sizeof(*back.heights)) == 0);
	VOE_TEST_CHECK_INT(back.layer_count, grid.layer_count);
	VOE_TEST_CHECK(memcmp(back.layers, grid.layers, sizeof(back.layers)) == 0);
	VOE_TEST_CHECK(memcmp(back.paint, grid.paint, 8 * POINTS) == 0);
	VOE_TEST_CHECK_INT(back.kind_count, grid.kind_count);
	VOE_TEST_CHECK(memcmp(back.kinds, grid.kinds, sizeof(back.kinds)) == 0);
	for (uint32_t k = 0; k < grid.kind_count; k++)
		VOE_TEST_CHECK(same_look(back.looks[k], grid.looks[k]));
	VOE_TEST_CHECK(memcmp(back.grows, grid.grows, sizeof(back.grows)) == 0);
	VOE_TEST_CHECK_INT(back.copy_count, grid.copy_count);
	for (uint32_t i = 0; i < grid.copy_count; i++)
		VOE_TEST_CHECK(same_copy(back.copies[i], grid.copies[i]));
}

static void removing_the_last_kind_leaves_the_others(voe_base_arena *arena)
{
	voe_assets_landscape grid = three_grown(arena);
	const voe_assets_landscape was = grid;
	voe_assets_landscape_copy before[THREE * COPIES_A_KIND];
	uint32_t c = 0;

	memcpy(before, grid.copies, sizeof(before));
	voe_assets_landscape_kind_remove(&grid, 2);
	VOE_TEST_CHECK_INT(grid.kind_count, 2);
	VOE_TEST_CHECK(memcmp(grid.kinds, was.kinds, 2 * sizeof(grid.kinds[0])) ==
		       0);
	VOE_TEST_CHECK(grid.kinds[2][0] == '\0');
	for (uint32_t k = 0; k < 2; k++) {
		VOE_TEST_CHECK(same_look(grid.looks[k], was.looks[k]));
		for (uint32_t l = 0; l < 2; l++)
			VOE_TEST_CHECK_INT(grid.grows[l][k], was.grows[l][k]);
	}
	VOE_TEST_CHECK(same_look(grid.looks[2], (voe_assets_landscape_look){ 0 }));
	VOE_TEST_CHECK_INT(grid.grows[0][2] + grid.grows[1][2], 0);
	for (uint32_t i = 0; i < THREE * COPIES_A_KIND; i++)
		if (before[i].kind != 2)
			VOE_TEST_CHECK(c < grid.copy_count &&
				       same_copy(grid.copies[c++], before[i]));
	VOE_TEST_CHECK_INT(c, grid.copy_count);
}

static void a_new_seed_after_a_removal_is_free(voe_base_arena *arena)
{
	voe_assets_landscape grid = three_grown(arena);
	uint32_t seed;

	voe_assets_landscape_kind_remove(&grid, 0);
	seed = voe_assets_landscape_seed_new(&grid);
	for (uint32_t k = 0; k < grid.kind_count; k++)
		VOE_TEST_CHECK(seed != voe_assets_landscape_kind_seed(&grid, k));
	VOE_TEST_CHECK_INT(seed, 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	missing_seeds_are_the_indices(arena);
	a_set_seed_round_trips(arena);
	an_out_of_range_seed_is_clamped_or_refused(arena);
	a_new_kind_takes_a_free_seed(arena);
	removing_a_middle_kind_moves_the_later_down(arena);
	a_removal_round_trips_strictly(arena);
	removing_the_last_kind_leaves_the_others(arena);
	a_new_seed_after_a_removal_is_free(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
