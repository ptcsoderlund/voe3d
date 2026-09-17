// One line the top bar has to say to a person: what a project or a save just
// refused, or why the editor fell back to an untitled scene. Nothing here
// decides when a notice is shown or cleared — that is task 12's session — this
// is only the text and how it is built.
//
// A FIXED BUFFER, BECAUSE A NOTICE OUTLIVES THE FAILURE THAT MADE IT. The
// message that explains why an open or a save was refused often comes from
// base/report.h's kept error, which is only valid until the next report; a
// notice copies it out so a bar can still show it several frames later.
//
// voe_editor_notice_from_report READS base/report.h's FIRST KEPT ERROR, NOT
// THE LAST REPORT MADE. A caller clears before the step whose failure it wants
// explained (base/report.h says so), and asks this to read it back only once
// that step has returned false — nothing here calls
// voe_base_report_error_clear() itself, because the caller is the one who
// knows which step it is explaining. `file` is named first because a notice
// that only repeats "line 4: ..." with no file to put it against is not
// something a person reading a top bar can act on.
#pragma once

// A message long enough for a path and one line of report text; longer is cut
// by voe_editor_notice_set, the same way base/report.h cuts its own line.
typedef struct {
	char text[512];
} voe_editor_notice;

// Empties notice: an empty top bar has nothing to show.
void voe_editor_notice_clear(voe_editor_notice *notice);

// Formats into notice, cut at its capacity, exactly as vsnprintf cuts any
// fixed buffer — there is no report to fall back on here, because the caller
// already has its own words for whatever it wants to say.
[[gnu::format(printf, 2, 3)]] void voe_editor_notice_set(voe_editor_notice *notice,
							 const char *format, ...);

// Writes "<file>: <voe_base_report_error_first()>", or "<file>: could not be
// read" when nothing has been kept since the caller's last
// voe_base_report_error_clear(). file is copied in, not kept by reference.
void voe_editor_notice_from_report(voe_editor_notice *notice, const char *file);
