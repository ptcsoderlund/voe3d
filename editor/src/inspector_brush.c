// The Brush section's one frame of `ui` calls and the read of its sliders
// afterwards. See the header for when it shows and why the nodes live in the
// sculpt state.
//
// Slider 0 is Radius (m), slider 1 Softness, drawn as the Sculpt section drew
// them: radius's figure to one decimal, softness's to two.
#include "inspector_brush.h"

#include "themes.h"

#include <base/assert.h>

#include <ui/slider.h>
#include <ui/widgets.h>

#include <stdio.h>

// As inspector_sculpt.c's section: inside its edges, between its rows, and
// between the things on one row. Millimetres.
#define SECTION_PAD (2.0f * VOE_EDITOR_SPACING)
#define SECTION_GAP (1.5f * VOE_EDITOR_SPACING)
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)

// A slider's track, as the Sculpt section's. Millimetres.
#define SLIDER_WIDE 40.0f

static const char *const SLIDER_NAMES[VOE_EDITOR_SCULPT_BRUSH_SLIDERS] = {
	"Radius (m)", "Softness"
};

static const float SLIDER_MIN[VOE_EDITOR_SCULPT_BRUSH_SLIDERS] = {
	VOE_EDITOR_SCULPT_RADIUS_MIN, VOE_EDITOR_SCULPT_SOFTNESS_MIN
};

static const float SLIDER_MAX[VOE_EDITOR_SCULPT_BRUSH_SLIDERS] = {
	VOE_EDITOR_SCULPT_RADIUS_MAX, VOE_EDITOR_SCULPT_SOFTNESS_MAX
};

// The number slider `i` writes: radius, softness.
static float *slider_value(voe_editor_sculpt *sculpt, uint32_t i)
{
	VOE_BASE_ASSERT(i < VOE_EDITOR_SCULPT_BRUSH_SLIDERS, "no such slider");

	return i == 0 ? &sculpt->radius : &sculpt->softness;
}

void voe_editor_inspector_brush_forget(voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(sculpt != NULL, "forgetting no Brush section");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_BRUSH_SLIDERS; i++)
		sculpt->brush_sliders[i] = VOE_UI_NODE_NONE;

	VOE_BASE_ASSERT(sculpt->brush_sliders[0] == VOE_UI_NODE_NONE,
			"a Brush section left remembered");
}

void voe_editor_inspector_brush_draw(voe_ui_context *ui,
				     voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(ui != NULL, "drawing a Brush section into no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "drawing no Brush section");

	if (!sculpt->chosen)
		return;

	voe_ui_panel_begin(ui, "brush", 0, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = SECTION_GAP,
				   .pad = { SECTION_PAD, SECTION_PAD, SECTION_PAD,
					    SECTION_PAD } });
	voe_ui_label(ui, "Brush");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_BRUSH_SLIDERS; i++) {
		const float value = *slider_value(sculpt, i);

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, SLIDER_NAMES[i]);
		sculpt->brush_sliders[i] = voe_ui_slider(ui, "brush slider", i,
							 value, SLIDER_MIN[i],
							 SLIDER_MAX[i],
							 SLIDER_WIDE);
		snprintf(sculpt->brush_figures[i], sizeof sculpt->brush_figures[i],
			 i == 0 ? "%.1f" : "%.2f", (double)value);
		voe_ui_label(ui, sculpt->brush_figures[i]);
		voe_ui_end(ui);
	}

	voe_ui_end(ui);
}

void voe_editor_inspector_brush_read(const voe_ui_context *ui,
				     voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a Brush section of no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "reading no Brush section");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_BRUSH_SLIDERS; i++)
		if (sculpt->brush_sliders[i] != VOE_UI_NODE_NONE)
			*slider_value(sculpt, i) = (float)voe_ui_slider_action(
							   ui, sculpt->brush_sliders[i],
							   SLIDER_MIN[i],
							   SLIDER_MAX[i])
							   .value;
}
