// What platform/library.h promises about failure, checked from outside: a
// library no system has is a NULL open, and the report it leaves behind names
// the library, so a caller can show why a load failed.
//
// The failing open prints a reported line to stderr on purpose.
#include <platform/library.h>

#include <base/report.h>
#include <testing/test.h>

#include <string.h>

#define MISSING_LIBRARY "voe-no-such-library-anywhere.so"

static void test_missing_library_is_null_and_reported(void)
{
	const char *reported;

	voe_base_report_error_clear();
	VOE_TEST_CHECK(voe_platform_library_new(MISSING_LIBRARY) == NULL);
	reported = voe_base_report_error_first();
	VOE_TEST_CHECK(reported != NULL && strstr(reported, MISSING_LIBRARY) != NULL);
}

int main(void)
{
	test_missing_library_is_null_and_reported();
	return voe_test_result();
}
