// The Preferences panel's one frame of `ui` calls and the read of its buttons
// afterwards. See the header for why each theme row is pushed into its own
// theme, why the font rows are not, and why Choose is carried out elsewhere.
#include "preferences.h"

#include <base/assert.h>

// The same plate, padding and gaps browser.c's panel uses, so the two
// floating panels read as one kind of thing. Millimetres.
#define PREFERENCES_PAD 3.0f
#define PREFERENCES_GAP 2.0f

// What the row of the theme in force ends in.
#define IN_FORCE_MARK "(in force)"

// What each font row reads, indexed by voe_editor_font_choice.
static const char *const FONT_NAMES[VOE_EDITOR_PREFERENCES_FONTS] = {
	[VOE_EDITOR_FONT_THEME] = "Theme's own",
	[VOE_EDITOR_FONT_PIXEL_OPERATOR] = "Pixel Operator",
	[VOE_EDITOR_FONT_OXANIUM] = "Oxanium",
};

void voe_editor_preferences_show(voe_editor_preferences *preferences)
{
	VOE_BASE_ASSERT(preferences != NULL, "showing no preferences");

	preferences->showing = true;
}

void voe_editor_preferences_hide(voe_editor_preferences *preferences)
{
	VOE_BASE_ASSERT(preferences != NULL, "hiding no preferences");

	preferences->showing = false;
}

void voe_editor_preferences_draw(voe_ui_context *ui,
				 voe_editor_preferences *preferences,
				 const voe_editor_themes *themes, float top,
				 voe_math_float2 size)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing no preferences into no interface");
	VOE_BASE_ASSERT(preferences != NULL, "drawing no preferences");
	VOE_BASE_ASSERT(preferences->showing,
			"drawing preferences that are not showing");
	VOE_BASE_ASSERT(themes != NULL, "drawing preferences with no themes");

	preferences->row_count = themes->count < VOE_EDITOR_PREFERENCES_ROWS ?
					 themes->count :
					 VOE_EDITOR_PREFERENCES_ROWS;

	// Anchored exactly as browser.c's panel is — see its own comment.
	voe_ui_panel_begin(
		ui, "preferences", 0, VOE_UI_SURFACE_RAISED,
		(voe_ui_container){
			.across = VOE_UI_ACROSS_FILL,
			.gap = PREFERENCES_GAP,
			.pad = { PREFERENCES_PAD, PREFERENCES_PAD,
				 PREFERENCES_PAD, PREFERENCES_PAD },
			.anchor = { .anchored = true,
				    .x = { VOE_UI_ACROSS_FILL, 0.0f },
				    .y = { VOE_UI_ACROSS_START, top } },
			.size = { .across = { VOE_UI_SIZE_FIXED, size.y } } });

	voe_ui_scroll_begin(
		ui, "theme_rows", 0,
		(voe_ui_container){ .size = { .along = { VOE_UI_SIZE_GROW,
							 1.0f } },
				    .across = VOE_UI_ACROSS_FILL,
				    .gap = PREFERENCES_GAP },
		(voe_ui_scroll_axes){ .y = true });
	for (uint32_t i = 0; i < preferences->row_count; i++) {
		const voe_editor_theme *entry = &themes->entries[i];

		// A panel so the row has the theme's own surface behind it,
		// holding a row so its contents run left to right.
		voe_ui_theme_push(ui, &entry->palette);
		voe_ui_panel_begin(ui, "theme", i, VOE_UI_SURFACE_SURFACE,
				   (voe_ui_container){
					   .across = VOE_UI_ACROSS_FILL,
					   .pad = { PREFERENCES_PAD,
						    PREFERENCES_PAD,
						    PREFERENCES_PAD,
						    PREFERENCES_PAD } });
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = PREFERENCES_GAP });
		voe_ui_label(ui, entry->name);
		if (i == themes->chosen)
			voe_ui_label(ui, IN_FORCE_MARK);
		preferences->choose_buttons[i] =
			voe_ui_button_begin(ui, "choose", i);
		voe_ui_label(ui, "Choose");
		voe_ui_end(ui); // choose button
		voe_ui_end(ui); // row
		voe_ui_end(ui); // panel
		voe_ui_theme_pop(ui);
	}
	voe_ui_end(ui); // scroll area

	// The font group, laid out as a theme row is but in the theme in
	// force — see the header.
	for (uint32_t i = 0; i < VOE_EDITOR_PREFERENCES_FONTS; i++) {
		voe_ui_panel_begin(ui, "font", i, VOE_UI_SURFACE_SURFACE,
				   (voe_ui_container){
					   .across = VOE_UI_ACROSS_FILL,
					   .pad = { PREFERENCES_PAD,
						    PREFERENCES_PAD,
						    PREFERENCES_PAD,
						    PREFERENCES_PAD } });
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = PREFERENCES_GAP });
		voe_ui_label(ui, FONT_NAMES[i]);
		if (i == (uint32_t)themes->font_choice)
			voe_ui_label(ui, IN_FORCE_MARK);
		preferences->font_buttons[i] =
			voe_ui_button_begin(ui, "font_choose", i);
		voe_ui_label(ui, "Choose");
		voe_ui_end(ui); // choose button
		voe_ui_end(ui); // row
		voe_ui_end(ui); // panel
	}

	// In a row of its own so it keeps its natural width, as browser.c's
	// Cancel does.
	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = PREFERENCES_GAP });
	preferences->close_button = voe_ui_button_begin(ui, "close", 0);
	voe_ui_label(ui, "Close");
	voe_ui_end(ui); // close button
	voe_ui_end(ui); // bottom row

	voe_ui_end(ui); // panel
}

voe_editor_preferences_result
voe_editor_preferences_clicks_read(const voe_ui_context *ui,
				   const voe_editor_preferences *preferences)
{
	VOE_BASE_ASSERT(ui != NULL, "reading the clicks of no interface");
	VOE_BASE_ASSERT(preferences != NULL,
			"reading the clicks of no preferences");

	// A refused frame hands back VOE_UI_NODE_NONE past the node budget,
	// and those are skipped, exactly as topbar.c's are.
	for (uint32_t i = 0; i < preferences->row_count; i++)
		if (preferences->choose_buttons[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, preferences->choose_buttons[i])
			    .fired)
			return (voe_editor_preferences_result){
				.action = VOE_EDITOR_PREFERENCES_CHOOSE,
				.index = i
			};

	for (uint32_t i = 0; i < VOE_EDITOR_PREFERENCES_FONTS; i++)
		if (preferences->font_buttons[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, preferences->font_buttons[i]).fired)
			return (voe_editor_preferences_result){
				.action = VOE_EDITOR_PREFERENCES_FONT,
				.index = i
			};

	if (preferences->close_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, preferences->close_button).fired)
		return (voe_editor_preferences_result){
			.action = VOE_EDITOR_PREFERENCES_CLOSE
		};

	return (voe_editor_preferences_result){
		.action = VOE_EDITOR_PREFERENCES_NONE
	};
}
