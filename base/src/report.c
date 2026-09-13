// The one function behind both report macros, and the composition it writes.
// See include/base/report.h for what a report is for and how to write one.
//
// The line is composed whole before anything is written, and then written in one
// call. The one write is what keeps a line whole when two threads report at once;
// the composed line is what makes sending it somewhere other than stderr a change
// inside voe_base_report_at and nowhere else.
#include "report_line.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

static const char *level_word(voe_base_report_level level)
{
	switch (level) {
	case VOE_BASE_LEVEL_WARNING:
		return "warning";
	case VOE_BASE_LEVEL_ERROR:
		return "error";
	}

	VOE_BASE_ASSERT(false, "a report level outside the enum");
	return "";
}

size_t voe_base_report_compose(char *line, voe_base_report_level level,
			       const char *module, const char *format,
			       va_list arguments)
{
	// Room for the text; the newline and the NUL come after it.
	const size_t text = VOE_BASE_REPORT_LINE_CAPACITY - 1;

	VOE_BASE_ASSERT(line != NULL, "a report composes into a buffer");
	VOE_BASE_ASSERT(module != NULL, "a report names its module");
	VOE_BASE_ASSERT(format != NULL, "a report has a message");

	int written = snprintf(line, text + 1, "%s: %s: ", level_word(level),
			       module);
	VOE_BASE_ASSERT(written >= 0, "the report prefix did not format");
	size_t length = (size_t)written;

	// At exactly the capacity the message is still formatted, for its length:
	// a prefix that fills the line must not hide that a message was cut.
	if (length <= text) {
		written = vsnprintf(line + length, text + 1 - length, format,
				    arguments);
		VOE_BASE_ASSERT(written >= 0, "the report message did not format");
		length += (size_t)written;
	}

	// Cut rather than grown, and marked so a reader can see it was cut.
	if (length > text) {
		length = text;
		memcpy(line + length - 3, "...", 3);
	}

	line[length] = '\n';
	line[length + 1] = '\0';
	return length + 1;
}

// file and line are not written: see the header for why they are passed at all.
void voe_base_report_at(voe_base_report_level level, const char *module,
			const char *file, int line, const char *format, ...)
{
	char composed[VOE_BASE_REPORT_LINE_CAPACITY + 1];
	va_list arguments;

	(void)file;
	(void)line;

	va_start(arguments, format);
	size_t length = voe_base_report_compose(composed, level, module, format,
						arguments);
	va_end(arguments);

	fwrite(composed, 1, length, stderr);
	fflush(stderr);
}
