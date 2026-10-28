// The Errors panel: what a failed build said, shown in the editor so a person
// reads the compiler's words without a terminal (ADR-0240, 0242 point 8). The
// last lines of <project>/Build/build.log — the contents of a dock panel, in
// the scroll area of a leaf or a floating box under the walk's header and its
// × (0439, 0491).
//
//     voe_editor_errors errors = { 0 };
//     voe_editor_errors_show(&errors, log);        // a refresh or Play failed
//     if (errors.showing) // inside the dock panel's scroll area
//             voe_editor_errors_contents_draw(ui, &errors);
//
// ONLY THE TAIL. A Ninja log ends at the step that failed, so the first error
// is usually within its last lines, and the whole log stays in Build/build.log
// for anything further up. The session (session.h) shows the panel and hides
// it; this file only reads the log and draws what it kept.
//
// Constraints. At most VOE_EDITOR_ERRORS_LINES lines, each cut at
// VOE_EDITOR_ERRORS_LINE_BYTES bytes, back to the start of a UTF-8 character;
// a carriage return is dropped and a tab drawn as a space. Those two numbers
// are what the interface's node and element budget counts (interface.h), so
// raising either, with that budget, is what would lift them. The show reads
// the whole log once, into a scratch arena of its own made and destroyed in
// the call; an unreadable log keeps one line saying so.
#pragma once

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// How many of the log's last lines one showing keeps.
#define VOE_EDITOR_ERRORS_LINES 48
// The most bytes of one line kept, not counting its NUL.
#define VOE_EDITOR_ERRORS_LINE_BYTES 160

// Zeroed is a panel never shown.
typedef struct {
	bool showing;
	uint32_t count;
	// Kept here because a label's text is drawn after the call that made
	// it (ui/widgets.h).
	char lines[VOE_EDITOR_ERRORS_LINES][VOE_EDITOR_ERRORS_LINE_BYTES + 1];
} voe_editor_errors;

// Keeps the last lines of the file at `log` and shows the panel.
void voe_editor_errors_show(voe_editor_errors *errors, const char *log);
void voe_editor_errors_hide(voe_editor_errors *errors);

// Draws the kept lines as labels into whatever the caller opened. Asserts
// when it is not showing.
void voe_editor_errors_contents_draw(voe_ui_context *ui,
				     const voe_editor_errors *errors);
