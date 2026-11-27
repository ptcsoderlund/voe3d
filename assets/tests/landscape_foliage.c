// The landscape file's foliage: kinds, their looks, each layer's grows and
// copies round-trip byte for byte, a resample keeps them, and each refusal
// comes with its category. The strict reader refuses a key or section the
// lenient one passes over, naming its line, and reads what the writer writes.
//
// The files refused are written by hand at four cells, the smallest a file may
// have, so each case shows the one thing wrong with it.
#include <assets/landscape.h>
#include <base/arena.h>
#include <base/report.h>

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

static void set_weight(voe_assets_landscape *landscape, size_t point,
		       uint32_t layer, uint8_t value)
{
	const size_t points = ((size_t)landscape->cells + 1) *
			      ((size_t)landscape->cells + 1);

	landscape->paint[(layer / 4) * points * 4 + point * 4 + layer % 4] =
		value;
}

static const char *const four_flat_rows =
	"[Heights]\nrow0=0 0 0 0 0\nrow1=0 0 0 0 0\nrow2=0 0 0 0 0\n"
	"row3=0 0 0 0 0\nrow4=0 0 0 0 0\n";

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

static void strict_refuses_an_unknown_key(voe_base_arena *arena)
{
	// What goes before the heights and after them, each with one key or
	// section the lenient reader passes over.
	static const struct {
		const char *before, *after;
	} unknown[] = {
		{ "sea=0\n", "" },
		{ "", "row5=0 0 0 0 0\n" },
		{ "", "[Weather]\n" },
		{ "[Layers]\nlayer0=\"a.material\"\nlayer2=\"c.material\"\n", "" },
		{ "[Foliage]\nkind0=\"a.glb\"\nglow0=1\n", "" },
		{ "[Foliage]\nkind0=\"a.glb\"\nsize1=500\n", "" },
		{ "[Layers]\nlayer0=\"a.material\"\n", "[Paint]\npaint1_0=0*5\n" },
		{ "", "[Paint]\npaint1_0=0*5\n" },
	};
	char text[512];
	const char *kept;

	for (size_t i = 0; i < sizeof(unknown) / sizeof(unknown[0]); i++) {
		voe_assets_landscape out = { .size = 1.0f };
		voe_base_error error = VOE_BASE_OK;

		snprintf(text, sizeof(text),
			 "[Landscape]\nsize=64\ncells=4\n%s%s%s",
			 unknown[i].before, four_flat_rows, unknown[i].after);
		VOE_TEST_CHECK(read_text(arena, text, &out, NULL));
		out = (voe_assets_landscape){ .size = 1.0f };
		voe_base_report_error_clear();
		VOE_TEST_CHECK(!voe_assets_landscape_read_strict(
			text, strlen(text), arena, &out, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
		VOE_TEST_CHECK(out.size == 1.0f && out.heights == NULL);
		kept = voe_base_report_error_first();
		VOE_TEST_CHECK(kept != NULL && strstr(kept, "line ") != NULL);
		if (i == 0)
			VOE_TEST_CHECK(strstr(kept, "line 4: sea: ") != NULL);
	}
	// What the lenient reader refuses, the strict one does too.
	snprintf(text, sizeof(text), "%s%s[Copies]\nc0=2 0 0 0 1000 0\n",
		 two_kinds, four_flat_rows);
	VOE_TEST_CHECK(!voe_assets_landscape_read_strict(text, strlen(text),
							 arena, &(voe_assets_landscape){ 0 },
							 NULL));
}

static void strict_reads_what_write_writes(voe_base_arena *arena)
{
	voe_assets_landscape grid = grown(arena);
	voe_assets_landscape_text text;
	voe_assets_landscape_text again;
	voe_assets_landscape back;

	grid.looks[1].blends = true;
	grid.looks[1].blend_height = 120;
	set_weight(&grid, 7, 1, 100);
	set_weight(&grid, 7, 0, 155);
	text = voe_assets_landscape_write(&grid, arena);
	VOE_TEST_CHECK(strstr(text.text, "\nblendheight1=120\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "\n[Paint]\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "\n[Copies]\n") != NULL);
	VOE_TEST_CHECK(voe_assets_landscape_read_strict(text.text, text.size,
							arena, &back, NULL));
	again = voe_assets_landscape_write(&back, arena);
	VOE_TEST_CHECK_INT(again.size, text.size);
	VOE_TEST_CHECK(strcmp(again.text, text.text) == 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

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
	strict_refuses_an_unknown_key(arena);
	strict_reads_what_write_writes(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
