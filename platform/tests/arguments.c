// What platform/arguments.h promises, checked from outside: a hand-made argv
// of three strings, one of them non-ASCII, comes back with count 3, the same
// bytes and a NULL after the last.
//
// LINUX ONLY BY CONSTRUCTION. Windows ignores the argv it is given and reads
// this process's own command line, so there the hand-made list is not what
// comes back; the check is skipped under _WIN32.
#include <platform/arguments.h>

#include <base/arena.h>
#include <testing/test.h>

#include <string.h>

static void test_argv_comes_back_as_it_is(void)
{
#ifndef _WIN32
	char program[] = "voe";
	char folder[] = "Åsa 李";
	char flag[] = "--open";
	char *argv[] = {program, folder, flag, NULL};
	voe_base_arena *arena = voe_base_arena_new(4096);
	voe_platform_arguments arguments = voe_platform_arguments_read(3, argv, arena);

	VOE_TEST_CHECK(arguments.count == 3);
	VOE_TEST_CHECK(strcmp(arguments.values[0], "voe") == 0);
	VOE_TEST_CHECK(strcmp(arguments.values[1], "Åsa 李") == 0);
	VOE_TEST_CHECK(strcmp(arguments.values[2], "--open") == 0);
	VOE_TEST_CHECK(arguments.values[3] == NULL);
	voe_base_arena_destroy(arena);
#endif
}

int main(void)
{
	test_argv_comes_back_as_it_is();
	return voe_test_result();
}
