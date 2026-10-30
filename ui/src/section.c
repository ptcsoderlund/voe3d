// The folding section: a bordered box and its header row with the fold button
// first, built from voe_ui_panel_begin, voe_ui_row_begin, voe_ui_button_begin
// and voe_ui_label. See include/ui/widgets.h for the promises and the usage.
//
// IT HOLDS NO STATE (ADR-0503). Whether the body shows is the caller's flag,
// handed in every frame; this file only reads it to pick the button's glyph.
// The caller composes the title, closes the header row and the box, and flips
// its flag when the fold button fires.
#include "context.h"

#include <base/assert.h>

// The box's pad and gap and the header row's gap, in millimetres at a
// spacing of one, the theme's `spacing` scaling all three as it scales a
// button's pad.
#define SECTION_PAD 2.0f
#define SECTION_GAP 1.5f
#define HEADER_GAP 2.0f

voe_ui_section voe_ui_section_begin(voe_ui_context *ui, const char *name,
				    uint32_t index, bool open)
{
	const voe_ui_theme *theme = voe_ui_theme_current(ui);
	voe_ui_section section;
	float pad;

	VOE_BASE_ASSERT(ui != NULL, "opening a section on no context");
	VOE_BASE_ASSERT(name != NULL, "a section with no name has no identity");
	VOE_BASE_ASSERT(theme != NULL,
			"a section needs a theme: see "
			"voe_ui_theme_set/voe_ui_theme_push");

	pad = SECTION_PAD * theme->spacing;

	// FILL across, so the header row, the body's rows and a section nested
	// in the body all take the box's width.
	section.box = voe_ui_panel_begin(
		ui, name, index, VOE_UI_SURFACE_SURFACE,
		(voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
				    .gap = SECTION_GAP * theme->spacing,
				    .pad = { pad, pad, pad, pad } });

	voe_ui_row_begin(ui, (voe_ui_container){
				     .across = VOE_UI_ACROSS_CENTER,
				     .gap = HEADER_GAP * theme->spacing });

	section.fold = voe_ui_button_begin(ui, "fold", 0);
	voe_ui_label(ui, open ? "-" : "+");
	voe_ui_end(ui);

	return section;
}
