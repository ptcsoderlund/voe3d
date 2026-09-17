// project.voe3d: the exact bytes the writer emits, a round trip back to the
// same scene, every refusal named in authoring/project.h with the line
// voe_base_report_error_first() keeps where the file has one, and an unknown
// key that still loads with a warning.
//
// A REFUSAL IS CHECKED BY THE CALL RETURNING FALSE, and voe_base_report_error_
// clear() runs before each one, as base/report.h says a reader must, so the
// error read back afterwards is this call's own and not one left over.
//
// Refusals and warnings print a line to stderr; that is the report doing its
// job, not a failure.
#include <authoring/project.h>

#include <base/arena.h>
#include <base/report.h>

#include <testing/test.h>

#include <stddef.h>
#include <string.h>

static void check_text(const char *actual, size_t size, const char *expected)
{
	VOE_TEST_CHECK(size == strlen(expected));
	VOE_TEST_CHECK(memcmp(actual, expected, size) == 0);
	VOE_TEST_CHECK(actual[size] == '\0');
}

static void test_write_exact_bytes(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_authoring_project project = { .scene = "main.scene" };
	size_t size;
	const char *text = voe_authoring_project_write(&project, arena, &size);

	check_text(text, size, "[project]\nscene = \"main.scene\"\n");
	voe_base_arena_destroy(arena);
}

static void test_write_escapes(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_authoring_project project = { .scene = "weird\"scene.scene" };
	size_t size;
	const char *text = voe_authoring_project_write(&project, arena, &size);

	check_text(text, size, "[project]\nscene = \"weird\\\"scene.scene\"\n");
	voe_base_arena_destroy(arena);
}

static void test_round_trip(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_authoring_project written = { .scene = "sub/main.scene" };
	size_t size;
	const char *text = voe_authoring_project_write(&written, arena, &size);

	voe_authoring_project read;

	voe_base_report_error_clear();
	VOE_TEST_CHECK(voe_authoring_project_read(text, size, arena, &read));
	VOE_TEST_CHECK(strcmp(read.scene, "sub/main.scene") == 0);
	voe_base_arena_destroy(arena);
}

static bool refused(const char *text, const char *line_prefix)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_authoring_project out;

	voe_base_report_error_clear();
	bool ok = voe_authoring_project_read(text, strlen(text), arena, &out);
	bool result = !ok;

	if (result && line_prefix != NULL) {
		const char *first = voe_base_report_error_first();

		result = first != NULL &&
			 strncmp(first, line_prefix, strlen(line_prefix)) == 0;
	}
	voe_base_arena_destroy(arena);
	return result;
}

static void test_refusals(void)
{
	// Malformed: the sectioned reader's own report, which is a line too.
	VOE_TEST_CHECK(refused("not a key line\n", "sectioned: line 1"));

	// A section other than [project].
	VOE_TEST_CHECK(
		refused("[other]\nscene = main.scene\n", "line 1"));

	// No [project] at all: nothing to name a line with.
	{
		voe_base_arena *arena = voe_base_arena_new(4096);
		voe_authoring_project out;

		voe_base_report_error_clear();
		VOE_TEST_CHECK(
			!voe_authoring_project_read("", 0, arena, &out));

		const char *first = voe_base_report_error_first();

		VOE_TEST_CHECK(first != NULL);
		VOE_TEST_CHECK(first == NULL ||
			       strncmp(first, "line ", 5) != 0);
		voe_base_arena_destroy(arena);
	}

	// No scene key: named by the [project] line.
	VOE_TEST_CHECK(refused("[project]\nother = 1\n", "line 1"));

	// A scene value that is empty, absolute, a drive letter, backslashed,
	// a control character, or an empty/./.. segment — each on its own
	// key's line.
	VOE_TEST_CHECK(refused("[project]\nscene = \"\"\n", "line 2"));
	VOE_TEST_CHECK(refused("[project]\nscene = /abs.scene\n", "line 2"));
	VOE_TEST_CHECK(refused("[project]\nscene = C:/x.scene\n", "line 2"));
	VOE_TEST_CHECK(refused("[project]\nscene = a\\b\n", "line 2"));
	VOE_TEST_CHECK(refused("[project]\nscene = a\x01" "b\n", "line 2"));
	VOE_TEST_CHECK(refused("[project]\nscene = a\x7f" "b\n", "line 2"));
	VOE_TEST_CHECK(refused("[project]\nscene = a//b\n", "line 2"));
	VOE_TEST_CHECK(refused("[project]\nscene = ./main.scene\n", "line 2"));
	VOE_TEST_CHECK(
		refused("[project]\nscene = ../main.scene\n", "line 2"));
	VOE_TEST_CHECK(refused("[project]\nscene = main.scene/\n", "line 2"));
}

static void test_unknown_key_warns_and_loads(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_authoring_project out;

	voe_base_report_error_clear();
	VOE_TEST_CHECK(voe_authoring_project_read(
		"[project]\nfuture = 1\nscene = main.scene\n",
		strlen("[project]\nfuture = 1\nscene = main.scene\n"), arena,
		&out));
	VOE_TEST_CHECK(strcmp(out.scene, "main.scene") == 0);
	voe_base_arena_destroy(arena);
}

int main(void)
{
	test_write_exact_bytes();
	test_write_escapes();
	test_round_trip();
	test_refusals();
	test_unknown_key_warns_and_loads();
	return voe_test_result();
}
