// The check macros a test program uses. A test is an ordinary C program:
//
//     #include <testing/test.h>
//
//     int main(void)
//     {
//	       VOE_TEST_CHECK(2 + 2 == 4);
//	       return voe_test_result();
//     }
//
// One file per module at <folder>/tests/<module>.c, one executable per file, and
// zero means pass. Nothing is registered anywhere: the build finds the file, so
// writing a test is writing a file and changing nothing else.
//
// A check never stops the program. Every check in a run reports, so one run
// tells you everything that is broken rather than only the first thing.
// voe_test_result() is the count of failures and belongs in the return of main.
//
// The failure message is the point of this header. Whoever reads it — a person
// or an agent — reads it and nothing else, and has to be able to fix the code
// without opening the test. So a failure names the file, the line, the
// expression as written, and for a comparison both of the values that did not
// match. It goes to stderr, which is what ctest --output-on-failure shows.
//
// Header-only, and it stays that way: there is no testing library to build and
// no link step to get wrong. The state is one counter per test program, which is
// sound because a test program is one translation unit with one main().
//
// Only tests/ may include this header. A folder's src/ may not, and check.cmake
// enforces that rather than trusting it.
//
// There are three checks on purpose. Add a fourth when something needs a fourth.
#pragma once

#include <stdio.h>

// The failure counter for this test program. A function-local static keeps it
// out of the global namespace and out of every warning about unused file-scope
// state in a translation unit that happens not to check anything.
static inline int *voe_test_failures(void)
{
	static int failures;

	return &failures;
}

static inline void voe_test_report(const char *file, int line, const char *expr)
{
	fprintf(stderr, "FAIL  %s:%d\n      %s\n", file, line, expr);
	(*voe_test_failures())++;
}

static inline void voe_test_check(int passed, const char *file, int line,
				  const char *expr)
{
	if (!passed)
		voe_test_report(file, line, expr);
}

static inline void voe_test_check_int(long long actual, long long expected,
				      const char *file, int line, const char *expr)
{
	if (actual == expected)
		return;

	voe_test_report(file, line, expr);
	fprintf(stderr, "      actual:   %lld\n      expected: %lld\n",
		actual, expected);
}

// The tolerance is a parameter because the caller knows what the number means
// and this header does not. No default is offered for the same reason.
static inline void voe_test_check_float(double actual, double expected,
					double tolerance, const char *file,
					int line, const char *expr)
{
	double difference = actual - expected;

	if (difference < 0.0)
		difference = -difference;
	if (difference <= tolerance)
		return;

	voe_test_report(file, line, expr);
	fprintf(stderr, "      actual:   %.9g\n      expected: %.9g\n"
			"      off by %.9g, tolerance %.9g\n",
		actual, expected, difference, tolerance);
}

static inline int voe_test_result(void)
{
	return *voe_test_failures();
}

#define VOE_TEST_CHECK(expr) \
	voe_test_check((expr) ? 1 : 0, __FILE__, __LINE__, #expr)

#define VOE_TEST_CHECK_INT(actual, expected) \
	voe_test_check_int((actual), (expected), __FILE__, __LINE__, \
			   #actual " == " #expected)

#define VOE_TEST_CHECK_FLOAT(actual, expected, tolerance) \
	voe_test_check_float((actual), (expected), (tolerance), __FILE__, \
			     __LINE__, #actual " == " #expected)
