// The Sculpt section's one frame of `ui` calls and the read of its buttons and
// Strength slider afterwards. See the header for why the nodes live in the
// sculpt state and what a second press does.
//
// The buttons are indexed as voe_3d_brush_kind is (3d/landscape.h): Raise,
// Lower, Smooth, Flatten, so a button's index is its kind; PAINT, SCATTER and
// ERASE, past them, equal no button's index, so none is drawn chosen and any
// fired one chooses its kind.
#include "inspector_sculpt.h"

#include "themes.h"

#include <base/assert.h>

#include <ui/slider.h>
#include <ui/widgets.h>

#include <stdio.h>

// Between the things on one row, as inspector.c's component rows; the
// section's own pad and gap are ui's. Millimetres.
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)

// A slider's track. Millimetres.
#define SLIDER_WIDE 40.0f

static const char *const KIND_NAMES[VOE_EDITOR_SCULPT_KINDS] = {
	"Raise", "Lower", "Smooth", "Flatten"
};

void voe_editor_inspector_sculpt_forget(voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(sculpt != NULL, "forgetting no Sculpt section");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_KINDS; i++)
		sculpt->buttons[i] = VOE_UI_NODE_NONE;
	sculpt->strength_slider = VOE_UI_NODE_NONE;

	VOE_BASE_ASSERT(sculpt->buttons[0] == VOE_UI_NODE_NONE,
			"a Sculpt section left remembered");
}

void voe_editor_inspector_sculpt_draw(voe_ui_context *ui,
				      voe_editor_sculpt *sculpt,
				      voe_editor_folds *folds)
{
	char name[VOE_EDITOR_FOLD_NAME];
	bool open;

	VOE_BASE_ASSERT(ui != NULL, "drawing a Sculpt section into no interface");
	VOE_BASE_ASSERT(sculpt != NULL && folds != NULL,
			"drawing no Sculpt section or with no folds");

	voe_editor_folds_section_name(name, sizeof name, "sculpt");
	open = voe_editor_folds_begin(ui, folds, name, "sculpt", 0);
	voe_ui_label_role(ui, "Sculpt", VOE_UI_TEXT_ROLE_HEADING);
	voe_ui_end(ui);
	if (!open) {
		voe_ui_end(ui);
		return;
	}

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_KINDS; i++) {
		sculpt->buttons[i] = voe_ui_choice_begin(
			ui, "brush", i,
			sculpt->chosen && (uint32_t)sculpt->kind == i);
		voe_ui_label(ui, KIND_NAMES[i]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui);

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	voe_ui_label(ui, "Strength");
	sculpt->strength_slider = voe_ui_slider(
		ui, "brush slider", 0, sculpt->strength,
		VOE_EDITOR_SCULPT_STRENGTH_MIN, VOE_EDITOR_SCULPT_STRENGTH_MAX,
		SLIDER_WIDE);
	snprintf(sculpt->strength_figure, sizeof sculpt->strength_figure,
		 "%.2f", (double)sculpt->strength);
	voe_ui_label(ui, sculpt->strength_figure);
	voe_ui_end(ui);

	voe_ui_end(ui);
}

void voe_editor_inspector_sculpt_read(const voe_ui_context *ui,
				      voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a Sculpt section of no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "reading no Sculpt section");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_KINDS; i++) {
		if (sculpt->buttons[i] == VOE_UI_NODE_NONE ||
		    !voe_ui_button_action(ui, sculpt->buttons[i]).fired)
			continue;
		if (sculpt->chosen && (uint32_t)sculpt->kind == i) {
			sculpt->chosen = false;
		} else {
			sculpt->chosen = true;
			sculpt->kind = (voe_3d_brush_kind)i;
		}
	}

	if (sculpt->strength_slider != VOE_UI_NODE_NONE)
		sculpt->strength = (float)voe_ui_slider_action(
					   ui, sculpt->strength_slider,
					   VOE_EDITOR_SCULPT_STRENGTH_MIN,
					   VOE_EDITOR_SCULPT_STRENGTH_MAX)
					   .value;
}
