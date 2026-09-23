// The editor's Preferences: every theme themes.h lists, one row each with the
// theme's display name and a Choose button, the one in force marked, and a
// Close button — shown as an anchored panel over the dock, below the bar,
// exactly where browser.h's own panel goes.
//
//     voe_editor_preferences preferences = { 0 };
//     voe_editor_preferences_show(&preferences);           // the bar's button
//     voe_editor_preferences_draw(ui, &preferences, &themes, top, size);
//     ... voe_ui_frame_end ...
//     result = voe_editor_preferences_clicks_read(ui, &preferences);
//
// EACH ROW IS DRAWN IN ITS OWN THEME. The row is made between
// voe_ui_theme_push and voe_ui_theme_pop of that entry's palette, so a person
// sees what Choose would give them while the rest of the panel stays in the
// theme in force (ADR-0170's "the nearest one wins").
//
// THE THREE SLIDERS BELONG TO THE THEME IN FORCE, not to the row under the
// pointer: they show that theme's contrast, surface separation and text size,
// and moving one moves the whole editor as it is dragged, because the palette
// every panel here is drawn from is derived again where it stands (themes.h).
// The first two range over VOE_UI_THEME_SCALAR_MIN..MAX, the range the
// derivation clamps into, so text stays readable at either end; text size
// over VOE_EDITOR_TEXT_SCALE_MIN..MAX, shown as a whole percentage (`%.0f%%`
// of the scale times 100). Reset puts back all three — the theme's own, the
// ones its file was authored with (ADR-0197, ADR-0224).
//
// THIS FILE CARRIES OUT NO COMMAND OF ITS OWN, exactly as topbar.h's and
// browser.h's buttons do not: a `ui` widget answers only after
// voe_ui_frame_end, so the draw records each button and
// voe_editor_preferences_clicks_read says which fired. What Choose does — the
// theme put in force and remembered — is themes.h's, called by interface.c.
//
// Constraints. At most VOE_EDITOR_PREFERENCES_ROWS themes are listed; a
// themes folder holding more shows only the first that many, in list order.
// The ceiling is what keeps this panel inside the interface's node and element
// budget (interface.h); raising it, with that budget, is what would lift it.
// `themes` must outlive the frame it is drawn in, because every palette it
// pushes is kept by pointer until the frame ends.
#pragma once

#include "themes.h"

#include <math/float2.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// How many themes one showing lists — see the header's constraints.
#define VOE_EDITOR_PREFERENCES_ROWS 16

// Zeroed is a panel never shown.
typedef struct {
	bool showing;
	// One Choose button per listed theme, recorded by the draw.
	voe_ui_node choose_buttons[VOE_EDITOR_PREFERENCES_ROWS];
	uint32_t row_count;
	voe_ui_node close_button;
	// The theme in force's three scalars, and the button that puts the
	// theme's own three back.
	voe_ui_node contrast_slider;
	voe_ui_node separation_slider;
	voe_ui_node text_size_slider;
	voe_ui_node reset_button;
	// The value labels' text: `%.2f` of where contrast and separation
	// stand, and text size as a whole percentage.
	// Kept here because a label's text is drawn after the call that made
	// it, so it must outlive that call (ui/widgets.h).
	char contrast_text[16];
	char separation_text[16];
	char text_size_text[16];
} voe_editor_preferences;

void voe_editor_preferences_show(voe_editor_preferences *preferences);
void voe_editor_preferences_hide(voe_editor_preferences *preferences);

// Draws the panel filling top..size.y of `size`'s width, the area below the
// bar, over the dock: a scroll area of one row per theme, the theme in force's
// three sliders with Reset under it, then Close. Records every button and
// slider into preferences, and formats the three value labels into its buffers.
// Asserts when it is not showing.
void voe_editor_preferences_draw(voe_ui_context *ui,
				 voe_editor_preferences *preferences,
				 const voe_editor_themes *themes, float top,
				 voe_math_float2 size);

typedef enum {
	VOE_EDITOR_PREFERENCES_NONE,
	VOE_EDITOR_PREFERENCES_CHOOSE,
	VOE_EDITOR_PREFERENCES_CLOSE,
	// Any of the three sliders moved this frame; `contrast`, `separation`
	// and `text_scale` are what the theme in force is to be adjusted to.
	VOE_EDITOR_PREFERENCES_ADJUST,
	// Reset fired: the theme in force takes its own two back.
	VOE_EDITOR_PREFERENCES_RESET,
} voe_editor_preferences_action;

// What fired this frame; `index` is the themes.h entry chosen, for CHOOSE.
typedef struct {
	voe_editor_preferences_action action;
	uint32_t index;
	// Where the three sliders stand this frame, whatever the action is:
	// the first two inside VOE_UI_THEME_SCALAR_MIN..MAX, `text_scale`
	// inside VOE_EDITOR_TEXT_SCALE_MIN..MAX; nought each when the frame was
	// refused and there were no sliders to read.
	float contrast;
	float separation;
	float text_scale;
	// Any of the three sliders is held — the pointer went down on it and
	// has not let go. The file of remembered values is written when this goes false.
	bool sliding;
} voe_editor_preferences_result;

// Which button fired this frame. Called after voe_ui_frame_end and before
// the frame's arena is rewound — the one window in which a widget answers.
voe_editor_preferences_result
voe_editor_preferences_clicks_read(const voe_ui_context *ui,
				   const voe_editor_preferences *preferences);
