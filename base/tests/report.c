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
// It checks ADR-0445's subjects: none leaves the line as it was, pushed ones
// sit between module and message in the line and the kept error, and a pop
// drops the last.
//
// It also checks ADR-0160's first-kept-error, through the real VOE_BASE_ERROR
// and VOE_BASE_WARNING macros rather than voe_base_report_compose(), because
// those macros are what feeds voe_base_report_error_first(). Each call still
// prints to stderr, same as every other test that exercises a refusal; nothing
// here captures it.
//
// And it checks ADR-0448's carried state (report_state.h): a state saved on
// one thread and loaded on another brings the subjects into that thread's lines
// and kept error, and its kept text until a clear; an empty state loaded over a
// thread with both leaves it with neither. The second thread's checks count on
// the shared counter safely because main joins it before reading anything.
#include "../src/report_line.h"
#include "../src/report_state.h"

#include <testing/test.h>

#include <stdarg.h>
#include <string.h>
#include <threads.h>

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

// Runs on a thread of its own, which starts with no subjects and nothing kept,
// and loads the state main saved: two subjects and a kept error.
static int run_under_loaded_state(void *state)
{
	static char line[VOE_BASE_REPORT_LINE_CAPACITY + 1];

	VOE_TEST_CHECK(voe_base_report_error_first() == NULL);
	voe_base_report_state_load(state);

	// The loaded kept text is this thread's, and a later error does not
	// replace it.
	const char *kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL &&
		       strcmp(kept, "a.glb: the part `x`: saved") == 0);
	VOE_BASE_ERROR("m", "there");
	kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL &&
		       strcmp(kept, "a.glb: the part `x`: saved") == 0);

	// Its line carries both subjects before the message.
	compose(line, VOE_BASE_LEVEL_ERROR, "m", "message");
	VOE_TEST_CHECK(strcmp(line, "error: m: a.glb: the part `x`: message\n") ==
		       0);

	// After a clear, an error reported here keeps both subjects too.
	voe_base_report_error_clear();
	VOE_TEST_CHECK(voe_base_report_error_first() == NULL);
	VOE_BASE_ERROR("m", "there");
	kept = voe_base_report_error_first();
	VOE_TEST_CHECK(kept != NULL &&
		       strcmp(kept, "a.glb: the part `x`: there") == 0);
	return 0;
}

// ADR-0448: the state crosses to another thread, and an empty one clears.
static void check_state_crosses_threads(void)
{
	static struct voe_base_report_state empty;
	static struct voe_base_report_state full;
	static char line[VOE_BASE_REPORT_LINE_CAPACITY + 1];
	thrd_t thread;
	int thread_result = -1;

	voe_base_report_error_clear();
	voe_base_report_state_save(&empty);
	VOE_TEST_CHECK_INT((long long)empty.subject_count, 0);
	VOE_TEST_CHECK(!empty.first_error_kept);

	voe_base_report_subject_push("a.glb");
	voe_base_report_subject_push("the part `x`");
	VOE_BASE_ERROR("m", "saved");
	voe_base_report_state_save(&full);

	VOE_TEST_CHECK(thrd_create(&thread, run_under_loaded_state, &full) ==
		       thrd_success);
	VOE_TEST_CHECK(thrd_join(thread, &thread_result) == thrd_success);
	VOE_TEST_CHECK_INT(thread_result, 0);

	// Main still has both; the empty state leaves it with neither.
	voe_base_report_state_load(&empty);
	VOE_TEST_CHECK(voe_base_report_error_first() == NULL);
	compose(line, VOE_BASE_LEVEL_ERROR, "m", "message");
	VOE_TEST_CHECK(strcmp(line, "error: m: message\n") == 0);
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

	// ADR-0445: with no subject pushed the line is exactly as before.
	compose(line, VOE_BASE_LEVEL_ERROR, "m", "message");
	VOE_TEST_CHECK(strcmp(line, "error: m: message\n") == 0);

	// Two subjects, in the order pushed, between the module and the message.
	voe_base_report_subject_push("a.glb");
	voe_base_report_subject_push("the part `x`");
	compose(line, VOE_BASE_LEVEL_ERROR, "m", "message");
	VOE_TEST_CHECK(strcmp(line, "error: m: a.glb: the part `x`: message\n") ==
		       0);

	// The kept first error carries them before its message.
	voe_base_report_error_clear();
	VOE_BASE_ERROR("m", "message");
	VOE_TEST_CHECK(strcmp(voe_base_report_error_first(),
			      "a.glb: the part `x`: message") == 0);

	// A line past the capacity is still cut and marked with subjects pushed.
	length = compose(line, VOE_BASE_LEVEL_ERROR, "m", "%s", message);
	VOE_TEST_CHECK_INT((long long)length, (long long)capacity);
	VOE_TEST_CHECK(strncmp(line, "error: m: a.glb: the part `x`: x", 32) == 0);
	VOE_TEST_CHECK(memcmp(line + capacity - 4, "...\n", 4) == 0);

	// After a pop, one subject.
	voe_base_report_subject_pop();
	compose(line, VOE_BASE_LEVEL_ERROR, "m", "message");
	VOE_TEST_CHECK(strcmp(line, "error: m: a.glb: message\n") == 0);

	// After the last pop, none again.
	voe_base_report_subject_pop();
	compose(line, VOE_BASE_LEVEL_ERROR, "m", "message");
	VOE_TEST_CHECK(strcmp(line, "error: m: message\n") == 0);

	check_state_crosses_threads();
	return voe_test_result();
}
