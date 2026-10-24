// The landscape file: a written grid reads back as itself up to 2048 cells,
// heights round to the millimetre, a resample keeps a plane and its corners,
// layers and paint round-trip byte for byte and resample to sums of 255,
// foliage kinds, their looks, each layer's grows and copies round-trip byte for
// byte and a resample keeps them, and each refusal comes with its category.
//
// The files refused are written by hand at four cells, the smallest a file may
// have, so each case shows the one thing wrong with it.
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <math.h>
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

static void the_largest_grid_reads_back_the_same(voe_base_arena *arena)
{
	const uint32_t cells = VOE_ASSETS_LANDSCAPE_CELLS_MAX;
	const size_t count = ((size_t)cells + 1) * ((size_t)cells + 1);
	voe_assets_landscape big = voe_assets_landscape_flat(4096.0f, cells,
							     arena);
	voe_assets_landscape_text text;
	voe_assets_landscape back;
	bool same = true;

	for (size_t i = 0; i < count; i++)
		big.heights[i] = (float)((double)(i % 1000) / 1000.0);
	text = voe_assets_landscape_write(&big, arena);
	VOE_TEST_CHECK(voe_assets_landscape_read(text.text, text.size, arena,
						 &back, NULL));
	VOE_TEST_CHECK_INT(back.cells, cells);
	for (size_t i = 0; i < count; i++)
		same = same && back.heights[i] == big.heights[i];
	VOE_TEST_CHECK(same);
}

// The plane h = 0.5 x + 0.25 z over a 64 m square at `cells`.
static voe_assets_landscape tilted_plane(uint32_t cells, voe_base_arena *arena)
{
	voe_assets_landscape plane = voe_assets_landscape_flat(64.0f, cells,
							       arena);
	const float step = 64.0f / (float)cells;

	for (uint32_t r = 0; r <= cells; r++)
		for (uint32_t c = 0; c <= cells; c++)
			plane.heights[r * (cells + 1) + c] =
				0.5f * (-32.0f + (float)c * step) +
				0.25f * (-32.0f + (float)r * step);
	return plane;
}

static void a_plane_resampled_up_and_down_is_the_same(voe_base_arena *arena)
{
	const voe_assets_landscape eight = tilted_plane(8, arena);
	const voe_assets_landscape sixteen_expected = tilted_plane(16, arena);
	const voe_assets_landscape sixteen =
		voe_assets_landscape_resample(&eight, 16, arena);
	const voe_assets_landscape back =
		voe_assets_landscape_resample(&sixteen, 8, arena);
	bool close = true;

	VOE_TEST_CHECK_INT(sixteen.cells, 16);
	VOE_TEST_CHECK(sixteen.size == 64.0f && back.size == 64.0f);
	for (size_t i = 0; i < 17 * 17; i++)
		close = close && fabsf(sixteen.heights[i] -
				       sixteen_expected.heights[i]) < 1e-4f;
	for (size_t i = 0; i < 9 * 9; i++)
		close = close && fabsf(back.heights[i] - eight.heights[i]) < 1e-4f;
	VOE_TEST_CHECK(close);
}

static void a_resample_keeps_corners_and_the_same_count_copies(
	voe_base_arena *arena)
{
	voe_assets_landscape rough = voe_assets_landscape_flat(64.0f, 8, arena);
	voe_assets_landscape twelve;
	voe_assets_landscape copy;

	for (size_t i = 0; i < 9 * 9; i++)
		rough.heights[i] = (float)((i * 37) % 11) - 5.0f;
	twelve = voe_assets_landscape_resample(&rough, 12, arena);
	VOE_TEST_CHECK(twelve.heights[0] == rough.heights[0]);
	VOE_TEST_CHECK(twelve.heights[12] == rough.heights[8]);
	VOE_TEST_CHECK(twelve.heights[12 * 13] == rough.heights[8 * 9]);
	VOE_TEST_CHECK(twelve.heights[13 * 13 - 1] == rough.heights[9 * 9 - 1]);

	copy = voe_assets_landscape_resample(&rough, 8, arena);
	VOE_TEST_CHECK(copy.heights != rough.heights);
	VOE_TEST_CHECK(memcmp(copy.heights, rough.heights,
			      9 * 9 * sizeof(float)) == 0);
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
	refused_as(arena, "[Landscape]\nsize=64\ncells=2052\n[Heights]\n",
		   VOE_BASE_ERROR_UNSUPPORTED);
}

static void a_size_out_of_range_is_unsupported(voe_base_arena *arena)
{
	refused_as(arena, "[Landscape]\nsize=15.9\ncells=4\n[Heights]\n",
		   VOE_BASE_ERROR_UNSUPPORTED);
	refused_as(arena, "[Landscape]\nsize=8193\ncells=4\n[Heights]\n",
		   VOE_BASE_ERROR_UNSUPPORTED);
}

// Layer `layer`'s weight at point `point`.
static uint8_t weight(const voe_assets_landscape *landscape, size_t point,
		      uint32_t layer)
{
	const size_t points = ((size_t)landscape->cells + 1) *
			      ((size_t)landscape->cells + 1);

	return landscape->paint[(layer / 4) * points * 4 + point * 4 +
				layer % 4];
}

static void set_weight(voe_assets_landscape *landscape, size_t point,
		       uint32_t layer, uint8_t value)
{
	const size_t points = ((size_t)landscape->cells + 1) *
			      ((size_t)landscape->cells + 1);

	landscape->paint[(layer / 4) * points * 4 + point * 4 + layer % 4] =
		value;
}

// Every point's eight weights sum to 255.
static bool every_point_sums_to_255(const voe_assets_landscape *landscape)
{
	const size_t points = ((size_t)landscape->cells + 1) *
			      ((size_t)landscape->cells + 1);
	bool all = true;

	for (size_t p = 0; p < points; p++) {
		uint32_t sum = 0;

		for (uint32_t k = 0; k < VOE_ASSETS_LANDSCAPE_LAYERS; k++)
			sum += weight(landscape, p, k);
		all = all && sum == 255;
	}
	return all;
}

// A grid with `layer_count` layers and paint on `arena`, all layer 0.
static voe_assets_landscape unpainted(uint32_t cells, uint32_t layer_count,
				      voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, cells,
							      arena);
	const size_t points = ((size_t)cells + 1) * ((size_t)cells + 1);

	grid.layer_count = layer_count;
	for (uint32_t k = 0; k < layer_count; k++)
		snprintf(grid.layers[k], sizeof(grid.layers[k]),
			 "Assets/layer \"%u\".material", k);
	grid.paint = voe_base_arena_push(arena, 8 * points);
	for (size_t p = 0; p < points; p++)
		set_weight(&grid, p, 0, 255);
	return grid;
}

static void layers_and_paint_round_trip_byte_for_byte(voe_base_arena *arena)
{
	voe_assets_landscape grid = unpainted(4, 3, arena);
	voe_assets_landscape_text text;
	voe_assets_landscape_text again;
	voe_assets_landscape back;

	for (size_t p = 6; p <= 8; p++) {
		set_weight(&grid, p, 1, 100);
		set_weight(&grid, p, 0, 155);
	}
	set_weight(&grid, 12, 2, 200);
	set_weight(&grid, 12, 1, 55);
	set_weight(&grid, 12, 0, 0);
	text = voe_assets_landscape_write(&grid, arena);

	VOE_TEST_CHECK(strstr(text.text, "[Layers]\nlayer0=\"Assets/layer "
					 "\\\"0\\\".material\"\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "paint1_1=0 100*3 0\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "paint1_2=0 0 55 0 0\n") != NULL);
	// An unpainted layer row is one run, and layer 0 is never written.
	VOE_TEST_CHECK(strstr(text.text, "paint2_0=0*5\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "paint0_") == NULL);

	VOE_TEST_CHECK(voe_assets_landscape_read(text.text, text.size, arena,
						 &back, NULL));
	VOE_TEST_CHECK_INT(back.layer_count, 3);
	VOE_TEST_CHECK(strcmp(back.layers[0], grid.layers[0]) == 0);
	VOE_TEST_CHECK(strcmp(back.layers[2], grid.layers[2]) == 0);
	VOE_TEST_CHECK(memcmp(back.paint, grid.paint, 8 * 25) == 0);
	again = voe_assets_landscape_write(&back, arena);
	VOE_TEST_CHECK_INT(again.size, text.size);
	VOE_TEST_CHECK(strcmp(again.text, text.text) == 0);
}

static const char *const four_flat_rows =
	"[Heights]\nrow0=0 0 0 0 0\nrow1=0 0 0 0 0\nrow2=0 0 0 0 0\n"
	"row3=0 0 0 0 0\nrow4=0 0 0 0 0\n";

static void layers_and_no_paint_read_as_all_layer_0(voe_base_arena *arena)
{
	char text[512];
	voe_assets_landscape out;
	bool ground = true;

	snprintf(text, sizeof(text),
		 "[Landscape]\nsize=64\ncells=4\n[Layers]\nlayer0=\"a.material\"\n"
		 "layer1=\"b.material\"\nlayer3=\"lost.material\"\n%s",
		 four_flat_rows);
	VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
	// Read to the first missing key: layer3 is past the gap.
	VOE_TEST_CHECK_INT(out.layer_count, 2);
	VOE_TEST_CHECK(strcmp(out.layers[1], "b.material") == 0);
	VOE_TEST_CHECK(out.paint != NULL);
	for (size_t p = 0; p < 25; p++)
		ground = ground && weight(&out, p, 0) == 255;
	VOE_TEST_CHECK(ground && every_point_sums_to_255(&out));

	// No layers at all: no paint either.
	snprintf(text, sizeof(text), "[Landscape]\nsize=64\ncells=4\n%s",
		 four_flat_rows);
	VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
	VOE_TEST_CHECK_INT(out.layer_count, 0);
	VOE_TEST_CHECK(out.paint == NULL);
}

static void bad_paint_and_layers_are_malformed(voe_base_arena *arena)
{
	const char *const head =
		"[Landscape]\nsize=64\ncells=4\n[Layers]\nlayer0=\"a.material\"\n"
		"layer1=\"b.material\"\nlayer2=\"c.material\"\n";
	const char *const rows =
		"paint1_1=0*5\npaint1_2=0*5\npaint1_3=0*5\npaint1_4=0*5\n"
		"paint2_1=0*5\npaint2_2=0*5\npaint2_3=0*5\npaint2_4=0*5\n";
	char text[1024];

	// Layers 1 and 2 at one point pass 255.
	snprintf(text, sizeof(text), "%s%s[Paint]\n%spaint1_0=200 0*4\n"
		 "paint2_0=100 0*4\n", head, four_flat_rows, rows);
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	// A row of four numbers.
	snprintf(text, sizeof(text), "%s%s[Paint]\n%spaint1_0=0 0 0 0\n"
		 "paint2_0=0*5\n", head, four_flat_rows, rows);
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	// A value past 255, and a row missing.
	snprintf(text, sizeof(text), "%s%s[Paint]\n%spaint1_0=256 0*4\n"
		 "paint2_0=0*5\n", head, four_flat_rows, rows);
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	snprintf(text, sizeof(text), "%s%s[Paint]\n%spaint1_0=0*5\n", head,
		 four_flat_rows, rows);
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	// A path not quoted.
	snprintf(text, sizeof(text),
		 "[Landscape]\nsize=64\ncells=4\n[Layers]\nlayer0=a.material\n%s",
		 four_flat_rows);
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	// And the same good rows are read.
	snprintf(text, sizeof(text), "%s%s[Paint]\n%spaint1_0=200 0*4\n"
		 "paint2_0=55 0*4\n", head, four_flat_rows, rows);
	{
		voe_assets_landscape out;

		VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
		VOE_TEST_CHECK_INT(weight(&out, 0, 0), 0);
		VOE_TEST_CHECK_INT(weight(&out, 0, 2), 55);
	}
}

static void a_resample_of_paint_sums_to_255_and_keeps_corners(
	voe_base_arena *arena)
{
	voe_assets_landscape halves = unpainted(4, 2, arena);
	const uint32_t counts[] = { 12, 8, 4 };

	// Columns 0-1 all layer 0, columns 2-4 all layer 1.
	for (size_t p = 0; p < 25; p++)
		if (p % 5 >= 2) {
			set_weight(&halves, p, 0, 0);
			set_weight(&halves, p, 1, 255);
		}
	for (size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); i++) {
		const uint32_t cells = counts[i];
		const voe_assets_landscape to =
			voe_assets_landscape_resample(&halves, cells, arena);
		const size_t last = (size_t)(cells + 1) * (cells + 1) - 1;

		VOE_TEST_CHECK_INT(to.layer_count, 2);
		VOE_TEST_CHECK(strcmp(to.layers[1], halves.layers[1]) == 0);
		VOE_TEST_CHECK(every_point_sums_to_255(&to));
		VOE_TEST_CHECK_INT(weight(&to, 0, 0), 255);
		VOE_TEST_CHECK_INT(weight(&to, cells, 1), 255);
		VOE_TEST_CHECK_INT(weight(&to, (size_t)cells * (cells + 1), 0),
				   255);
		VOE_TEST_CHECK_INT(weight(&to, last, 1), 255);
	}
	// No paint stays no paint.
	halves.paint = NULL;
	VOE_TEST_CHECK(voe_assets_landscape_resample(&halves, 8, arena).paint ==
		       NULL);
}

// Field by field: the struct has padding, so never memcmp.
static bool same_copy(voe_assets_landscape_copy a, voe_assets_landscape_copy b)
{
	return a.x == b.x && a.z == b.z && a.turn == b.turn &&
	       a.scale == b.scale && a.kind == b.kind && a.lean == b.lean;
}

// Three copies of two kinds: a negative x, a lean of 255, a scale of 65535.
static const voe_assets_landscape_copy three_copies[3] = {
	{ -1500, 2000, 16384, 1000, 0, 0 },
	{ 3000, -4000, 0, 500, 1, 255 },
	{ 0, 7, 65535, 65535, 1, 9 },
};

// A 4-cell grid with two kinds and `three_copies`, copied onto `arena`.
static voe_assets_landscape forested(voe_base_arena *arena)
{
	voe_assets_landscape grid = voe_assets_landscape_flat(64.0f, 4, arena);

	snprintf(grid.kinds[0], sizeof(grid.kinds[0]), "Assets/pine.glb");
	snprintf(grid.kinds[1], sizeof(grid.kinds[1]), "Assets/\"birch\".glb");
	grid.kind_count = 2;
	grid.looks[0] = voe_assets_landscape_look_default();
	grid.looks[1] = voe_assets_landscape_look_default();
	grid.copies = voe_base_arena_push(arena, sizeof(three_copies));
	memcpy(grid.copies, three_copies, sizeof(three_copies));
	grid.copy_count = 3;
	return grid;
}

static void foliage_round_trips_byte_for_byte(voe_base_arena *arena)
{
	const voe_assets_landscape grid = forested(arena);
	const voe_assets_landscape_text text =
		voe_assets_landscape_write(&grid, arena);
	voe_assets_landscape_text again;
	voe_assets_landscape back;
	bool same = true;

	VOE_TEST_CHECK(strstr(text.text,
			      "\n\n[Foliage]\nkind0=\"Assets/pine.glb\"\n"
			      "kind1=\"Assets/\\\"birch\\\".glb\"\n\n"
			      "[Heights]\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text,
			      "\n\n[Copies]\nc0=0 -1500 2000 16384 1000 0\n"
			      "c1=1 3000 -4000 0 500 255\n"
			      "c2=1 0 7 65535 65535 9\n") != NULL);

	VOE_TEST_CHECK(voe_assets_landscape_read(text.text, text.size, arena,
						 &back, NULL));
	VOE_TEST_CHECK_INT(back.kind_count, 2);
	VOE_TEST_CHECK(strcmp(back.kinds[0], grid.kinds[0]) == 0);
	VOE_TEST_CHECK(strcmp(back.kinds[1], grid.kinds[1]) == 0);
	VOE_TEST_CHECK_INT(back.copy_count, 3);
	VOE_TEST_CHECK_INT(back.copies[0].x, -1500);
	VOE_TEST_CHECK_INT(back.copies[0].z, 2000);
	VOE_TEST_CHECK_INT(back.copies[0].turn, 16384);
	VOE_TEST_CHECK_INT(back.copies[1].lean, 255);
	VOE_TEST_CHECK_INT(back.copies[1].kind, 1);
	VOE_TEST_CHECK_INT(back.copies[2].scale, 65535);
	for (uint32_t i = 0; i < 3; i++)
		same = same && same_copy(back.copies[i], three_copies[i]);
	VOE_TEST_CHECK(same);
	again = voe_assets_landscape_write(&back, arena);
	VOE_TEST_CHECK_INT(again.size, text.size);
	VOE_TEST_CHECK(strcmp(again.text, text.text) == 0);
}

static const char *const two_kinds =
	"[Landscape]\nsize=64\ncells=4\n[Foliage]\nkind0=\"a.glb\"\n"
	"kind1=\"b.glb\"\n";

static void no_copies_read_as_none(voe_base_arena *arena)
{
	char text[512];
	voe_assets_landscape out;

	snprintf(text, sizeof(text), "%s%s", two_kinds, four_flat_rows);
	VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
	VOE_TEST_CHECK_INT(out.kind_count, 2);
	VOE_TEST_CHECK_INT(out.copy_count, 0);
	VOE_TEST_CHECK(out.copies == NULL);
}

static void bad_copies_are_malformed(voe_base_arena *arena)
{
	const char *const bad[] = {
		// Out of order.
		"c1=0 0 0 0 1000 0\nc0=0 0 0 0 1000 0\n",
		// A kind 2 with two kinds.
		"c0=2 0 0 0 1000 0\n",
		// A scale of 0.
		"c0=0 0 0 0 0 0\n",
		// Five numbers.
		"c0=0 0 0 0 1000\n",
	};
	char text[1024];

	for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		snprintf(text, sizeof(text), "%s%s[Copies]\n%s", two_kinds,
			 four_flat_rows, bad[i]);
		refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	}
	// A kind path not quoted.
	snprintf(text, sizeof(text),
		 "[Landscape]\nsize=64\ncells=4\n[Foliage]\nkind0=a.glb\n%s",
		 four_flat_rows);
	refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	// And a good row is read.
	snprintf(text, sizeof(text), "%s%s[Copies]\nc0=1 -2 3 4 1000 5\n",
		 two_kinds, four_flat_rows);
	{
		voe_assets_landscape out;

		VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
		VOE_TEST_CHECK_INT(out.copy_count, 1);
		VOE_TEST_CHECK(same_copy(out.copies[0],
					 (voe_assets_landscape_copy){
						 -2, 3, 4, 1000, 1, 5 }));
	}
}

static void a_resample_keeps_the_copies(voe_base_arena *arena)
{
	const voe_assets_landscape grid = forested(arena);
	const voe_assets_landscape eight =
		voe_assets_landscape_resample(&grid, 8, arena);
	bool same = true;

	VOE_TEST_CHECK_INT(eight.kind_count, 2);
	VOE_TEST_CHECK(strcmp(eight.kinds[1], grid.kinds[1]) == 0);
	VOE_TEST_CHECK_INT(eight.copy_count, 3);
	VOE_TEST_CHECK(eight.copies != grid.copies);
	for (uint32_t i = 0; i < 3; i++)
		same = same && same_copy(eight.copies[i], three_copies[i]);
	VOE_TEST_CHECK(same);
	// No copies stays none.
	VOE_TEST_CHECK(voe_assets_landscape_resample(
			       &(voe_assets_landscape){ .size = 64.0f,
							.cells = 4,
							.heights = grid.heights },
			       8, arena)
			       .copies == NULL);
}

static void kind_sizes_round_trip_byte_for_byte(voe_base_arena *arena)
{
	voe_assets_landscape grid = forested(arena);
	voe_assets_landscape_text text;
	voe_assets_landscape_text again;
	voe_assets_landscape back;

	grid.looks[1].size = 2000;
	text = voe_assets_landscape_write(&grid, arena);
	VOE_TEST_CHECK(strstr(text.text, "kind0=\"Assets/pine.glb\"\nkind1=") !=
		       NULL);
	VOE_TEST_CHECK(strstr(text.text, "\\\"birch\\\".glb\"\nsize1=2000\n\n"
					 "[Heights]\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "size0=") == NULL);

	VOE_TEST_CHECK(voe_assets_landscape_read(text.text, text.size, arena,
						 &back, NULL));
	VOE_TEST_CHECK_INT(back.looks[0].size, 0);
	VOE_TEST_CHECK_INT(back.looks[1].size, 2000);
	VOE_TEST_CHECK(voe_assets_landscape_kind_size(&back, 0) == 1.0f);
	VOE_TEST_CHECK(voe_assets_landscape_kind_size(&back, 1) == 2.0f);
	again = voe_assets_landscape_write(&back, arena);
	VOE_TEST_CHECK_INT(again.size, text.size);
	VOE_TEST_CHECK(strcmp(again.text, text.text) == 0);

	// A kind at exactly 1× is written without its size line.
	grid.looks[0].size = 1000;
	text = voe_assets_landscape_write(&grid, arena);
	VOE_TEST_CHECK(strstr(text.text, "size0=") == NULL);
	VOE_TEST_CHECK(strstr(text.text, "size1=2000\n") != NULL);
}

static void a_bad_kind_size_is_malformed(voe_base_arena *arena)
{
	const char *const bad[] = { "0", "99", "10001", "1.5" };
	char text[512];

	for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
		snprintf(text, sizeof(text), "%ssize0=%s\n%s", two_kinds,
			 bad[i], four_flat_rows);
		refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	}
	// And the range's ends are read.
	snprintf(text, sizeof(text), "%ssize0=100\nsize1=10000\n%s", two_kinds,
		 four_flat_rows);
	{
		voe_assets_landscape out;

		VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
		VOE_TEST_CHECK(voe_assets_landscape_kind_size(&out, 0) == 0.1f);
		VOE_TEST_CHECK(voe_assets_landscape_kind_size(&out, 1) ==
			       10.0f);
	}
}

static void a_resample_keeps_the_kind_sizes(voe_base_arena *arena)
{
	voe_assets_landscape grid = forested(arena);
	voe_assets_landscape eight;

	grid.looks[0].size = 250;
	grid.looks[1].size = 2000;
	eight = voe_assets_landscape_resample(&grid, 8, arena);
	VOE_TEST_CHECK_INT(eight.looks[0].size, 250);
	VOE_TEST_CHECK_INT(eight.looks[1].size, 2000);
	VOE_TEST_CHECK(voe_assets_landscape_kind_size(&eight, 1) == 2.0f);
}

static void a_flat_grids_kind_added_by_hand_is_1x(voe_base_arena *arena)
{
	voe_assets_landscape flat = voe_assets_landscape_flat(64.0f, 4, arena);

	snprintf(flat.kinds[0], sizeof(flat.kinds[0]), "Assets/oak.glb");
	flat.kind_count = 1;
	VOE_TEST_CHECK_INT(flat.looks[0].size, 0);
	VOE_TEST_CHECK(voe_assets_landscape_kind_size(&flat, 0) == 1.0f);
}

static bool same_look(voe_assets_landscape_look a, voe_assets_landscape_look b)
{
	return a.size == b.size && a.size_vary == b.size_vary &&
	       a.turn_vary == b.turn_vary && a.tilt == b.tilt &&
	       a.shadowless == b.shadowless;
}

// `forested` on two layers, kind 0 with every look key off the default and
// kind 1 shadowless, layer 0 growing kind 1 and layer 1 both.
static voe_assets_landscape grown(voe_base_arena *arena)
{
	voe_assets_landscape grid = forested(arena);

	grid.layer_count = 2;
	snprintf(grid.layers[0], sizeof(grid.layers[0]), "Assets/grass.material");
	snprintf(grid.layers[1], sizeof(grid.layers[1]), "Assets/rock.material");
	// All layer 0, as a read gives it.
	grid.paint = voe_base_arena_push(arena, 8 * 25);
	for (size_t p = 0; p < 25; p++)
		set_weight(&grid, p, 0, 255);
	grid.looks[0] = (voe_assets_landscape_look){ .size = 2000,
						     .size_vary = 35,
						     .turn_vary = 0,
						     .tilt = 100 };
	grid.looks[1].shadowless = true;
	grid.grows[0][1] = 1;
	grid.grows[1][0] = 40;
	grid.grows[1][1] = VOE_ASSETS_LANDSCAPE_GROW_MAX;
	return grid;
}

static void looks_and_grows_round_trip_byte_for_byte(voe_base_arena *arena)
{
	const voe_assets_landscape grid = grown(arena);
	const voe_assets_landscape_text text =
		voe_assets_landscape_write(&grid, arena);
	voe_assets_landscape_text again;
	voe_assets_landscape back;

	VOE_TEST_CHECK(strstr(text.text, "layer1=\"Assets/rock.material\"\n"
					 "grow0_1=1\ngrow1_0=40\ngrow1_1=500\n\n"
					 "[Foliage]\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "kind0=\"Assets/pine.glb\"\nsize0=2000\n"
					 "vary0=35\nturn0=0\ntilt0=100\nkind1=") !=
		       NULL);
	VOE_TEST_CHECK(strstr(text.text, "\\\"birch\\\".glb\"\nshadow1=0\n\n") !=
		       NULL);
	VOE_TEST_CHECK(strstr(text.text, "shadow0") == NULL);
	VOE_TEST_CHECK(strstr(text.text, "grow0_0") == NULL);

	VOE_TEST_CHECK(voe_assets_landscape_read(text.text, text.size, arena,
						 &back, NULL));
	VOE_TEST_CHECK(same_look(back.looks[0], grid.looks[0]));
	VOE_TEST_CHECK(same_look(back.looks[1], grid.looks[1]));
	VOE_TEST_CHECK(memcmp(back.grows, grid.grows, sizeof(back.grows)) == 0);
	again = voe_assets_landscape_write(&back, arena);
	VOE_TEST_CHECK_INT(again.size, text.size);
	VOE_TEST_CHECK(strcmp(again.text, text.text) == 0);
}

// Two layers then two kinds: grow keys, look keys and the heights go in.
static const char *const two_layers_two_kinds =
	"[Landscape]\nsize=64\ncells=4\n[Layers]\nlayer0=\"a.material\"\n"
	"layer1=\"b.material\"\n%s[Foliage]\nkind0=\"a.glb\"\nkind1=\"b.glb\"\n"
	"%s%s";

static void no_look_keys_read_as_the_default(voe_base_arena *arena)
{
	const voe_assets_landscape_look fallback =
		voe_assets_landscape_look_default();
	char text[1024];
	voe_assets_landscape out;
	voe_assets_landscape_text written;
	bool none_grown = true;

	VOE_TEST_CHECK_INT(fallback.size, 0);
	VOE_TEST_CHECK_INT(fallback.size_vary, 20);
	VOE_TEST_CHECK_INT(fallback.turn_vary, 180);
	VOE_TEST_CHECK_INT(fallback.tilt, 0);
	VOE_TEST_CHECK(!fallback.shadowless);
	snprintf(text, sizeof(text), two_layers_two_kinds, "", "",
		 four_flat_rows);
	VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
	VOE_TEST_CHECK(same_look(out.looks[0], fallback));
	VOE_TEST_CHECK(same_look(out.looks[1], fallback));
	for (uint32_t l = 0; l < VOE_ASSETS_LANDSCAPE_LAYERS; l++)
		for (uint32_t k = 0; k < VOE_ASSETS_LANDSCAPE_KINDS; k++)
			none_grown = none_grown && out.grows[l][k] == 0;
	VOE_TEST_CHECK(none_grown);
	written = voe_assets_landscape_write(&out, arena);
	VOE_TEST_CHECK(strstr(written.text, "kind1=\"b.glb\"\n\n[Heights]") !=
		       NULL);
	VOE_TEST_CHECK(strstr(written.text, "size0") == NULL &&
		       strstr(written.text, "vary") == NULL &&
		       strstr(written.text, "turn") == NULL &&
		       strstr(written.text, "tilt") == NULL &&
		       strstr(written.text, "shadow") == NULL &&
		       strstr(written.text, "grow") == NULL);
}

static void bad_looks_and_grows_are_malformed(voe_base_arena *arena)
{
	const char *const bad_looks[] = {
		"vary0=101\n", "vary0=1.5\n", "turn1=181\n", "turn0=-1\n",
		"tilt0=101\n", "shadow0=2\n", "shadow1=x\n",
	};
	const char *const bad_grows[] = {
		// A layer and a kind past their counts.
		"grow2_0=40\n", "grow0_2=40\n",
		// Densities out of range or not whole.
		"grow0_0=0\n", "grow0_0=501\n", "grow1_1=2.5\n",
		// Not shaped grow<l>_<k>.
		"grow1=40\n", "grow_0_0=40\n", "grow0_0x=40\n",
	};
	char text[1024];

	for (size_t i = 0; i < sizeof(bad_looks) / sizeof(bad_looks[0]); i++) {
		snprintf(text, sizeof(text), two_layers_two_kinds, "",
			 bad_looks[i], four_flat_rows);
		refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	}
	for (size_t i = 0; i < sizeof(bad_grows) / sizeof(bad_grows[0]); i++) {
		snprintf(text, sizeof(text), two_layers_two_kinds, bad_grows[i],
			 "", four_flat_rows);
		refused_as(arena, text, VOE_BASE_ERROR_MALFORMED);
	}
	// And the ranges' ends are read.
	snprintf(text, sizeof(text), two_layers_two_kinds,
		 "grow0_0=1\ngrow1_1=500\n",
		 "vary0=0\nturn0=180\ntilt0=100\nshadow0=1\nvary1=100\n"
		 "turn1=0\nshadow1=0\n",
		 four_flat_rows);
	{
		voe_assets_landscape out;

		VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
		VOE_TEST_CHECK_INT(out.grows[0][0], 1);
		VOE_TEST_CHECK_INT(out.grows[1][1], 500);
		VOE_TEST_CHECK(same_look(out.looks[0],
					 (voe_assets_landscape_look){
						 .turn_vary = 180, .tilt = 100 }));
		VOE_TEST_CHECK(same_look(out.looks[1],
					 (voe_assets_landscape_look){
						 .size_vary = 100,
						 .shadowless = true }));
	}
}

static void a_resample_keeps_looks_and_grows(voe_base_arena *arena)
{
	const voe_assets_landscape grid = grown(arena);
	const voe_assets_landscape eight =
		voe_assets_landscape_resample(&grid, 8, arena);

	VOE_TEST_CHECK(same_look(eight.looks[0], grid.looks[0]));
	VOE_TEST_CHECK(same_look(eight.looks[1], grid.looks[1]));
	VOE_TEST_CHECK(memcmp(eight.grows, grid.grows, sizeof(eight.grows)) ==
		       0);
	VOE_TEST_CHECK_INT(eight.grows[1][1], VOE_ASSETS_LANDSCAPE_GROW_MAX);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	flat_written_reads_back_the_same(arena);
	the_largest_grid_reads_back_the_same(arena);
	a_plane_resampled_up_and_down_is_the_same(arena);
	a_resample_keeps_corners_and_the_same_count_copies(arena);
	heights_round_to_the_millimetre(arena);
	a_short_row_is_malformed(arena);
	a_missing_row_is_malformed(arena);
	cells_not_a_multiple_of_four_is_unsupported(arena);
	a_size_out_of_range_is_unsupported(arena);
	layers_and_paint_round_trip_byte_for_byte(arena);
	layers_and_no_paint_read_as_all_layer_0(arena);
	bad_paint_and_layers_are_malformed(arena);
	a_resample_of_paint_sums_to_255_and_keeps_corners(arena);
	foliage_round_trips_byte_for_byte(arena);
	no_copies_read_as_none(arena);
	bad_copies_are_malformed(arena);
	a_resample_keeps_the_copies(arena);
	kind_sizes_round_trip_byte_for_byte(arena);
	a_bad_kind_size_is_malformed(arena);
	a_resample_keeps_the_kind_sizes(arena);
	a_flat_grids_kind_added_by_hand_is_1x(arena);
	looks_and_grows_round_trip_byte_for_byte(arena);
	no_look_keys_read_as_the_default(arena);
	bad_looks_and_grows_are_malformed(arena);
	a_resample_keeps_looks_and_grows(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
