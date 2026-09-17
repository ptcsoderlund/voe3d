// A notice's text: cleared, set from a format, or built from base/report.h's
// first kept error. See the header for what a caller has to do before asking
// for the last one.
#include "notice.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdarg.h>
#include <stdio.h>

void voe_editor_notice_clear(voe_editor_notice *notice)
{
	VOE_BASE_ASSERT(notice != NULL, "clearing no notice");

	notice->text[0] = '\0';
}

void voe_editor_notice_set(voe_editor_notice *notice, const char *format, ...)
{
	va_list args;

	VOE_BASE_ASSERT(notice != NULL, "setting no notice");
	VOE_BASE_ASSERT(format != NULL, "setting a notice with no format");

	va_start(args, format);
	vsnprintf(notice->text, sizeof notice->text, format, args);
	va_end(args);
}

void voe_editor_notice_from_report(voe_editor_notice *notice, const char *file)
{
	const char *reason;

	VOE_BASE_ASSERT(notice != NULL, "building no notice from a report");
	VOE_BASE_ASSERT(file != NULL, "building a notice with no file to name");

	reason = voe_base_report_error_first();
	if (reason != NULL)
		voe_editor_notice_set(notice, "%s: %s", file, reason);
	else
		voe_editor_notice_set(notice, "%s: could not be read", file);
}
