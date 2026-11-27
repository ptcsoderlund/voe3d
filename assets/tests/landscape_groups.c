// A landscape's foliage groups (0520, 0521 point 1). A group's seed (0517
// point 4): a file with three groups and no seed keys reads seeds 0, 1 and 2
// and writes no `seed` line; seed 5 on group 1 writes `seed1=5` and reads back
// 5; `seed0=99` is clamped to 15 by the lenient read and refused MALFORMED by
// the strict one; and a new group's seed is the group count on a fresh
// landscape, the least free seed when that one is taken. A group removed
// (0516): of three, group 1 goes with its copies and the old group 2 moves down
// with its glbs, looks, growth and seed 2, its copies in order; that landscape
// round-trips through the strict read; removing the last group leaves the
// others untouched; and a new seed after a removal is free.
//
// The old spelling: a text of `kind<k>` keys with look keys, a seed and copies
// reads, lenient and strict, as one-glb groups with the same looks, seeds and
// copies. Two groups of 3 and 1 glbs, shares 0 and 3 and a size off the
// default, write with no `kind` key and read back equal. Both spellings in one
// text, a share of 11 and a path twice in one group are refused MALFORMED by
// the strict read.
//
// The texts are written by hand at four cells, the smallest a file may have.
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

static const char *const four_flat_rows =
	"[Heights]\nrow0=0 0 0 0 0\nrow1=0 0 0 0 0\nrow2=0 0 0 0 0\n"
	"row3=0 0 0 0 0\nrow4=0 0 0 0 0\n";

// Three groups of one glb; group 0's look keys go in, then the heights follow.
static const char *const three_groups =
	"[Landscape]\nsize=64\ncells=4\n[Foliage]\nglb0_0=\"a.glb\"\n"
	"glb1_0=\"b.glb\"\nglb2_0=\"c.glb\"\n%s\n%s";

static bool read_text(voe_base_arena *arena, const char *text, bool strict,
		      voe_assets_landscape *out, voe_base_error *error)
{
	return strict ? voe_assets_landscape_read_strict(text, strlen(text),
							 arena, out, error) :
			voe_assets_landscape_read(text, strlen(text), arena,
						  out, error);
}

static bool read_groups(voe_base_arena *arena, const char *look_keys,
			bool strict, voe_assets_landscape *out,
			voe_base_error *error)
{
	char text[512];
	const int length = snprintf(text, sizeof(text), three_groups, look_keys,
				    four_flat_rows);

	VOE_TEST_CHECK(length > 0 && (size_t)length < sizeof(text));
	return read_text(arena, text, strict, out, error);
}

// A group of one glb at `path` with the default look into group g.
static void one_glb(voe_assets_landscape *landscape, uint32_t g,
		    const char *path)
{
	memcpy(landscape->groups[g].glbs[0], path, strlen(path) + 1);
	landscape->groups[g].looks[0] = voe_assets_landscape_look_default();
	landscape->groups[g].glb_count = 1;
}

static void missing_seeds_are_the_indices(voe_base_arena *arena)
{
	voe_assets_landscape out;
	voe_assets_landscape_text written;

	VOE_TEST_CHECK(read_groups(arena, "", true, &out, NULL));
	VOE_TEST_CHECK_INT(out.group_count, 3);
	for (uint32_t g = 0; g < 3; g++) {
		VOE_TEST_CHECK_INT(out.groups[g].seed, 0);
		VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&out, g), g);
	}
	written = voe_assets_landscape_write(&out, arena);
	VOE_TEST_CHECK(strstr(written.text, "seed") == NULL);
}

static void a_set_seed_round_trips(voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, 4, arena);
	voe_assets_landscape back;
	voe_assets_landscape_text written;

	one_glb(&grid, 0, "a.glb");
	one_glb(&grid, 1, "b.glb");
	grid.group_count = 2;
	grid.groups[1].seed = 5 + 1;
	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&grid, 1), 5);
	written = voe_assets_landscape_write(&grid, arena);
	VOE_TEST_CHECK(strstr(written.text, "seed1=5\n") != NULL);
	VOE_TEST_CHECK(strstr(written.text, "seed0") == NULL);
	VOE_TEST_CHECK(voe_assets_landscape_read_strict(
		written.text, written.size, arena, &back, NULL));
	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&back, 0), 0);
	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&back, 1), 5);
}

static void an_out_of_range_seed_is_clamped_or_refused(voe_base_arena *arena)
{
	voe_assets_landscape out = { .size = 1.0f };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(read_groups(arena, "seed0=99\n", false, &out, NULL));
	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&out, 0), 15);

	out = (voe_assets_landscape){ .size = 1.0f };
	VOE_TEST_CHECK(!read_groups(arena, "seed0=99\n", true, &out, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
	VOE_TEST_CHECK(out.size == 1.0f && out.heights == NULL);
}

static void a_new_group_takes_a_free_seed(voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, 4, arena);

	VOE_TEST_CHECK_INT(voe_assets_landscape_seed_new(&grid), 0);
	grid.group_count = 3;
	VOE_TEST_CHECK_INT(voe_assets_landscape_seed_new(&grid), 3);
	// Group 0 removed: the two left keep seeds 1 and 2, so 2 is taken.
	grid.groups[0].seed = 1 + 1;
	grid.groups[1].seed = 2 + 1;
	grid.group_count = 2;
	VOE_TEST_CHECK_INT(voe_assets_landscape_seed_new(&grid), 0);
	// Seeds 0 and 2 taken: the least free is 1.
	grid.groups[0].seed = 0;
	VOE_TEST_CHECK_INT(voe_assets_landscape_seed_new(&grid), 1);
}

enum { COPIES_A_GROUP = 2, THREE = 3, POINTS = 25 };

static bool same_look(voe_assets_landscape_look a, voe_assets_landscape_look b)
{
	return a.size == b.size && a.size_vary == b.size_vary &&
	       a.turn_vary == b.turn_vary && a.tilt == b.tilt &&
	       a.shadowless == b.shadowless && a.blends == b.blends &&
	       a.blend_height == b.blend_height && a.share == b.share;
}

// Field by field: the structs have padding, so never memcmp.
static bool same_group(const voe_assets_landscape_group *a,
		       const voe_assets_landscape_group *b)
{
	bool same = a->glb_count == b->glb_count && a->seed == b->seed;

	for (uint32_t j = 0; same && j < a->glb_count; j++)
		same = strcmp(a->glbs[j], b->glbs[j]) == 0 &&
		       same_look(a->looks[j], b->looks[j]);
	return same;
}

static bool same_copy(voe_assets_landscape_copy a, voe_assets_landscape_copy b)
{
	return a.x == b.x && a.z == b.z && a.turn == b.turn &&
	       a.scale == b.scale && a.group == b.group && a.lean == b.lean;
}

// Three groups on two layers, each look apart, layer 0 growing groups 0 and 2
// and layer 1 groups 1 and 2, and two copies of each group, interleaved.
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
	grid.group_count = THREE;
	for (uint32_t g = 0; g < THREE; g++) {
		voe_assets_landscape_look *look = &grid.groups[g].looks[0];

		one_glb(&grid, g, paths[g]);
		// Off 1000, which writes nothing and reads back as 0.
		look->size = (uint16_t)(1500 + 500 * g);
		look->tilt = (uint8_t)(10 * g);
		look->blends = g == 2;
		look->blend_height = g == 2 ? 120 : 0;
	}
	grid.grows[0][0] = 10;
	grid.grows[0][2] = 30;
	grid.grows[1][1] = 20;
	grid.grows[1][2] = 40;
	grid.copy_count = THREE * COPIES_A_GROUP;
	grid.copies = voe_base_arena_push(arena, grid.copy_count *
							 sizeof(*grid.copies));
	for (uint32_t i = 0; i < grid.copy_count; i++)
		grid.copies[i] = (voe_assets_landscape_copy){
			.x = (int32_t)(1000 * i), .z = -(int32_t)(500 * i),
			.turn = (uint16_t)(100 * i), .scale = (uint16_t)(900 + i),
			.group = (uint8_t)(i % THREE), .lean = (uint8_t)(i * 7)
		};
	return grid;
}

static void removing_a_middle_group_moves_the_later_down(voe_base_arena *arena)
{
	voe_assets_landscape grid = three_grown(arena);
	const voe_assets_landscape was = grid;
	voe_assets_landscape_copy before[THREE * COPIES_A_GROUP];
	uint32_t c = 0;

	memcpy(before, grid.copies, sizeof(before));
	voe_assets_landscape_group_remove(&grid, 1);
	VOE_TEST_CHECK_INT(grid.group_count, 2);
	VOE_TEST_CHECK(strcmp(grid.groups[0].glbs[0], "a.glb") == 0);
	VOE_TEST_CHECK(strcmp(grid.groups[1].glbs[0], "c.glb") == 0);
	VOE_TEST_CHECK(grid.groups[2].glbs[0][0] == '\0');
	VOE_TEST_CHECK_INT(grid.groups[2].glb_count, 0);
	VOE_TEST_CHECK(same_group(&grid.groups[0], &was.groups[0]));
	VOE_TEST_CHECK_INT(grid.groups[1].looks[0].size,
			   was.groups[2].looks[0].size);
	VOE_TEST_CHECK_INT(grid.groups[1].looks[0].tilt,
			   was.groups[2].looks[0].tilt);
	VOE_TEST_CHECK(grid.groups[1].looks[0].blends);
	VOE_TEST_CHECK_INT(grid.groups[1].looks[0].blend_height, 120);
	VOE_TEST_CHECK(same_look(grid.groups[2].looks[0],
				 (voe_assets_landscape_look){ 0 }));
	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&grid, 0), 0);
	VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&grid, 1), 2);
	for (uint32_t l = 0; l < 2; l++) {
		VOE_TEST_CHECK_INT(grid.grows[l][0], was.grows[l][0]);
		VOE_TEST_CHECK_INT(grid.grows[l][1], was.grows[l][2]);
		VOE_TEST_CHECK_INT(grid.grows[l][2], 0);
	}
	VOE_TEST_CHECK_INT(grid.copy_count, 2 * COPIES_A_GROUP);
	for (uint32_t i = 0; i < THREE * COPIES_A_GROUP; i++) {
		if (before[i].group == 1)
			continue;
		voe_assets_landscape_copy moved = before[i];

		moved.group = moved.group == 2 ? 1 : 0;
		VOE_TEST_CHECK(c < grid.copy_count &&
			       same_copy(grid.copies[c], moved));
		c++;
	}
	VOE_TEST_CHECK_INT(c, grid.copy_count);
}

// `back` equal to `grid` field by field, its groups, grows and copies too.
static bool same_landscape(const voe_assets_landscape *back,
			   const voe_assets_landscape *grid)
{
	bool same = back->size == grid->size && back->cells == grid->cells &&
		    memcmp(back->heights, grid->heights,
			   POINTS * sizeof(*back->heights)) == 0 &&
		    back->layer_count == grid->layer_count &&
		    memcmp(back->layers, grid->layers, sizeof(back->layers)) ==
			    0 &&
		    back->group_count == grid->group_count &&
		    memcmp(back->grows, grid->grows, sizeof(back->grows)) == 0 &&
		    back->copy_count == grid->copy_count;

	for (uint32_t g = 0; same && g < grid->group_count; g++)
		same = same_group(&back->groups[g], &grid->groups[g]);
	for (uint32_t i = 0; same && i < grid->copy_count; i++)
		same = same_copy(back->copies[i], grid->copies[i]);
	return same;
}

static void a_removal_round_trips_strictly(voe_base_arena *arena)
{
	voe_assets_landscape grid = three_grown(arena);
	voe_assets_landscape back;
	voe_assets_landscape_text written;

	voe_assets_landscape_group_remove(&grid, 1);
	written = voe_assets_landscape_write(&grid, arena);
	VOE_TEST_CHECK(strstr(written.text, "seed1=2\n") != NULL);
	VOE_TEST_CHECK(voe_assets_landscape_read_strict(
		written.text, written.size, arena, &back, NULL));
	VOE_TEST_CHECK(memcmp(back.paint, grid.paint, 8 * POINTS) == 0);
	VOE_TEST_CHECK(same_landscape(&back, &grid));
}

static void removing_the_last_group_leaves_the_others(voe_base_arena *arena)
{
	voe_assets_landscape grid = three_grown(arena);
	const voe_assets_landscape was = grid;
	voe_assets_landscape_copy before[THREE * COPIES_A_GROUP];
	uint32_t c = 0;

	memcpy(before, grid.copies, sizeof(before));
	voe_assets_landscape_group_remove(&grid, 2);
	VOE_TEST_CHECK_INT(grid.group_count, 2);
	VOE_TEST_CHECK(grid.groups[2].glbs[0][0] == '\0');
	for (uint32_t g = 0; g < 2; g++) {
		VOE_TEST_CHECK(same_group(&grid.groups[g], &was.groups[g]));
		for (uint32_t l = 0; l < 2; l++)
			VOE_TEST_CHECK_INT(grid.grows[l][g], was.grows[l][g]);
	}
	VOE_TEST_CHECK(same_look(grid.groups[2].looks[0],
				 (voe_assets_landscape_look){ 0 }));
	VOE_TEST_CHECK_INT(grid.grows[0][2] + grid.grows[1][2], 0);
	for (uint32_t i = 0; i < THREE * COPIES_A_GROUP; i++)
		if (before[i].group != 2)
			VOE_TEST_CHECK(c < grid.copy_count &&
				       same_copy(grid.copies[c++], before[i]));
	VOE_TEST_CHECK_INT(c, grid.copy_count);
}

static void a_new_seed_after_a_removal_is_free(voe_base_arena *arena)
{
	voe_assets_landscape grid = three_grown(arena);
	uint32_t seed;

	voe_assets_landscape_group_remove(&grid, 0);
	seed = voe_assets_landscape_seed_new(&grid);
	for (uint32_t g = 0; g < grid.group_count; g++)
		VOE_TEST_CHECK(seed != voe_assets_landscape_group_seed(&grid, g));
	VOE_TEST_CHECK_INT(seed, 0);
}

// Two kinds in the old spelling, kind 0's look off the default, kind 1 seeded
// 7, and a copy of each.
static const char *const old_spelling =
	"[Landscape]\nsize=64\ncells=4\n[Foliage]\nkind0=\"a.glb\"\n"
	"size0=2000\nvary0=35\nturn0=0\ntilt0=100\nshadow0=0\nblend0=1\n"
	"blendheight0=120\nkind1=\"b.glb\"\nseed1=7\n%s"
	"[Copies]\nc0=1 -2 3 4 1000 5\nc1=0 6 7 8 900 9\n";

static void the_old_spelling_reads_as_one_glb_groups(voe_base_arena *arena)
{
	const voe_assets_landscape_look fallback =
		voe_assets_landscape_look_default();
	char text[1024];

	snprintf(text, sizeof(text), old_spelling, four_flat_rows);
	for (int strict = 0; strict < 2; strict++) {
		voe_assets_landscape out;
		const voe_assets_landscape_look *look = &out.groups[0].looks[0];

		VOE_TEST_CHECK(read_text(arena, text, strict, &out, NULL));
		VOE_TEST_CHECK_INT(out.group_count, 2);
		VOE_TEST_CHECK_INT(out.groups[0].glb_count, 1);
		VOE_TEST_CHECK_INT(out.groups[1].glb_count, 1);
		VOE_TEST_CHECK(strcmp(out.groups[0].glbs[0], "a.glb") == 0);
		VOE_TEST_CHECK(strcmp(out.groups[1].glbs[0], "b.glb") == 0);
		VOE_TEST_CHECK(look->size == 2000 && look->size_vary == 35 &&
			       look->turn_vary == 0 && look->tilt == 100 &&
			       look->shadowless && look->blends &&
			       look->blend_height == 120 && look->share == 1);
		VOE_TEST_CHECK(same_look(out.groups[1].looks[0], fallback));
		VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&out, 0), 0);
		VOE_TEST_CHECK_INT(voe_assets_landscape_group_seed(&out, 1), 7);
		VOE_TEST_CHECK_INT(out.copy_count, 2);
		VOE_TEST_CHECK(same_copy(out.copies[0],
					 (voe_assets_landscape_copy){
						 .x = -2, .z = 3, .turn = 4,
						 .scale = 1000, .group = 1,
						 .lean = 5 }));
		VOE_TEST_CHECK(same_copy(out.copies[1],
					 (voe_assets_landscape_copy){
						 .x = 6, .z = 7, .turn = 8,
						 .scale = 900, .group = 0,
						 .lean = 9 }));
	}
}

// Group 0 of three glbs with shares 0, 3 and the default, the second at 2.5×;
// group 1 of one glb.
static voe_assets_landscape two_groups(voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, 4, arena);
	static const char *const paths[THREE] = { "a.glb", "b.glb", "c.glb" };

	for (uint32_t j = 0; j < THREE; j++) {
		memcpy(grid.groups[0].glbs[j], paths[j], strlen(paths[j]) + 1);
		grid.groups[0].looks[j] = voe_assets_landscape_look_default();
	}
	grid.groups[0].glb_count = THREE;
	grid.groups[0].looks[0].share = 0;
	grid.groups[0].looks[1].share = 3;
	grid.groups[0].looks[1].size = 2500;
	one_glb(&grid, 1, "d.glb");
	grid.group_count = 2;
	return grid;
}

static void groups_round_trip_with_no_kind_key(voe_base_arena *arena)
{
	const voe_assets_landscape grid = two_groups(arena);
	const voe_assets_landscape_text written =
		voe_assets_landscape_write(&grid, arena);
	voe_assets_landscape back;

	VOE_TEST_CHECK(strstr(written.text, "kind") == NULL);
	VOE_TEST_CHECK(strstr(written.text,
			      "[Foliage]\nglb0_0=\"a.glb\"\nshare0_0=0\n"
			      "glb0_1=\"b.glb\"\nsize0_1=2500\nshare0_1=3\n"
			      "glb0_2=\"c.glb\"\nglb1_0=\"d.glb\"\n\n") != NULL);
	VOE_TEST_CHECK(voe_assets_landscape_read_strict(
		written.text, written.size, arena, &back, NULL));
	VOE_TEST_CHECK(same_landscape(&back, &grid));
	VOE_TEST_CHECK(voe_assets_landscape_glb_size(&back, 0, 1) == 2.5f);
	VOE_TEST_CHECK(voe_assets_landscape_glb_size(&back, 0, 0) == 1.0f);
}

static void a_mixed_spelling_or_bad_group_is_malformed(voe_base_arena *arena)
{
	const char *const foliage[] = {
		// Both spellings.
		"kind0=\"a.glb\"\nglb1_0=\"b.glb\"\n",
		"glb0_0=\"a.glb\"\nkind1=\"b.glb\"\n",
		// A share past 10.
		"glb0_0=\"a.glb\"\nshare0_0=11\n",
		// A path twice in one group.
		"glb0_0=\"a.glb\"\nglb0_1=\"a.glb\"\n",
	};
	char text[512];

	for (size_t i = 0; i < sizeof(foliage) / sizeof(foliage[0]); i++) {
		voe_assets_landscape out = { .size = 1.0f };
		voe_base_error error = VOE_BASE_OK;

		snprintf(text, sizeof(text),
			 "[Landscape]\nsize=64\ncells=4\n[Foliage]\n%s%s",
			 foliage[i], four_flat_rows);
		VOE_TEST_CHECK(!read_text(arena, text, true, &out, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
		VOE_TEST_CHECK(out.size == 1.0f && out.heights == NULL);
	}
	// The same path in two groups is no repeat.
	snprintf(text, sizeof(text),
		 "[Landscape]\nsize=64\ncells=4\n[Foliage]\nglb0_0=\"a.glb\"\n"
		 "share0_0=10\nglb1_0=\"a.glb\"\n%s",
		 four_flat_rows);
	VOE_TEST_CHECK(
		read_text(arena, text, true, &(voe_assets_landscape){ 0 }, NULL));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	missing_seeds_are_the_indices(arena);
	a_set_seed_round_trips(arena);
	an_out_of_range_seed_is_clamped_or_refused(arena);
	a_new_group_takes_a_free_seed(arena);
	removing_a_middle_group_moves_the_later_down(arena);
	a_removal_round_trips_strictly(arena);
	removing_the_last_group_leaves_the_others(arena);
	a_new_seed_after_a_removal_is_free(arena);
	the_old_spelling_reads_as_one_glb_groups(arena);
	groups_round_trip_with_no_kind_key(arena);
	a_mixed_spelling_or_bad_group_is_malformed(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
