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

// The first error's message, per thread, and whether one is kept. See
// include/base/report.h for what is kept and why.
static thread_local char g_first_error[VOE_BASE_REPORT_LINE_CAPACITY];
static thread_local bool g_first_error_kept;

void voe_base_report_error_clear(void)
{
	g_first_error_kept = false;
}

const char *voe_base_report_error_first(void)
{
	return g_first_error_kept ? g_first_error : NULL;
}

// Copies the "<message>" part out of a line already composed by
// voe_base_report_compose(): everything after the "<level>: <module>: "
// prefix and before the trailing newline. Reusing the composed line, rather
// than formatting the message a second time, is what keeps the kept message
// cut at exactly the same capacity as the printed line.
static void keep_first_error(voe_base_report_level level, const char *module,
			     const char *composed, size_t length)
{
	size_t prefix = strlen(level_word(level)) + strlen(": ") +
			strlen(module) + strlen(": ");

	// length counts the trailing newline; the pathological case of a
	// module name alone filling the line is clamped rather than read past.
	if (prefix > length - 1)
		prefix = length - 1;

	size_t message_length = length - 1 - prefix;
	memcpy(g_first_error, composed + prefix, message_length);
	g_first_error[message_length] = '\0';
	g_first_error_kept = true;
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

	if (level == VOE_BASE_LEVEL_ERROR && !g_first_error_kept)
		keep_first_error(level, module, composed, length);

	fwrite(composed, 1, length, stderr);
	fflush(stderr);
}
