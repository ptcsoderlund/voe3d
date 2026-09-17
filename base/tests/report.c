// What base/report.h promises about the line, checked without capturing stderr:
// that each level writes its own word, that the line is exactly
// "<level>: <module>: <message>\n", and that the truncation boundary is where the
// header says it is — a line at the capacity arrives whole and one byte past it
// is cut and marked.
//
// THE BOUNDARY IS THE FAILURE THIS FILE IS REALLY FOR. An off-by-one there looks
// right on every message anyone writes by hand; it shows only on the one message
// long enough to matter, which is the one a person was trying to read.
//
// It also checks ADR-0160's first-kept-error, through the real VOE_BASE_ERROR
// and VOE_BASE_WARNING macros rather than voe_base_report_compose(), because
// those macros are what feeds voe_base_report_error_first(). Each call still
// prints to stderr, same as every other test that exercises a refusal; nothing
// here captures it.
#include "../src/report_line.h"

#include <testing/test.h>

#include <stdarg.h>
#include <string.h>

[[gnu::format(printf, 4, 5)]]
static size_t compose(char *line, voe_base_report_level level,
		      const char *module, const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	size_t length = voe_base_report_compose(line, level, module, format,
						arguments);
	va_end(arguments);
	return length;
}

int main(void)
{
	static char line[VOE_BASE_REPORT_LINE_CAPACITY + 1];
	static char message[VOE_BASE_REPORT_LINE_CAPACITY + 1];
	const size_t capacity = VOE_BASE_REPORT_LINE_CAPACITY;

	// Both levels, their own word, the module as its own field, one newline.
	size_t length = compose(line, VOE_BASE_LEVEL_WARNING, "demo",
				"something (%d)", 7);
	VOE_TEST_CHECK(strcmp(line, "warning: demo: something (7)\n") == 0);
	VOE_TEST_CHECK_INT((long long)length, (long long)strlen(line));

	length = compose(line, VOE_BASE_LEVEL_ERROR, "demo", "something (%d)", 7);
	VOE_TEST_CHECK(strcmp(line, "error: demo: something (7)\n") == 0);
	VOE_TEST_CHECK_INT((long long)length, (long long)strlen(line));

	// A message that makes the line exactly the capacity, newline included.
	// "error: m: " is ten bytes.
	const size_t prefix = strlen("error: m: ");
	size_t fill = capacity - 1 - prefix;

	memset(message, 'x', fill);
	message[fill] = '\0';
	length = compose(line, VOE_BASE_LEVEL_ERROR, "m", "%s", message);
	VOE_TEST_CHECK_INT((long long)length, (long long)capacity);
	VOE_TEST_CHECK(line[capacity - 1] == '\n');
	VOE_TEST_CHECK(line[capacity - 2] == 'x');
	VOE_TEST_CHECK(strstr(line, "...") == NULL);

	// One byte more: the same length, cut, and marked as cut.
	message[fill] = 'x';
	message[fill + 1] = '\0';
	length = compose(line, VOE_BASE_LEVEL_ERROR, "m", "%s", message);
	VOE_TEST_CHECK_INT((long long)length, (long long)capacity);
	VOE_TEST_CHECK_INT((long long)strlen(line), (long long)capacity);
	VOE_TEST_CHECK(line[capacity - 1] == '\n');
	VOE_TEST_CHECK(memcmp(line + capacity - 4, "...", 3) == 0);
	VOE_TEST_CHECK(line[capacity - 5] == 'x');

	// A prefix that alone fills the line still marks the message it hid.
	memset(message, 'm', capacity - 1 - strlen("error: : "));
	message[capacity - 1 - strlen("error: : ")] = '\0';
	length = compose(line, VOE_BASE_LEVEL_ERROR, message, "lost");
	VOE_TEST_CHECK_INT((long long)length, (long long)capacity);
	VOE_TEST_CHECK(memcmp(line + capacity - 4, "...", 3) == 0);

	// ADR-0160: nothing is kept before the first error, and a clear leaves
	// nothing kept.
	voe_base_report_error_clear();
	VOE_TEST_CHECK(voe_base_report_error_first() == NULL);

	// The first of two errors is what's kept, not the second.
	VOE_BASE_ERROR("demo", "first (%d)", 1);
	VOE_BASE_ERROR("demo", "second (%d)", 2);
	VOE_TEST_CHECK(strcmp(voe_base_report_error_first(), "first (1)") == 0);

	// A clear forgets it.
	voe_base_report_error_clear();
	VOE_TEST_CHECK(voe_base_report_error_first() == NULL);

	// A warning is never kept.
	VOE_BASE_WARNING("demo", "a warning");
	VOE_TEST_CHECK(voe_base_report_error_first() == NULL);

	// A long message is cut at the same capacity as the printed line.
	voe_base_report_error_clear();
	memset(message, 'x', fill + 1);
	message[fill + 1] = '\0';
	VOE_BASE_ERROR("m", "%s", message);
	const char *kept = voe_base_report_error_first();
	VOE_TEST_CHECK_INT((long long)strlen(kept), (long long)fill);
	VOE_TEST_CHECK(memcmp(kept + fill - 3, "...", 3) == 0);

	return voe_test_result();
}
