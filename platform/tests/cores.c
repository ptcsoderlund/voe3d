// What platform/cores.h promises, checked from outside: the count is at least
// 1, no more than 4096, and two asks in a row agree. Needs no window and no
// display, and includes no OS header, so it builds on Windows too.
//
// 4096 IS A SANITY BOUND, NOT A LIMIT OF THE ENGINE'S. It is the largest core
// count Linux is configured for on common distributions; an answer above it is
// a garbage number, not a big machine. Two asks agreeing assumes no core goes
// offline between them, which nothing in a test run does.
#include <platform/cores.h>

#include <testing/test.h>

int main(void)
{
	uint32_t first = voe_platform_cores();
	uint32_t second = voe_platform_cores();

	VOE_TEST_CHECK(first >= 1);
	VOE_TEST_CHECK(first <= 4096);
	VOE_TEST_CHECK(first == second);
	return voe_test_result();
}
