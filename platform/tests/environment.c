// What platform/environment.h promises, checked from outside: a variable every
// system sets comes back with its value, and one no system sets comes back
// NULL. PATH is the one every test run has, on Linux and on Windows alike.
//
// NOTHING IS SET HERE: Windows has no setenv, and this test builds there too,
// so it reads only what the environment already holds. That leaves the empty
// value ("" and not NULL) unchecked.
#include <platform/environment.h>

#include <base/arena.h>
#include <testing/test.h>

static void test_path_is_set(voe_base_arena *arena)
{
	const char *path = voe_platform_environment_read("PATH", arena);

	VOE_TEST_CHECK(path != NULL && path[0] != '\0');
}

static void test_missing_is_null(voe_base_arena *arena)
{
	VOE_TEST_CHECK(voe_platform_environment_read(
			       "VOE_PLATFORM_NO_SUCH_VARIABLE", arena) == NULL);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(4096);

	test_path_is_set(arena);
	test_missing_is_null(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
