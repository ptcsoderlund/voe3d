// Composing a report's line, apart from writing it. Internal to base: split out
// of report.c only so that tests/report.c can check the line and its truncation
// without capturing stderr.
#pragma once

#include <base/report.h>

#include <stdarg.h>
#include <stddef.h>

// The longest line a report writes, its newline included. base/report.h states
// this number in prose; the two are changed together.
enum { VOE_BASE_REPORT_LINE_CAPACITY = 1024 };

// Writes "<level>: <module>: <message>\n" and a terminating NUL into `line`,
// which holds VOE_BASE_REPORT_LINE_CAPACITY + 1 bytes, and returns the length of
// the line without the NUL. A line that would be longer is cut at the capacity
// and its last three bytes before the newline become "...".
[[gnu::format(printf, 4, 0)]]
size_t voe_base_report_compose(char *line, voe_base_report_level level,
			       const char *module, const char *format,
			       va_list arguments);
