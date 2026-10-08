// The landscape file: a written grid reads back as itself, heights round to the
// millimetre, and each refusal comes with its category.
//
// The files refused are written by hand at four cells, the smallest a file may
// have, so each case shows the one thing wrong with it.
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

static bool read_text(voe_base_arena *arena, const char *text,
		      voe_assets_landscape *out, voe_base_error *error)
{
	return voe_assets_landscape_read(text, strlen(text), arena, out, error);
}

// Refused with this category, and `out` left as it was.
static void refused_as(voe_base_arena *arena, const char *text,
		       voe_base_error expected)
{
	voe_assets_landscape out = { .size = 1.0f };
	voe_base_error error = VOE_BASE_OK;

	VOE_TEST_CHECK(!read_text(arena, text, &out, &error));
	VOE_TEST_CHECK_INT(error, expected);
	VOE_TEST_CHECK(out.size == 1.0f && out.heights == NULL);
}

static void flat_written_reads_back_the_same(voe_base_arena *arena)
{
	voe_assets_landscape flat = voe_assets_landscape_flat(
		VOE_ASSETS_LANDSCAPE_SIZE_DEFAULT, VOE_ASSETS_LANDSCAPE_CELLS,
		arena);
	voe_assets_landscape_text text = voe_assets_landscape_write(&flat, arena);
	voe_assets_landscape back;
	const size_t count = (VOE_ASSETS_LANDSCAPE_CELLS + 1) *
			     (VOE_ASSETS_LANDSCAPE_CELLS + 1);
	bool all_zero = true;

	VOE_TEST_CHECK_INT(strlen(text.text), text.size);
	VOE_TEST_CHECK(voe_assets_landscape_read(text.text, text.size, arena,
						 &back, NULL));
	VOE_TEST_CHECK(back.size == VOE_ASSETS_LANDSCAPE_SIZE_DEFAULT);
	VOE_TEST_CHECK_INT(back.cells, VOE_ASSETS_LANDSCAPE_CELLS);
	for (size_t i = 0; i < count; i++)
		all_zero = all_zero && back.heights[i] == 0.0f;
	VOE_TEST_CHECK(all_zero);
}

static void heights_round_to_the_millimetre(voe_base_arena *arena)
{
	voe_assets_landscape small = voe_assets_landscape_flat(100.5f, 4, arena);
	voe_assets_landscape back;
	voe_assets_landscape_text text;

	small.heights[0] = 1.2344f;
	small.heights[1] = 1.2346f;
	small.heights[2] = -0.0004f;
	small.heights[3] = -2.5006f;
	small.heights[24] = 812.0f;
	text = voe_assets_landscape_write(&small, arena);

	// Whole millimetres in the text, and no "-0".
	VOE_TEST_CHECK(strstr(text.text, "row0=1234 1235 0 -2501 0\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "row4=0 0 0 0 812000\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "size=100.5\n") != NULL);

	VOE_TEST_CHECK(voe_assets_landscape_read(text.text, text.size, arena,
						 &back, NULL));
	VOE_TEST_CHECK(back.size == 100.5f);
	VOE_TEST_CHECK_INT(back.cells, 4);
	VOE_TEST_CHECK(back.heights[0] == 1.234f);
	VOE_TEST_CHECK(back.heights[1] == 1.235f);
	VOE_TEST_CHECK(back.heights[2] == 0.0f);
	VOE_TEST_CHECK(back.heights[3] == -2.501f);
	VOE_TEST_CHECK(back.heights[24] == 812.0f);
}

static void a_short_row_is_malformed(voe_base_arena *arena)
{
	const char *const head = "[Landscape]\nsize=64\ncells=4\n[Heights]\n"
				 "row0=0 0 0 0 0\nrow1=0 0 0 0 0\n"
				 "row2=0 0 0 0 0\nrow3=0 0 0 0 0\n";
	char text[256];

	snprintf(text, sizeof(text), "%s%s", head, "row4=0 0 0 0\n");
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	// Too many, and one not whole, are the same refusal.
	snprintf(text, sizeof(text), "%s%s", head, "row4=0 0 0 0 0 0\n");
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	snprintf(text, sizeof(text), "%s%s", head, "row4=0 0 0.5 0 0\n");
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	// And the same head with a good last row is read.
	snprintf(text, sizeof(text), "%s%s", head, "row4=0 0 -7 0 0\n");
	{
		voe_assets_landscape out;

		VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
		VOE_TEST_CHECK(out.heights[22] == -0.007f);
	}
}

static void a_missing_row_is_malformed(voe_base_arena *arena)
{
	refused_as(arena,
		   "[Landscape]\nsize=64\ncells=4\n[Heights]\n"
		   "row0=0 0 0 0 0\nrow1=0 0 0 0 0\n"
		   "row3=0 0 0 0 0\nrow4=0 0 0 0 0\n",
		   VOE_BASE_ERROR_MALFORMED);
	// A missing key, a missing section, and text that is not sectioned.
	refused_as(arena, "[Landscape]\nsize=64\n[Heights]\n",
		   VOE_BASE_ERROR_MALFORMED);
	refused_as(arena, "[Landscape]\nsize=64\ncells=4\n",
		   VOE_BASE_ERROR_MALFORMED);
	refused_as(arena, "not a landscape\n", VOE_BASE_ERROR_MALFORMED);
}

static void cells_not_a_multiple_of_four_is_unsupported(voe_base_arena *arena)
{
	refused_as(arena, "[Landscape]\nsize=64\ncells=6\n[Heights]\n",
		   VOE_BASE_ERROR_UNSUPPORTED);
	refused_as(arena, "[Landscape]\nsize=64\ncells=0\n[Heights]\n",
		   VOE_BASE_ERROR_UNSUPPORTED);
	refused_as(arena, "[Landscape]\nsize=64\ncells=516\n[Heights]\n",
		   VOE_BASE_ERROR_UNSUPPORTED);
}

static void a_size_out_of_range_is_unsupported(voe_base_arena *arena)
{
	refused_as(arena, "[Landscape]\nsize=15.9\ncells=4\n[Heights]\n",
		   VOE_BASE_ERROR_UNSUPPORTED);
	refused_as(arena, "[Landscape]\nsize=8193\ncells=4\n[Heights]\n",
		   VOE_BASE_ERROR_UNSUPPORTED);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	flat_written_reads_back_the_same(arena);
	heights_round_to_the_millimetre(arena);
	a_short_row_is_malformed(arena);
	a_missing_row_is_malformed(arena);
	cells_not_a_multiple_of_four_is_unsupported(arena);
	a_size_out_of_range_is_unsupported(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
