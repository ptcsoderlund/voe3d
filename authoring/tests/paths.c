// authoring/paths.h: a renamed file and a moved folder followed in scene text,
// a longer name sharing the start left alone, strings inside an array and a
// kept section reached, the longest rewritten value reported, an open string
// refused, and the named check finding a user while ignoring keys and section
// names.
//
// The open-string refusal prints a line to stderr; that is the report doing
// its job, not a failure.
#include <authoring/paths.h>

#include <base/arena.h>
#include <base/report.h>

#include <testing/test.h>

#include <stddef.h>
#include <string.h>

// Follows `from` to `to` in `text`, which must succeed, and checks the result
// is `expected` with `changed` values rewritten.
static voe_authoring_paths_followed follow_ok(const char *text,
					      const char *from, const char *to,
					      const char *expected,
					      size_t changed)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_authoring_paths_followed out = { 0 };

	voe_base_report_error_clear();
	VOE_TEST_CHECK(voe_authoring_paths_follow(text, strlen(text), from, to,
						  arena, &out));
	VOE_TEST_CHECK(out.text.size == strlen(expected));
	VOE_TEST_CHECK(memcmp(out.text.text, expected, out.text.size) == 0);
	VOE_TEST_CHECK(out.text.text[out.text.size] == '\0');
	VOE_TEST_CHECK(out.changed == changed);
	voe_base_arena_destroy(arena);
	out.text = (voe_authoring_text){ 0 };
	return out;
}

static void follow_renames_a_file(void)
{
	follow_ok("[1]\nname = \"Rock\"\n[1.voe_model]\n"
		  "path = \"Assets/Rock.glb\"\n",
		  "Assets/Rock.glb", "Assets/Stone.glb",
		  "[1]\nname = \"Rock\"\n[1.voe_model]\n"
		  "path = \"Assets/Stone.glb\"\n", 1);
}

static void follow_moves_a_folders_contents(void)
{
	follow_ok("[1.voe_model]\npath = \"Assets/Rocks/a.glb\"\n"
		  "[2.voe_model]\npath=\"Assets/Rocks/deep/b.glb\"\n"
		  "[3.voe_model]\npath = \"Assets/Rocks\"\n"
		  "[4.voe_model]\npath = \"Assets/Other/a.glb\"",
		  "Assets/Rocks", "Assets/Nature/Stones",
		  "[1.voe_model]\npath = \"Assets/Nature/Stones/a.glb\"\n"
		  "[2.voe_model]\npath=\"Assets/Nature/Stones/deep/b.glb\"\n"
		  "[3.voe_model]\npath = \"Assets/Nature/Stones\"\n"
		  "[4.voe_model]\npath = \"Assets/Other/a.glb\"", 3);
}

static void follow_leaves_a_longer_name_alone(void)
{
	const char *text = "[1.voe_model]\npath = \"Assets/Rocks.glb\"\n"
			   "other = \"Assets/Rock2/a.glb\"\n";

	follow_ok(text, "Assets/Rock", "Assets/Stone", text, 0);
}

static void follow_reaches_an_array_and_a_kept_section(void)
{
	follow_ok("[1.game_spawner]\n"
		  "models = [\"Assets/A.glb\", \"x\\\"y\", \"Assets/A.glb\"]\n"
		  "count = 2\n"
		  "[1.not_registered_here]\nsound = \"Assets/A.glb\"\n",
		  "Assets/A.glb", "Assets/B.glb",
		  "[1.game_spawner]\n"
		  "models = [\"Assets/B.glb\", \"x\\\"y\", \"Assets/B.glb\"]\n"
		  "count = 2\n"
		  "[1.not_registered_here]\nsound = \"Assets/B.glb\"\n", 3);
}

static void follow_reports_the_longest(void)
{
	// "Assets/New/a\"b.glb" unescaped is 18 bytes; the escape stays.
	voe_authoring_paths_followed out =
		follow_ok("a = \"Assets/Old/x.glb\"\n"
			  "b = \"Assets/Old/a\\\"b.glb\"\n",
			  "Assets/Old", "Assets/New",
			  "a = \"Assets/New/x.glb\"\n"
			  "b = \"Assets/New/a\\\"b.glb\"\n", 2);

	VOE_TEST_CHECK(out.longest == strlen("Assets/New/a\"b.glb"));

	out = follow_ok("a = \"b\"\n", "Assets/Old", "Assets/New",
			"a = \"b\"\n", 0);
	VOE_TEST_CHECK(out.longest == 0);
}

static void follow_refuses_an_open_string(void)
{
	const char *text = "[1]\nname = \"Rock\npath = \"Assets/A.glb\"\n";
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_authoring_paths_followed out = { .changed = 7 };

	voe_base_report_error_clear();
	VOE_TEST_CHECK(!voe_authoring_paths_follow(text, strlen(text),
						   "Assets/A.glb", "Assets/B",
						   arena, &out));
	VOE_TEST_CHECK(out.changed == 7 && out.text.text == NULL);

	const char *first = voe_base_report_error_first();

	VOE_TEST_CHECK(first != NULL && strstr(first, "line 2") != NULL);
	voe_base_arena_destroy(arena);
}

static void named_finds_a_user(void)
{
	const char *text = "[1.voe_model]\npath = \"Assets/Rocks/a.glb\"\n"
			   "tags = [\"x\", \"Assets/Tree.glb\"]\n";

	VOE_TEST_CHECK(voe_authoring_paths_named(text, strlen(text),
						 "Assets/Rocks"));
	VOE_TEST_CHECK(voe_authoring_paths_named(text, strlen(text),
						 "Assets/Tree.glb"));
	VOE_TEST_CHECK(!voe_authoring_paths_named(text, strlen(text),
						  "Assets/Rock"));
	VOE_TEST_CHECK(!voe_authoring_paths_named(text, strlen(text),
						  "Assets/Rocks/a"));
}

static void named_ignores_keys_and_names(void)
{
	const char *text = "[Assets/A]\n"
			   "// \"Assets/A\"\n"
			   "Assets/A = 1\n"
			   "unquoted = Assets/A\n"
			   "note = x \"Assets/A\"\n";

	VOE_TEST_CHECK(!voe_authoring_paths_named(text, strlen(text),
						  "Assets/A"));
}

int main(void)
{
	follow_renames_a_file();
	follow_moves_a_folders_contents();
	follow_leaves_a_longer_name_alone();
	follow_reaches_an_array_and_a_kept_section();
	follow_reports_the_longest();
	follow_refuses_an_open_string();
	named_finds_a_user();
	named_ignores_keys_and_names();
	return voe_test_result();
}
