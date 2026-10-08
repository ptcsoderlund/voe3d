// The Sculpt section's one frame of `ui` calls and the read of its buttons and
// sliders afterwards. See the header for why the nodes live in the sculpt
// state and what a second press does.
//
// The buttons are indexed as voe_3d_brush_kind is (3d/landscape.h): Raise,
// Lower, Smooth, Flatten, so a button's index is its kind.
#include "inspector_sculpt.h"

#include "themes.h"

#include <base/assert.h>

#include <ui/slider.h>
#include <ui/widgets.h>

#include <stdio.h>

// As inspector.c's component panel: inside its edges, between its rows, and
// between the things on one row. Millimetres.
#define SECTION_PAD (2.0f * VOE_EDITOR_SPACING)
#define SECTION_GAP (1.5f * VOE_EDITOR_SPACING)
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)

// A slider's track. Millimetres.
#define SLIDER_WIDE 40.0f

static const char *const KIND_NAMES[VOE_EDITOR_SCULPT_KINDS] = {
	"Raise", "Lower", "Smooth", "Flatten"
};

static const char *const SLIDER_NAMES[VOE_EDITOR_SCULPT_SLIDERS] = {
	"Radius (m)", "Strength", "Softness"
};

static const float SLIDER_MIN[VOE_EDITOR_SCULPT_SLIDERS] = {
	VOE_EDITOR_SCULPT_RADIUS_MIN, VOE_EDITOR_SCULPT_STRENGTH_MIN,
	VOE_EDITOR_SCULPT_SOFTNESS_MIN
};

static const float SLIDER_MAX[VOE_EDITOR_SCULPT_SLIDERS] = {
	VOE_EDITOR_SCULPT_RADIUS_MAX, VOE_EDITOR_SCULPT_STRENGTH_MAX,
	VOE_EDITOR_SCULPT_SOFTNESS_MAX
};

// The number slider `i` writes: radius, strength, softness.
static float *slider_value(voe_editor_sculpt *sculpt, uint32_t i)
{
	VOE_BASE_ASSERT(i < VOE_EDITOR_SCULPT_SLIDERS, "no such slider");

	return i == 0 ? &sculpt->radius :
	       i == 1 ? &sculpt->strength :
			&sculpt->softness;
}

void voe_editor_inspector_sculpt_forget(voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(sculpt != NULL, "forgetting no Sculpt section");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_KINDS; i++)
		sculpt->buttons[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_SLIDERS; i++)
		sculpt->sliders[i] = VOE_UI_NODE_NONE;

	VOE_BASE_ASSERT(sculpt->buttons[0] == VOE_UI_NODE_NONE,
			"a Sculpt section left remembered");
}

void voe_editor_inspector_sculpt_draw(voe_ui_context *ui,
				      voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing a Sculpt section into no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "drawing no Sculpt section");

	voe_ui_panel_begin(ui, "sculpt", 0, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = SECTION_GAP,
				   .pad = { SECTION_PAD, SECTION_PAD, SECTION_PAD,
					    SECTION_PAD } });
	voe_ui_label(ui, "Sculpt");

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

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_SLIDERS; i++) {
		const float value = *slider_value(sculpt, i);

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, SLIDER_NAMES[i]);
		sculpt->sliders[i] = voe_ui_slider(ui, "brush slider", i,
						   value, SLIDER_MIN[i],
						   SLIDER_MAX[i], SLIDER_WIDE);
		snprintf(sculpt->figures[i], sizeof sculpt->figures[i],
			 i == 0 ? "%.1f" : "%.2f", (double)value);
		voe_ui_label(ui, sculpt->figures[i]);
		voe_ui_end(ui);
	}

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

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_SLIDERS; i++)
		if (sculpt->sliders[i] != VOE_UI_NODE_NONE)
			*slider_value(sculpt, i) = (float)voe_ui_slider_action(
							   ui, sculpt->sliders[i],
							   SLIDER_MIN[i],
							   SLIDER_MAX[i])
							   .value;
}
