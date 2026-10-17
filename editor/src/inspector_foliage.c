// The Foliage section's one frame of `ui` calls and the read of its buttons,
// rows and sliders afterwards. See the header for what a press and a drop do
// and why the nodes live in the sculpt state.
//
// Button i is brush kind SCATTER + i, so Scatter then Erase. Row i is kind i,
// so a row's index is the picked bit it flips and the kind a drop replaces;
// the add row sits at the grid's kind count, the index a drop there adds at.
// The sliders hold sculpt.h's own numbers; only the figures beside Size, Turn
// and Lean are shown in per cent and degrees.
#include "inspector_foliage.h"

#include "inspector_layers.h"
#include "themes.h"

#include <assets/landscape.h>

#include <base/assert.h>

#include <platform/path.h>

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

// Degrees in a radian, for Turn's figure.
#define DEGREES (180.0f / 3.14159265f)

static_assert(VOE_ASSETS_LANDSCAPE_KINDS <= 32,
	      "a kind past the picked mask's bits");
static_assert(VOE_3D_BRUSH_ERASE == VOE_3D_BRUSH_SCATTER + 1,
	      "Scatter and Erase not consecutive");

static const char *const KIND_NAMES[VOE_EDITOR_SCULPT_FOLIAGE_KINDS] = {
	"Scatter", "Erase"
};

static const char *const SLIDER_NAMES[VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS] = {
	"Density", "Size (%)", "Turn (deg)", "Lean (%)"
};

static const float SLIDER_MIN[VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS] = {
	VOE_EDITOR_SCULPT_DENSITY_MIN, VOE_EDITOR_SCULPT_SIZE_VARY_MIN,
	VOE_EDITOR_SCULPT_TURN_VARY_MIN, VOE_EDITOR_SCULPT_LEAN_MIN
};

static const float SLIDER_MAX[VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS] = {
	VOE_EDITOR_SCULPT_DENSITY_MAX, VOE_EDITOR_SCULPT_SIZE_VARY_MAX,
	VOE_EDITOR_SCULPT_TURN_VARY_MAX, VOE_EDITOR_SCULPT_LEAN_MAX
};

// What each slider's figure shows of its number: density as it is, size and
// lean in per cent, turn in degrees.
static const float FIGURE_SCALE[VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS] = {
	1.0f, 100.0f, DEGREES, 100.0f
};

// The number slider `i` writes: density, size, turn, lean.
static float *slider_value(voe_editor_sculpt *sculpt, uint32_t i)
{
	VOE_BASE_ASSERT(i < VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS, "no such slider");

	return i == 0 ? &sculpt->density :
	       i == 1 ? &sculpt->size_vary :
	       i == 2 ? &sculpt->turn_vary :
			&sculpt->lean;
}

// Whether brush button `i` is the chosen kind.
static bool button_chosen(const voe_editor_sculpt *sculpt, uint32_t i)
{
	VOE_BASE_ASSERT(i < VOE_EDITOR_SCULPT_FOLIAGE_KINDS, "no such button");

	return sculpt->chosen &&
	       sculpt->kind == (voe_3d_brush_kind)(VOE_3D_BRUSH_SCATTER + i);
}

void voe_editor_inspector_foliage_forget(voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(sculpt != NULL, "forgetting no Foliage section");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_FOLIAGE_KINDS; i++)
		sculpt->foliage_buttons[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS; i++)
		sculpt->foliage_sliders[i] = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_KIND_ROWS; i++)
		sculpt->kind_rows[i] = VOE_UI_NODE_NONE;
	sculpt->kinds_drawn = 0;

	VOE_BASE_ASSERT(sculpt->kind_rows[0] == VOE_UI_NODE_NONE,
			"a Foliage section left remembered");
}

// Density, Size, Turn and Lean, each a row of name, slider and figure.
static void sliders_draw(voe_ui_context *ui, voe_editor_sculpt *sculpt)
{
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS; i++) {
		const float value = *slider_value(sculpt, i);

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, SLIDER_NAMES[i]);
		sculpt->foliage_sliders[i] =
			voe_ui_slider(ui, "foliage slider", i, value,
				      SLIDER_MIN[i], SLIDER_MAX[i], SLIDER_WIDE);
		snprintf(sculpt->foliage_figures[i],
			 sizeof sculpt->foliage_figures[i],
			 i == 0 ? "%.1f" : "%.0f",
			 (double)(value * FIGURE_SCALE[i]));
		voe_ui_label(ui, sculpt->foliage_figures[i]);
		voe_ui_end(ui);
	}
}

void voe_editor_inspector_foliage_draw(voe_ui_context *ui,
				       voe_editor_sculpt *sculpt,
				       const voe_3d_models *store,
				       const voe_ecs_world *world,
				       voe_ecs_entity entity)
{
	const voe_assets_landscape *land;

	VOE_BASE_ASSERT(ui != NULL, "drawing a Foliage section into no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "drawing no Foliage section");

	land = voe_editor_inspector_layers_grid(store, world, entity).grid;
	if (land == NULL)
		return;
	VOE_BASE_ASSERT(land->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"a grid with more kinds than it holds");

	voe_ui_panel_begin(ui, "foliage", 0, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = SECTION_GAP,
				   .pad = { SECTION_PAD, SECTION_PAD, SECTION_PAD,
					    SECTION_PAD } });
	voe_ui_label(ui, "Foliage");

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_FOLIAGE_KINDS; i++) {
		sculpt->foliage_buttons[i] = voe_ui_choice_begin(
			ui, "foliage brush", i, button_chosen(sculpt, i));
		voe_ui_label(ui, KIND_NAMES[i]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui);

	for (uint32_t i = 0; i < land->kind_count; i++) {
		sculpt->kind_rows[i] = voe_ui_choice_begin(
			ui, "kind", i, (sculpt->picked >> i & 1u) != 0);
		voe_ui_label(ui, voe_platform_path_name(land->kinds[i]));
		voe_ui_end(ui);
	}
	sculpt->kinds_drawn = land->kind_count;

	if (land->kind_count < VOE_ASSETS_LANDSCAPE_KINDS) {
		sculpt->kind_rows[land->kind_count] = voe_ui_row_begin(
			ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER });
		voe_ui_label(ui, "Drop a model to add a kind");
		voe_ui_end(ui);
	}

	sliders_draw(ui, sculpt);
	voe_ui_end(ui);
}

void voe_editor_inspector_foliage_read(const voe_ui_context *ui,
				       voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a Foliage section of no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "reading no Foliage section");
	VOE_BASE_ASSERT(sculpt->kinds_drawn <= VOE_ASSETS_LANDSCAPE_KINDS,
			"more kind rows drawn than a grid holds");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_KIND_ROWS; i++)
		sculpt->kinds_seen[i] =
			sculpt->kind_rows[i] == VOE_UI_NODE_NONE ?
				(voe_ui_rect){ 0 } :
				voe_ui_node_visible(ui, sculpt->kind_rows[i]);

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_FOLIAGE_KINDS; i++) {
		if (sculpt->foliage_buttons[i] == VOE_UI_NODE_NONE ||
		    !voe_ui_button_action(ui, sculpt->foliage_buttons[i]).fired)
			continue;
		if (button_chosen(sculpt, i)) {
			sculpt->chosen = false;
		} else {
			sculpt->chosen = true;
			sculpt->kind =
				(voe_3d_brush_kind)(VOE_3D_BRUSH_SCATTER + i);
		}
	}

	for (uint32_t i = 0; i < sculpt->kinds_drawn; i++)
		if (sculpt->kind_rows[i] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, sculpt->kind_rows[i]).fired)
			sculpt->picked ^= 1u << i;

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS; i++) {
		float value;

		if (sculpt->foliage_sliders[i] == VOE_UI_NODE_NONE)
			continue;
		value = (float)voe_ui_slider_action(ui, sculpt->foliage_sliders[i],
						    SLIDER_MIN[i], SLIDER_MAX[i])
				.value;
		*slider_value(sculpt, i) =
			value < SLIDER_MIN[i] ? SLIDER_MIN[i] :
			value > SLIDER_MAX[i] ? SLIDER_MAX[i] :
						value;
	}
}
