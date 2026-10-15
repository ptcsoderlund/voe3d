// The parts file: three entries with the middle one empty and levels 2,
// STRONG round-trip through write and read and are written again byte for
// byte; an empty `[Parts]` reads as count 0; no `[Levels]` reads the default
// levels, and one with only `strength="gentle"` reads count 4, GENTLE; and a
// key past material31, a key that is not `material<N>`, an over-long path, no
// `[Parts]`, text that is not sectioned, `count=0`, `count=5`,
// `strength="hard"`, `colour=1` in `[Levels]` and another section are refused
// MALFORMED with `out` left as it was.
#include <assets/parts.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

static bool read_text(const char *text, voe_assets_parts *out,
		      voe_base_error *error)
{
	return voe_assets_parts_read(text, strlen(text), out, error);
}

// Refused MALFORMED, and `out` left as it was.
static void malformed(const char *text)
{
	voe_assets_parts out = { .count = 7 };
	voe_base_error error = VOE_BASE_OK;

	snprintf(out.materials[0], sizeof out.materials[0], "%s", "kept");
	VOE_TEST_CHECK(!read_text(text, &out, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
	VOE_TEST_CHECK(out.count == 7 && strcmp(out.materials[0], "kept") == 0);
}

static void three_entries_round_trip(voe_base_arena *arena)
{
	voe_assets_parts parts = {
		.count = 3,
		.levels = { 2, VOE_ASSETS_LEVELS_STRONG },
	};
	voe_assets_parts back = { 0 };
	voe_assets_material_text text;
	voe_assets_material_text again;

	snprintf(parts.materials[0], sizeof parts.materials[0], "%s",
		 "Assets/tree-bark.material");
	snprintf(parts.materials[2], sizeof parts.materials[2], "%s",
		 "Assets/\"odd\" \\ leaves.material");

	text = voe_assets_parts_write(&parts, arena);
	VOE_TEST_CHECK_INT(strlen(text.text), text.size);
	VOE_TEST_CHECK(strstr(text.text, "[Parts]\n") == text.text);
	VOE_TEST_CHECK(strstr(text.text, "\nmaterial1=\"\"\n") != NULL);
	VOE_TEST_CHECK(voe_assets_parts_read(text.text, text.size, &back, NULL));
	VOE_TEST_CHECK_INT(back.count, 3);
	VOE_TEST_CHECK_INT(back.levels.count, 2);
	VOE_TEST_CHECK_INT(back.levels.strength, VOE_ASSETS_LEVELS_STRONG);
	VOE_TEST_CHECK(strstr(text.text, "\n[Levels]\ncount=2\n"
					 "strength=\"strong\"\n") != NULL);
	for (uint32_t i = 0; i < 3; i++)
		VOE_TEST_CHECK(strcmp(back.materials[i], parts.materials[i]) ==
			       0);
	again = voe_assets_parts_write(&back, arena);
	VOE_TEST_CHECK_INT(again.size, text.size);
	VOE_TEST_CHECK(memcmp(again.text, text.text, text.size) == 0);

	// A key missing below the highest reads as "".
	VOE_TEST_CHECK(read_text("[Parts]\nmaterial31=\"a.material\"\n", &back,
				 NULL));
	VOE_TEST_CHECK_INT(back.count, 32);
	VOE_TEST_CHECK(back.materials[0][0] == '\0' &&
		       strcmp(back.materials[31], "a.material") == 0);
}

static void an_empty_section_reads_count_zero(voe_base_arena *arena)
{
	voe_assets_parts out = { .count = 5 };
	const voe_assets_parts none = { .levels = VOE_ASSETS_LEVELS_DEFAULT };
	const voe_assets_material_text text =
		voe_assets_parts_write(&none, arena);

	VOE_TEST_CHECK(read_text("[Parts]\n", &out, NULL));
	VOE_TEST_CHECK_INT(out.count, 0);
	VOE_TEST_CHECK(strcmp(text.text, "[Parts]\n\n[Levels]\ncount=4\n"
					 "strength=\"medium\"\n") == 0);
}

static void absent_levels_read_as_defaults(void)
{
	voe_assets_parts out = { 0 };

	VOE_TEST_CHECK(read_text("[Parts]\nmaterial0=\"\"\n", &out, NULL));
	VOE_TEST_CHECK_INT(out.levels.count, 4);
	VOE_TEST_CHECK_INT(out.levels.strength, VOE_ASSETS_LEVELS_MEDIUM);

	VOE_TEST_CHECK(read_text("[Parts]\n[Levels]\nstrength=\"gentle\"\n",
				 &out, NULL));
	VOE_TEST_CHECK_INT(out.levels.count, 4);
	VOE_TEST_CHECK_INT(out.levels.strength, VOE_ASSETS_LEVELS_GENTLE);
}

static void bad_keys_and_paths_are_malformed(void)
{
	char text[512];

	malformed("[Parts]\nmaterial32=\"a.material\"\n");
	malformed("[Parts]\ncolour=1 1 1\n");
	malformed("[Parts]\nmaterial=\"a.material\"\n");
	malformed("[Parts]\nmaterial01=\"a.material\"\n");
	malformed("[Parts]\nmaterial1x=\"a.material\"\n");
	malformed("[Other]\n");
	malformed("not sectioned\n");
	snprintf(text, sizeof(text), "[Parts]\nmaterial0=%0128d\n", 0);
	malformed(text);

	malformed("[Parts]\n[Levels]\ncount=0\n");
	malformed("[Parts]\n[Levels]\ncount=5\n");
	malformed("[Parts]\n[Levels]\nstrength=\"hard\"\n");
	malformed("[Parts]\n[Levels]\ncolour=1\n");
	malformed("[Parts]\n[Other]\n");
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);

	three_entries_round_trip(arena);
	an_empty_section_reads_count_zero(arena);
	absent_levels_read_as_defaults();
	bad_keys_and_paths_are_malformed();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
