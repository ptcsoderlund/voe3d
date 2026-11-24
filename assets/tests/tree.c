// The tree species file: the default written and read back equal field by
// field, by both readers; the lenient reader clamping an out-of-range value,
// defaulting a missing key and passing over an unknown one; the strict reader
// refusing an unknown key, an unknown section and an out-of-range value,
// naming each; `path_is` on species, variation and other paths; a variation
// path made or refused for its room; and one key table row per key. Leaf roll
// and width: absent, clamped, refused and round-tripped.
#include <assets/tree.h>
#include <base/arena.h>
#include <base/report.h>

#include <testing/test.h>

#include <string.h>

static bool same(const voe_assets_tree *a, const voe_assets_tree *b)
{
	return strcmp(a->bark, b->bark) == 0 && strcmp(a->leaf, b->leaf) == 0 &&
	       a->height == b->height && a->trunk == b->trunk &&
	       a->crown == b->crown && a->branching == b->branching &&
	       a->reach == b->reach && a->angle == b->angle &&
	       a->droop == b->droop && a->crooked == b->crooked &&
	       a->leaf_size == b->leaf_size &&
	       a->leaf_density == b->leaf_density &&
	       a->leaf_roll == b->leaf_roll && a->leaf_width == b->leaf_width &&
	       a->variations == b->variations;
}

static void the_default_round_trips(voe_base_arena *arena)
{
	voe_assets_tree tree = voe_assets_tree_default();
	voe_assets_tree back = { .height = 7.0f };
	voe_assets_tree_text text;

	text = voe_assets_tree_write(&tree, arena);
	VOE_TEST_CHECK_INT(strlen(text.text), text.size);
	VOE_TEST_CHECK(voe_assets_tree_read(text.text, text.size, &back, NULL));
	VOE_TEST_CHECK(same(&back, &tree));
	back = (voe_assets_tree){ .height = 7.0f };
	VOE_TEST_CHECK(voe_assets_tree_read_strict(text.text, text.size, &back,
						   NULL));
	VOE_TEST_CHECK(same(&back, &tree));

	// With paths, a quote and a backslash in one.
	strcpy(tree.bark, "Assets/pine bark.material");
	strcpy(tree.leaf, "Assets/\"odd\" \\ leaf.material");
	tree.droop = -0.75f;
	tree.variations = 16;
	text = voe_assets_tree_write(&tree, arena);
	VOE_TEST_CHECK(voe_assets_tree_read_strict(text.text, text.size, &back,
						   NULL));
	VOE_TEST_CHECK(same(&back, &tree));
}

static void lenient_clamps_and_defaults(void)
{
	const char *const text = "[Tree]\nheight=100\nangle=1\n"
				 "variations=40\nleaf_density=-3\nglow=1\n";
	const voe_assets_tree expected = voe_assets_tree_default();
	voe_assets_tree out;

	VOE_TEST_CHECK(voe_assets_tree_read(text, strlen(text), &out, NULL));
	VOE_TEST_CHECK(out.height == VOE_ASSETS_TREE_HEIGHT_MAX);
	VOE_TEST_CHECK(out.angle == VOE_ASSETS_TREE_ANGLE_MIN);
	VOE_TEST_CHECK_INT(out.variations, VOE_ASSETS_TREE_VARIATIONS_MAX);
	VOE_TEST_CHECK(out.leaf_density == VOE_ASSETS_TREE_LEAF_DENSITY_MIN);
	// Every key not given reads its default.
	VOE_TEST_CHECK(out.trunk == expected.trunk &&
		       out.crown == expected.crown &&
		       out.branching == expected.branching &&
		       out.reach == expected.reach &&
		       out.droop == expected.droop &&
		       out.crooked == expected.crooked &&
		       out.leaf_size == expected.leaf_size);
	VOE_TEST_CHECK(out.bark[0] == '\0' && out.leaf[0] == '\0');
	VOE_TEST_CHECK(voe_assets_tree_read("[Tree]\n", 7, &out, NULL));
	VOE_TEST_CHECK(same(&out, &expected));
	VOE_TEST_CHECK(voe_assets_tree_read("[Tree]\nvariations=0\n", 20, &out,
					    NULL));
	VOE_TEST_CHECK_INT(out.variations, VOE_ASSETS_TREE_VARIATIONS_MIN);
}

// Refused MALFORMED by `strict` or the lenient reader, `out` left as it was,
// and the first kept line holding `names`.
static void refused(const char *text, bool strict, const char *names)
{
	voe_assets_tree out = { .height = 7.0f };
	voe_base_error error = VOE_BASE_OK;
	const char *kept;
	bool read;

	voe_base_report_error_clear();
	read = strict ? voe_assets_tree_read_strict(text, strlen(text), &out,
						    &error) :
			voe_assets_tree_read(text, strlen(text), &out, &error);
	VOE_TEST_CHECK(!read);
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);
	VOE_TEST_CHECK(out.height == 7.0f && out.variations == 0);
	kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL && strstr(kept, names) != NULL);
}

static void strict_refuses(void)
{
	refused("[Tree]\nheight=12\nglow=1\n", true, "line 3: glow: ");
	refused("[Tree]\n[Leaves]\n", true, "line 2: Leaves: ");
	refused("[Tree]\nheight=61\n", true, "height: outside 1 to 60");
	refused("[Tree]\nangle=4.5\n", true, "angle: outside 5 to 120");
	refused("[Tree]\nvariations=17\n", true, "variations: outside 1 to 16");
	refused("[Tree]\nvariations=0\n", true, "variations: outside");
	// What both refuse.
	refused("[Tree]\nheight=tall\n", false, "height: not a number");
	refused("[Tree]\nvariations=2.5\n", false, "variations: not a whole");
	refused("[Leaves]\n", false, "[Tree]: no such section");
}

// A file without the keys reads roll 0 and width 1; the lenient reader clamps
// them, the strict one refuses them, and written values come back.
static void leaf_roll_and_width(voe_base_arena *arena)
{
	const char *const wide = "[Tree]\nleaf_roll=2\nleaf_width=0\n";
	voe_assets_tree tree = voe_assets_tree_default();
	voe_assets_tree back;
	voe_assets_tree_text text;

	VOE_TEST_CHECK(voe_assets_tree_read("[Tree]\nheight=12\n", 17, &back,
					    NULL));
	VOE_TEST_CHECK(back.leaf_roll == 0.0f && back.leaf_width == 1.0f);

	VOE_TEST_CHECK(voe_assets_tree_read(wide, strlen(wide), &back, NULL));
	VOE_TEST_CHECK(back.leaf_roll == 1.0f);
	VOE_TEST_CHECK(back.leaf_width == 0.1f);

	refused("[Tree]\nleaf_roll=2\n", true, "leaf_roll: outside 0 to 1");
	refused("[Tree]\nleaf_width=0\n", true, "leaf_width: outside 0.1 to 4");

	tree.leaf_roll = 0.5f;
	tree.leaf_width = 2.5f;
	text = voe_assets_tree_write(&tree, arena);
	VOE_TEST_CHECK(voe_assets_tree_read_strict(text.text, text.size, &back,
						   NULL));
	VOE_TEST_CHECK(back.leaf_roll == 0.5f && back.leaf_width == 2.5f);
	VOE_TEST_CHECK(same(&back, &tree));
}

static void paths_are_told(void)
{
	char out[16] = "kept";

	VOE_TEST_CHECK(voe_assets_tree_path_is("a.tree"));
	VOE_TEST_CHECK(voe_assets_tree_path_is("A.TREE"));
	VOE_TEST_CHECK(voe_assets_tree_path_is("a.tree#3"));
	VOE_TEST_CHECK(voe_assets_tree_path_is("Assets/pine.Tree#15"));
	VOE_TEST_CHECK(!voe_assets_tree_path_is("a.tree#"));
	VOE_TEST_CHECK(!voe_assets_tree_path_is("a.glb"));
	VOE_TEST_CHECK(!voe_assets_tree_path_is("a.tree.glb"));
	VOE_TEST_CHECK(!voe_assets_tree_path_is("a.tree#x"));

	VOE_TEST_CHECK(voe_assets_tree_variation_path(out, sizeof out,
						      "a.tree", 12));
	VOE_TEST_CHECK(strcmp(out, "a.tree#12") == 0);
	VOE_TEST_CHECK(voe_assets_tree_path_is(out));
	// "abcdefghij.tree#5" is 17 bytes with its NUL.
	VOE_TEST_CHECK(!voe_assets_tree_variation_path(out, sizeof out,
						       "abcdefghij.tree", 5));
	VOE_TEST_CHECK(strcmp(out, "a.tree#12") == 0);
}

static void one_row_per_key(void)
{
	static const char *const keys[] = {
		"bark",	   "leaf",    "height",	   "trunk",
		"crown",   "branching", "reach",   "angle",
		"droop",   "crooked", "leaf_size", "leaf_density",
		"leaf_roll", "leaf_width", "variations",
	};
	uint32_t count = 0;
	const voe_assets_documented_key *rows = voe_assets_tree_keys(&count);

	VOE_TEST_CHECK_INT(count, sizeof(keys) / sizeof(keys[0]));
	for (uint32_t i = 0; i < count && i < sizeof(keys) / sizeof(keys[0]);
	     i++) {
		VOE_TEST_CHECK(strcmp(rows[i].section, "Tree") == 0);
		VOE_TEST_CHECK(strcmp(rows[i].key, keys[i]) == 0);
		VOE_TEST_CHECK(rows[i].kind[0] != '\0' &&
			       rows[i].says[0] != '\0');
	}
	VOE_TEST_CHECK(rows[2].least == VOE_ASSETS_TREE_HEIGHT_MIN &&
		       rows[2].greatest == VOE_ASSETS_TREE_HEIGHT_MAX &&
		       rows[2].refused);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);

	the_default_round_trips(arena);
	lenient_clamps_and_defaults();
	strict_refuses();
	leaf_roll_and_width(arena);
	paths_are_told();
	one_row_per_key();

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
