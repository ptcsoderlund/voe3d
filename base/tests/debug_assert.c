// What VOE_BASE_DEBUG_ASSERT promises under NDEBUG, checked from outside: that
// its expression is still compiled, so a static function named only there
// counts as used, and that the expression never runs, so the check costs
// nothing and its side effects never happen.
//
// THAT THIS FILE BUILT IS HALF THE TEST. `refuse_and_count` below is named only
// inside one debug assert. Under -Werror, a shape that hid the expression from
// the compiler's idea of "used" (sizeof did, on Clang: 052 bug 04) fails this
// build with an unused or unneeded declaration. The other half is the count at
// run time: zero calls, and no abort although the expression is false.
//
// NDEBUG IS DEFINED HERE, WHATEVER THE BUILD SAID. The suite and the folder
// check build Debug, so a test that followed the build would only ever see the
// Debug shape and never the one this file is for.
#undef NDEBUG
#define NDEBUG 1

#include <base/assert.h>

#include <stdbool.h>

#include <testing/test.h>

static int calls;

// False on purpose: were the expression ever run, the assert would also abort.
static bool refuse_and_count(void)
{
	calls++;
	return false;
}

int main(void)
{
	VOE_BASE_DEBUG_ASSERT(refuse_and_count(), "never runs under NDEBUG");

	VOE_TEST_CHECK_INT(calls, 0);

	return voe_test_result();
}
