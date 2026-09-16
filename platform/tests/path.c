// What platform/path.h promises, checked from outside: joining puts exactly
// one separator between and none extra after a root, a parent ignores one
// trailing separator and is NULL at a root or for a bare name, a name is
// whatever follows the last separator and "" at a root, and resolving to an
// absolute path is idempotent and NULL for a path that is not there. Needs no
// window and no display, so it runs under ctest on a machine with neither.
//
// ORDINARY PATHS, ROOTS AND TRAILING SEPARATORS ARE CHECKED FOR THE PLATFORM
// THIS RUNS ON, GUARDED BY #ifdef _WIN32, because a root and a separator are
// spelled differently on each — "/" against "C:\" — exactly as
// platform/tests/folder.c guards its XDG_CONFIG_HOME checks to the platform
// that has the environment variable being checked. Task 4's Windows backend
// is written and unverified, as ADR-0130 allows.
//
// ABSOLUTE NEEDS NOTHING PLATFORM-SPECIFIC IN THE TEST ITSELF: "." always
// names the working directory, wherever ctest sets it, and a name this test
// makes up is never there on either platform.
#include <platform/path.h>

#include <base/arena.h>
#include <testing/test.h>

#include <string.h>

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_base_error error = VOE_BASE_OK;

#ifdef _WIN32
	VOE_TEST_CHECK(strcmp(voe_platform_path_join(arena, "C:\\Users", "abc"),
			      "C:\\Users\\abc") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_join(arena, "C:\\", "abc"),
			      "C:\\abc") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_join(arena, "C:\\Users\\",
						     "abc"),
			      "C:\\Users\\abc") == 0);

	VOE_TEST_CHECK(strcmp(voe_platform_path_parent(arena,
							"C:\\Users\\foo"),
			      "C:\\Users") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_parent(arena,
							"C:\\Users\\foo\\"),
			      "C:\\Users") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_parent(arena, "C:\\Users"),
			      "C:\\") == 0);
	VOE_TEST_CHECK(voe_platform_path_parent(arena, "C:\\") == NULL);
	VOE_TEST_CHECK(voe_platform_path_parent(arena, "abc") == NULL);

	VOE_TEST_CHECK(strcmp(voe_platform_path_name("C:\\Users\\foo"), "foo")
		      == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_name("C:\\Users\\foo\\"), "")
		      == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_name("C:\\"), "") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_name("abc"), "abc") == 0);
#else
	// Ordinary paths, and joining never adds a separator past one folder
	// already ends in.
	VOE_TEST_CHECK(strcmp(voe_platform_path_join(arena, "/tmp", "abc"),
			      "/tmp/abc") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_join(arena, "/", "abc"),
			      "/abc") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_join(arena, "/tmp/", "abc"),
			      "/tmp/abc") == 0);

	// A parent ignores one trailing separator, is the root for a path one
	// level under it, and NULL at the root itself and for a bare name.
	VOE_TEST_CHECK(strcmp(voe_platform_path_parent(arena, "/tmp/abc"),
			      "/tmp") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_parent(arena, "/tmp/abc/"),
			      "/tmp") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_parent(arena, "/tmp"), "/")
		      == 0);
	VOE_TEST_CHECK(voe_platform_path_parent(arena, "/") == NULL);
	VOE_TEST_CHECK(voe_platform_path_parent(arena, "abc") == NULL);

	// A name is whatever follows the last separator, a pointer into the
	// path itself, and "" at a root or a path ending in a separator.
	VOE_TEST_CHECK(strcmp(voe_platform_path_name("/tmp/abc"), "abc") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_name("/tmp/abc/"), "") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_name("/"), "") == 0);
	VOE_TEST_CHECK(strcmp(voe_platform_path_name("abc"), "abc") == 0);
#endif

	// Resolving "." twice is the same as resolving it once: the first
	// call's result is already absolute, so the second is a no-op.
	{
		const char *once = voe_platform_path_absolute(".", arena,
							       &error);

		VOE_TEST_CHECK(once != NULL);
		VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
		if (once != NULL) {
			const char *twice = voe_platform_path_absolute(
				once, arena, &error);

			VOE_TEST_CHECK(twice != NULL);
			VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
			if (twice != NULL)
				VOE_TEST_CHECK(strcmp(once, twice) == 0);
		}
	}

	// Nothing is at a made-up name, on either platform.
	error = VOE_BASE_OK;
	VOE_TEST_CHECK(voe_platform_path_absolute(
			       "voe_platform_path_test_missing_xyz", arena,
			       &error) == NULL);
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
