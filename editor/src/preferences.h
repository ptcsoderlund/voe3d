// The editor's Preferences: every theme themes.h lists, one row each with the
// theme's display name and a Choose button, the one in force marked; beside
// them the font group, three rows — `Theme's own`, `Pixel Operator`,
// `Oxanium` — each with a Choose button, the one equal to
// `themes->font_choice` marked the same way (ADR-0179); and a Close button —
// shown as an anchored panel over the dock, below the bar, exactly where
// browser.h's own panel goes.
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
// THE FONT ROWS ARE DRAWN IN THE THEME IN FORCE, pushed in no entry's
// palette: the override applies to whichever theme is chosen, so it belongs
// to no one theme's look, and the theme in force — its font already the
// override — is what shows a person the face they have chosen.
//
// THIS FILE CARRIES OUT NO COMMAND OF ITS OWN, exactly as topbar.h's and
// browser.h's buttons do not: a `ui` widget answers only after
// voe_ui_frame_end, so the draw records each button and
// voe_editor_preferences_clicks_read says which fired. What Choose does — the
// theme or the font override put in force and remembered — is themes.h's,
// called by interface.c.
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

// The font group's rows, one per voe_editor_font_choice, in its order.
#define VOE_EDITOR_PREFERENCES_FONTS 3

// Zeroed is a panel never shown.
typedef struct {
	bool showing;
	// One Choose button per listed theme, recorded by the draw.
	voe_ui_node choose_buttons[VOE_EDITOR_PREFERENCES_ROWS];
	uint32_t row_count;
	// One Choose button per font row, indexed by voe_editor_font_choice.
	voe_ui_node font_buttons[VOE_EDITOR_PREFERENCES_FONTS];
	voe_ui_node close_button;
} voe_editor_preferences;

void voe_editor_preferences_show(voe_editor_preferences *preferences);
void voe_editor_preferences_hide(voe_editor_preferences *preferences);

// Draws the panel filling top..size.y of `size`'s width, the area below the
// bar, over the dock: a scroll area of one row per theme, the three font
// rows, then Close. Records
// every button into preferences. Asserts when it is not showing.
void voe_editor_preferences_draw(voe_ui_context *ui,
				 voe_editor_preferences *preferences,
				 const voe_editor_themes *themes, float top,
				 voe_math_float2 size);

typedef enum {
	VOE_EDITOR_PREFERENCES_NONE,
	VOE_EDITOR_PREFERENCES_CHOOSE,
	VOE_EDITOR_PREFERENCES_FONT,
	VOE_EDITOR_PREFERENCES_CLOSE,
} voe_editor_preferences_action;

// What fired this frame; `index` is the themes.h entry chosen, for CHOOSE,
// and the voe_editor_font_choice chosen, for FONT.
typedef struct {
	voe_editor_preferences_action action;
	uint32_t index;
} voe_editor_preferences_result;

// Which button fired this frame. Called after voe_ui_frame_end and before
// the frame's arena is rewound — the one window in which a widget answers.
voe_editor_preferences_result
voe_editor_preferences_clicks_read(const voe_ui_context *ui,
				   const voe_editor_preferences *preferences);
