// A foliage kind's seed (0517 point 4): a file with three kinds and no seed
// keys reads seeds 0, 1 and 2 and writes no `seed` line; a look with seed 5 on
// kind 1 writes `seed1=5` and reads back 5; `seed0=99` is clamped to 15 by the
// lenient read and refused MALFORMED by the strict one; and a new kind's seed
// is the kind count on a fresh landscape, the least free seed when that one is
// taken.
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

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	missing_seeds_are_the_indices(arena);
	a_set_seed_round_trips(arena);
	an_out_of_range_seed_is_clamped_or_refused(arena);
	a_new_kind_takes_a_free_seed(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
