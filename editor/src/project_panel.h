// The editor's Project panel: the game's window (0291 point 4) — a Width and
// a Height number box showing the window it is handed, a Windowed /
// Fullscreen choice and Close — shown as an anchored panel over the dock,
// below the bar, exactly where preferences.h's and browser.h's go.
//
//     voe_editor_project_panel panel = { 0 };
//     voe_editor_project_panel_show(&panel);             // the bar's Project
//     voe_editor_project_panel_draw(ui, &panel, window, top, size);
//     ... voe_ui_frame_end ...
//     result = voe_editor_project_panel_clicks_read(ui, &panel);
//
// A CHANGE IS WRITTEN AT ONCE AND IS NOT UNDONE. The caller hands a changed
// window straight to project.h's voe_editor_project_window_set, which writes
// project.voe3d; it is a setting, not a scene edit, so it marks nothing unsaved
// and never enters the undo line (0291 point 4).
//
// THE SIZE IS IGNORED WHILE FULLSCREEN IS CHOSEN: the game takes the screen and
// its size (0291 point 2). The two boxes still show and change the size, which
// is kept for going back to a window.
//
// A typed or dragged number is rounded to a whole one and clamped to
// VOE_AUTHORING_PROJECT_WINDOW_MIN..MAX before it is handed back, so a result's
// window is always one voe_editor_project_window_set takes.
//
// THIS FILE CARRIES OUT NO COMMAND OF ITS OWN, exactly as preferences.h does
// not: a `ui` widget answers only after voe_ui_frame_end, so the draw records
// each control and voe_editor_project_panel_clicks_read says what it did.
//
// Constraints. The panel has no scroll area: its five rows fit the smallest
// surface the dock allows. Drawing asserts when it is not showing.
#pragma once

#include <authoring/project.h>

#include <math/float2.h>

#include <ui/widgets.h>

#include <stdbool.h>

// Zeroed is a panel never shown.
typedef struct {
	bool showing;
	// The window the draw was handed, which a changed one starts from.
	voe_authoring_project_window shown;
	// The controls, recorded by the draw.
	voe_ui_node width_box;
	voe_ui_node height_box;
	voe_ui_node windowed_choice;
	voe_ui_node fullscreen_choice;
	voe_ui_node close_button;
	// The two boxes' text, `%d`, kept here because a label's text is drawn
	// after the call that made it (ui/widgets.h).
	char width_text[16];
	char height_text[16];
} voe_editor_project_panel;

void voe_editor_project_panel_show(voe_editor_project_panel *panel);
void voe_editor_project_panel_hide(voe_editor_project_panel *panel);

// Draws the panel filling top..size.y of `size`'s width, the area below the
// bar, over the dock: Width and Height rows, the choice, then Close, showing
// `window`. Records every control into panel. Asserts when it is not showing.
void voe_editor_project_panel_draw(voe_ui_context *ui,
				   voe_editor_project_panel *panel,
				   voe_authoring_project_window window,
				   float top, voe_math_float2 size);

// What this frame did: Close fired, and whether a number was committed or the
// other choice picked, with the window that makes.
typedef struct {
	bool closed;
	bool changed;
	voe_authoring_project_window window;
} voe_editor_project_panel_result;

// Called after voe_ui_frame_end and before the frame's arena is rewound — the
// one window in which a widget answers.
voe_editor_project_panel_result
voe_editor_project_panel_clicks_read(const voe_ui_context *ui,
				     const voe_editor_project_panel *panel);
