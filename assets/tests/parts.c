// The parts file: three entries with the middle one empty and levels 2,
// shares 80, 40, 10, 5, 2.5 and swaps 3, 1, 0.3, 1, 1 round-trip through write
// and read and are written again byte for byte, with no `strength` line; an
// empty `[Parts]` reads as count 0 and writes the ten default level keys; no
// `[Levels]` reads the default levels, shares and swaps included; an old
// `strength` key, `"gentle"` or `"hard"`, reads as the defaults with no error,
// and what it read writes back with no `strength`; one with only
// `swap="farther"` reads swaps 4 on levels 1 to 5, and `count=6` reads count 6;
// the old swap word fills only the swaps whose own key is absent, in either key
// order; and a key past material31, a key that is not `material<N>`, an
// over-long path, no `[Parts]`, text that is not sectioned, `count=0`,
// `count=7`, `swap="soon"`, `colour=1` in `[Levels]`, another section, shares
// out of order or out of range, swaps out of range, `share6` and a swap that
// is not a number are refused MALFORMED with `out` left as it was.
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
		.levels = { .count = 2,
			    .shares = { 100, 80, 40, 10, 5, 2.5f },
			    .swaps = { 1, 3, 1, 0.3f, 1, 1 } },
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
	for (uint32_t i = 1; i < VOE_ASSETS_LEVELS; i++) {
		VOE_TEST_CHECK_FLOAT(back.levels.shares[i],
				     parts.levels.shares[i], 0.0f);
		VOE_TEST_CHECK_FLOAT(back.levels.swaps[i],
				     parts.levels.swaps[i], 0.0f);
	}
	VOE_TEST_CHECK(strstr(text.text,
			      "\n[Levels]\ncount=2\n"
			      "share1=80\nshare2=40\nshare3=10\nshare4=5\n"
			      "share5=2.5\nswap1=3\nswap2=1\nswap3=0.3\n"
			      "swap4=1\nswap5=1\n") != NULL);
	VOE_TEST_CHECK(strstr(text.text, "swap=") == NULL);
	VOE_TEST_CHECK(strstr(text.text, "strength") == NULL);
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
	VOE_TEST_CHECK(strcmp(text.text, "[Parts]\n\n[Levels]\ncount=6\n"
					 "share1=50\nshare2=25\nshare3=12\n"
					 "share4=6\nshare5=3\nswap1=1\n"
					 "swap2=1\nswap3=1\nswap4=1\n"
					 "swap5=1\n") == 0);
}

// The default levels: count 6, shares 100, 50, 25, 12, 6, 3 and swaps 1.
static void check_default_levels(const voe_assets_levels *levels)
{
	const float shares[VOE_ASSETS_LEVELS] = { 100, 50, 25, 12, 6, 3 };

	VOE_TEST_CHECK_INT(levels->count, 6);
	for (uint32_t i = 0; i < VOE_ASSETS_LEVELS; i++) {
		VOE_TEST_CHECK_FLOAT(levels->shares[i], shares[i], 0.0f);
		VOE_TEST_CHECK_FLOAT(levels->swaps[i], 1.0f, 0.0f);
	}
}

static void absent_levels_read_as_defaults(voe_base_arena *arena)
{
	const char *const strengths[] = {
		"[Parts]\n[Levels]\nstrength=\"gentle\"\n",
		"[Parts]\n[Levels]\nstrength=\"hard\"\n",
	};
	voe_assets_parts out = { 0 };

	VOE_TEST_CHECK(read_text("[Parts]\nmaterial0=\"\"\n", &out, NULL));
	check_default_levels(&out.levels);

	// An old strength key, whatever its value, is read and ignored.
	for (uint32_t t = 0; t < 2; t++) {
		voe_base_error error = VOE_BASE_OK;
		voe_assets_material_text text;

		VOE_TEST_CHECK(read_text(strengths[t], &out, &error));
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		check_default_levels(&out.levels);
		text = voe_assets_parts_write(&out, arena);
		VOE_TEST_CHECK(strstr(text.text, "strength") == NULL);
	}

	VOE_TEST_CHECK(read_text("[Parts]\n[Levels]\nswap=\"farther\"\n",
				 &out, NULL));
	VOE_TEST_CHECK_INT(out.levels.count, 6);
	VOE_TEST_CHECK_FLOAT(out.levels.swaps[0], 1.0f, 0.0f);
	for (uint32_t i = 1; i < VOE_ASSETS_LEVELS; i++)
		VOE_TEST_CHECK_FLOAT(out.levels.swaps[i], 4.0f, 0.0f);

	VOE_TEST_CHECK(read_text("[Parts]\n[Levels]\ncount=6\n", &out, NULL));
	VOE_TEST_CHECK_INT(out.levels.count, 6);
}

static void old_swap_word_fills_only_absent_swaps(void)
{
	const char *const texts[] = {
		"[Parts]\n[Levels]\nswap2=0.5\nswap=\"far\"\n",
		"[Parts]\n[Levels]\nswap=\"far\"\nswap2=0.5\n",
	};

	for (uint32_t t = 0; t < 2; t++) {
		voe_assets_parts out = { 0 };

		VOE_TEST_CHECK(read_text(texts[t], &out, NULL));
		for (uint32_t i = 1; i < VOE_ASSETS_LEVELS; i++)
			VOE_TEST_CHECK_FLOAT(out.levels.swaps[i],
					     i == 2 ? 0.5f : 2.0f, 0.0f);
	}
}

static void shares_out_of_order_are_malformed(void)
{
	malformed("[Parts]\n[Levels]\nshare2=60\n");
	malformed("[Parts]\n[Levels]\nshare1=100\n");
	malformed("[Parts]\n[Levels]\nshare3=0\n");
	malformed("[Parts]\n[Levels]\nswap1=0.05\n");
	malformed("[Parts]\n[Levels]\nswap1=11\n");
	malformed("[Parts]\n[Levels]\nshare6=1\n");
	malformed("[Parts]\n[Levels]\nswap1=x\n");
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
	malformed("[Parts]\n[Levels]\ncount=7\n");
	malformed("[Parts]\n[Levels]\nswap=\"soon\"\n");
	malformed("[Parts]\n[Levels]\ncolour=1\n");
	malformed("[Parts]\n[Other]\n");
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);

	three_entries_round_trip(arena);
	an_empty_section_reads_count_zero(arena);
	absent_levels_read_as_defaults(arena);
	old_swap_word_fills_only_absent_swaps();
	shares_out_of_order_are_malformed();
	bad_keys_and_paths_are_malformed();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
