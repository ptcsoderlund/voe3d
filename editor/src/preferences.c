// The Preferences panel's one frame of `ui` calls and the read of its buttons
// afterwards. See the header for why each row is pushed into its own theme and
// why Choose is carried out elsewhere.
#include "preferences.h"

#include <base/assert.h>

#include <ui/slider.h>
#include <ui/theme.h>

#include <stdio.h>

// The same plate, padding and gaps browser.c's panel uses, so the two
// floating panels read as one kind of thing. Millimetres.
#define PREFERENCES_PAD 3.0f
#define PREFERENCES_GAP 2.0f

// What the row of the theme in force ends in.
#define IN_FORCE_MARK "(in force)"

// How wide a scalar's track is, in millimetres: wide enough that the whole
// range is a comfortable drag, narrow enough to sit in a panel this wide.
#define PREFERENCES_SLIDER_WIDE 60.0f

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

	// The theme in force's own three scalars, under the list: a row each of
	// the scalar's name, its slider and where it stands, then Reset. Each
	// row wraps, so at a large text size a slider drops under its name
	// rather than out of the panel. The three numbers are formatted into the struct because a label's text is
	// read after this call (the header).
	{
		const voe_editor_theme *chosen =
			voe_editor_themes_chosen(themes);

		voe_ui_column_begin(ui,
				    (voe_ui_container){
					    .across = VOE_UI_ACROSS_FILL,
					    .gap = PREFERENCES_GAP });

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = PREFERENCES_GAP,
					     .wrap = true });
		voe_ui_label(ui, "Contrast");
		preferences->contrast_slider = voe_ui_slider(
			ui, "contrast", 0, chosen->contrast_strength,
			VOE_UI_THEME_SCALAR_MIN, VOE_UI_THEME_SCALAR_MAX,
			PREFERENCES_SLIDER_WIDE);
		snprintf(preferences->contrast_text,
			 sizeof preferences->contrast_text, "%.2f",
			 (double)chosen->contrast_strength);
		voe_ui_label(ui, preferences->contrast_text);
		voe_ui_end(ui); // contrast row

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = PREFERENCES_GAP,
					     .wrap = true });
		voe_ui_label(ui, "Surface separation");
		preferences->separation_slider = voe_ui_slider(
			ui, "separation", 0, chosen->surface_separation,
			VOE_UI_THEME_SCALAR_MIN, VOE_UI_THEME_SCALAR_MAX,
			PREFERENCES_SLIDER_WIDE);
		snprintf(preferences->separation_text,
			 sizeof preferences->separation_text, "%.2f",
			 (double)chosen->surface_separation);
		voe_ui_label(ui, preferences->separation_text);
		voe_ui_end(ui); // separation row

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = PREFERENCES_GAP,
					     .wrap = true });
		voe_ui_label(ui, "Text size");
		preferences->text_size_slider = voe_ui_slider(
			ui, "text_size", 0, chosen->text_scale,
			VOE_EDITOR_TEXT_SCALE_MIN, VOE_EDITOR_TEXT_SCALE_MAX,
			PREFERENCES_SLIDER_WIDE);
		snprintf(preferences->text_size_text,
			 sizeof preferences->text_size_text, "%.0f%%",
			 (double)chosen->text_scale * 100.0);
		voe_ui_label(ui, preferences->text_size_text);
		voe_ui_end(ui); // text size row

		preferences->reset_button = voe_ui_button_begin(ui, "reset", 0);
		voe_ui_label(ui, "Reset");
		voe_ui_end(ui); // reset button

		voe_ui_end(ui); // scalars column
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
	voe_ui_slider_result contrast = { 0 };
	voe_ui_slider_result separation = { 0 };
	voe_ui_slider_result text_size = { 0 };
	voe_editor_preferences_result result = {
		.action = VOE_EDITOR_PREFERENCES_NONE
	};

	if (preferences->contrast_slider != VOE_UI_NODE_NONE)
		contrast = voe_ui_slider_action(ui,
						preferences->contrast_slider,
						VOE_UI_THEME_SCALAR_MIN,
						VOE_UI_THEME_SCALAR_MAX);
	if (preferences->separation_slider != VOE_UI_NODE_NONE)
		separation = voe_ui_slider_action(ui,
						  preferences->separation_slider,
						  VOE_UI_THEME_SCALAR_MIN,
						  VOE_UI_THEME_SCALAR_MAX);
	if (preferences->text_size_slider != VOE_UI_NODE_NONE)
		text_size = voe_ui_slider_action(ui,
						 preferences->text_size_slider,
						 VOE_EDITOR_TEXT_SCALE_MIN,
						 VOE_EDITOR_TEXT_SCALE_MAX);
	result.contrast = (float)contrast.value;
	result.separation = (float)separation.value;
	result.text_scale = (float)text_size.value;
	result.sliding = contrast.held || separation.held || text_size.held;

	for (uint32_t i = 0; i < preferences->row_count; i++)
		if (preferences->choose_buttons[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, preferences->choose_buttons[i])
			    .fired) {
			result.action = VOE_EDITOR_PREFERENCES_CHOOSE;
			result.index = i;
			return result;
		}

	// Choose and Close win over a slider that moved in the same frame, and
	// so does Reset: a click said what to do with the scalars outright.
	if (preferences->close_button != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, preferences->close_button).fired)
		result.action = VOE_EDITOR_PREFERENCES_CLOSE;
	else if (preferences->reset_button != VOE_UI_NODE_NONE &&
		 voe_ui_button_action(ui, preferences->reset_button).fired)
		result.action = VOE_EDITOR_PREFERENCES_RESET;
	else if (contrast.changed || separation.changed || text_size.changed)
		result.action = VOE_EDITOR_PREFERENCES_ADJUST;

	return result;
}
