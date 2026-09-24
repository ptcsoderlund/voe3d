// The Errors panel: what a failed build said, shown in the editor so a person
// reads the compiler's words without a terminal (ADR-0240, 0242 point 8). A
// title, the last lines of <project>/Build/build.log in a scroll area, and
// Close — an anchored panel over the dock below the bar, where preferences.h's
// own panel goes.
//
//     voe_editor_errors errors = { 0 };
//     voe_editor_errors_show(&errors, log);        // a refresh or Play failed
//     voe_editor_errors_draw(ui, &errors, top, size);
//     ... voe_ui_frame_end ...
//     if (voe_editor_errors_clicks_read(ui, &errors))
//             voe_editor_errors_hide(&errors);
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

#include <math/float2.h>

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
	voe_ui_node close_button;
} voe_editor_errors;

// Keeps the last lines of the file at `log` and shows the panel.
void voe_editor_errors_show(voe_editor_errors *errors, const char *log);
void voe_editor_errors_hide(voe_editor_errors *errors);

// Draws the panel filling top..size.y of `size`'s width, the area below the
// bar, over the dock, and records Close. Asserts when it is not showing.
void voe_editor_errors_draw(voe_ui_context *ui, voe_editor_errors *errors,
			    float top, voe_math_float2 size);

// Whether Close fired this frame. Called after voe_ui_frame_end and before
// the frame's arena is rewound — the one window in which a widget answers.
bool voe_editor_errors_clicks_read(const voe_ui_context *ui,
				   const voe_editor_errors *errors);
