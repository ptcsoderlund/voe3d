// The Foliage section's one frame of `ui` calls and the read of its buttons,
// rows and sliders afterwards. See the header for what a press and a drop do
// and why the nodes live in the sculpt state.
//
// Button i is brush kind SCATTER + i, so Scatter then Erase. Row i is kind i,
// so a row's index is the picked bit it flips and the kind a drop replaces;
// the add row sits at the grid's kind count, the index a drop there adds at.
// The Density slider holds sculpt.h's own number. Kind slider i, under row i,
// shows kind i's base size as a factor; its read asks for kind i's look as
// drawn with the size in thousandths, which sculpt.c sets. Under it kind i's
// Cast shadows button, drawn chosen while the look casts, and spread slider
// i, s, spread s of kind i in the look's whole units, asked the same way.
#include "inspector_foliage.h"

#include "inspector_layers.h"
#include "themes.h"

#include <assets/landscape.h>

#include <base/assert.h>

#include <platform/path.h>

#include <ui/slider.h>
#include <ui/widgets.h>

#include <math.h>
#include <stdio.h>

// As inspector_sculpt.c's section: inside its edges, between its rows, and
// between the things on one row. Millimetres.
#define SECTION_PAD (2.0f * VOE_EDITOR_SPACING)
#define SECTION_GAP (1.5f * VOE_EDITOR_SPACING)
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)

// A slider's track, as the Sculpt section's. Millimetres.
#define SLIDER_WIDE 40.0f

static_assert(VOE_ASSETS_LANDSCAPE_KINDS <= 32,
	      "a kind past the picked mask's bits");
static_assert(VOE_3D_BRUSH_ERASE == VOE_3D_BRUSH_SCATTER + 1,
	      "Scatter and Erase not consecutive");

static const char *const KIND_NAMES[VOE_EDITOR_SCULPT_FOLIAGE_KINDS] = {
	"Scatter", "Erase"
};

static_assert(VOE_EDITOR_SCULPT_FOLIAGE_SLIDERS == 1,
	      "the brush's sliders past Density alone");

// A kind slider's range: the file's thousandths as a factor.
#define KIND_SIZE_MIN (VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN / 1000.0)
#define KIND_SIZE_MAX (VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX / 1000.0)

// A kind's spreads, slider s of its row: the label as the brush's was, and
// the most, each from 0.
static const char *const SPREAD_NAMES[VOE_EDITOR_SCULPT_SPREADS] = {
	"Size ±%", "Turn ±°", "Tilt with slope %"
};
static const uint8_t SPREAD_MAX[VOE_EDITOR_SCULPT_SPREADS] = {
	VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX, VOE_ASSETS_LANDSCAPE_TURN_VARY_MAX,
	VOE_ASSETS_LANDSCAPE_TILT_MAX
};

// Spread `s` of `look`, the field slider s shows and sets.
static uint8_t *spread_of(voe_assets_landscape_look *look, uint32_t s)
{
	VOE_BASE_ASSERT(look != NULL, "the spread of no look");
	VOE_BASE_ASSERT(s < VOE_EDITOR_SCULPT_SPREADS, "no such spread");

	return s == 0 ? &look->size_vary : s == 1 ? &look->turn_vary : &look->tilt;
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
	for (uint32_t i = 0; i < VOE_ASSETS_LANDSCAPE_KINDS; i++) {
		sculpt->kind_sliders[i] = VOE_UI_NODE_NONE;
		sculpt->kind_shadows[i] = VOE_UI_NODE_NONE;
		for (uint32_t s = 0; s < VOE_EDITOR_SCULPT_SPREADS; s++)
			sculpt->spread_sliders[i][s] = VOE_UI_NODE_NONE;
	}
	sculpt->kinds_drawn = 0;

	VOE_BASE_ASSERT(sculpt->kind_rows[0] == VOE_UI_NODE_NONE,
			"a Foliage section left remembered");
}

// Kind `i`'s base-size slider, a row of name, slider and figure as the
// section's other sliders.
static void kind_slider_draw(voe_ui_context *ui, voe_editor_sculpt *sculpt,
			     const voe_assets_landscape *land, uint32_t i)
{
	const float size = voe_assets_landscape_kind_size(land, i);

	VOE_BASE_ASSERT(i < land->kind_count, "a slider for no kind");
	sculpt->kind_looks[i] = land->looks[i];
	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	voe_ui_label(ui, "Base size");
	sculpt->kind_sliders[i] = voe_ui_slider(ui, "kind size", i, size,
						KIND_SIZE_MIN, KIND_SIZE_MAX,
						SLIDER_WIDE);
	snprintf(sculpt->kind_figures[i], sizeof sculpt->kind_figures[i],
		 "%.2f×", (double)size);
	voe_ui_label(ui, sculpt->kind_figures[i]);
	voe_ui_end(ui);
	VOE_BASE_ASSERT(sculpt->kind_sliders[i] != VOE_UI_NODE_NONE,
			"a kind slider not drawn");
}

// Kind `i`'s Cast shadows button, then its spreads, each a row of name,
// slider and figure. Reads the look kind_slider_draw kept.
static void kind_spreads_draw(voe_ui_context *ui, voe_editor_sculpt *sculpt,
			      uint32_t i)
{
	VOE_BASE_ASSERT(i < VOE_ASSETS_LANDSCAPE_KINDS, "spreads for no kind");

	sculpt->kind_shadows[i] = voe_ui_choice_begin(
		ui, "kind shadows", i, !sculpt->kind_looks[i].shadowless);
	voe_ui_label(ui, "Cast shadows");
	voe_ui_end(ui);
	for (uint32_t s = 0; s < VOE_EDITOR_SCULPT_SPREADS; s++) {
		const uint8_t value = *spread_of(&sculpt->kind_looks[i], s);

		voe_ui_row_begin(ui,
				 (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						     .gap = ROW_GAP,
						     .wrap = true });
		voe_ui_label(ui, SPREAD_NAMES[s]);
		sculpt->spread_sliders[i][s] = voe_ui_slider(
			ui, "kind spread", i * VOE_EDITOR_SCULPT_SPREADS + s,
			value, 0.0, SPREAD_MAX[s], SLIDER_WIDE);
		snprintf(sculpt->spread_figures[i][s],
			 sizeof sculpt->spread_figures[i][s], "%u",
			 (unsigned)value);
		voe_ui_label(ui, sculpt->spread_figures[i][s]);
		voe_ui_end(ui);
	}
	VOE_BASE_ASSERT(sculpt->kind_shadows[i] != VOE_UI_NODE_NONE,
			"a Cast shadows button not drawn");
}

// Density, a row of name, slider and figure.
static void density_draw(voe_ui_context *ui, voe_editor_sculpt *sculpt)
{
	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	voe_ui_label(ui, "Density");
	sculpt->foliage_sliders[0] =
		voe_ui_slider(ui, "foliage slider", 0, sculpt->density,
			      VOE_EDITOR_SCULPT_DENSITY_MIN,
			      VOE_EDITOR_SCULPT_DENSITY_MAX, SLIDER_WIDE);
	snprintf(sculpt->foliage_figures[0], sizeof sculpt->foliage_figures[0],
		 "%.1f", (double)sculpt->density);
	voe_ui_label(ui, sculpt->foliage_figures[0]);
	voe_ui_end(ui);
	VOE_BASE_ASSERT(sculpt->foliage_sliders[0] != VOE_UI_NODE_NONE,
			"the Density slider not drawn");
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
		kind_slider_draw(ui, sculpt, land, i);
		kind_spreads_draw(ui, sculpt, i);
	}
	sculpt->kinds_drawn = land->kind_count;

	if (land->kind_count < VOE_ASSETS_LANDSCAPE_KINDS) {
		sculpt->kind_rows[land->kind_count] = voe_ui_row_begin(
			ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER });
		voe_ui_label(ui, "Drop a model to add a kind");
		voe_ui_end(ui);
	}

	density_draw(ui, sculpt);
	voe_ui_end(ui);
}

// Kind `i`'s look as drawn asked, for the caller to change one field of.
static voe_assets_landscape_look *look_ask(voe_editor_sculpt *sculpt,
					   uint32_t i)
{
	VOE_BASE_ASSERT(i < VOE_ASSETS_LANDSCAPE_KINDS, "asking no kind's look");

	sculpt->look_asked = true;
	sculpt->look_kind = i;
	sculpt->look_to = sculpt->kind_looks[i];
	return &sculpt->look_to;
}

// Kind `i`'s Base size slider: held recorded, a change asked in thousandths.
static void kind_size_read(const voe_ui_context *ui, voe_editor_sculpt *sculpt,
			   uint32_t i)
{
	voe_ui_slider_result size;
	long to;

	if (sculpt->kind_sliders[i] == VOE_UI_NODE_NONE)
		return;
	size = voe_ui_slider_action(ui, sculpt->kind_sliders[i], KIND_SIZE_MIN,
				    KIND_SIZE_MAX);
	sculpt->look_held = sculpt->look_held || size.held;
	if (!size.changed)
		return;
	to = lround(size.value * 1000.0);
	look_ask(sculpt, i)->size =
		(uint16_t)(to < VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN ?
				   VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN :
			   to > VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX ?
				   VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX :
				   to);
}

// Kind `i`'s Cast shadows and spreads. A fired button asks the look with
// `shadowless` flipped and never holds, so it is one step at once; a spread
// slider is held and asked as the Base size slider is, in whole units.
static void kind_spreads_read(const voe_ui_context *ui,
			      voe_editor_sculpt *sculpt, uint32_t i)
{
	if (sculpt->kind_shadows[i] != VOE_UI_NODE_NONE &&
	    voe_ui_button_action(ui, sculpt->kind_shadows[i]).fired)
		look_ask(sculpt, i)->shadowless =
			!sculpt->kind_looks[i].shadowless;

	for (uint32_t s = 0; s < VOE_EDITOR_SCULPT_SPREADS; s++) {
		voe_ui_slider_result spread;
		long to;

		if (sculpt->spread_sliders[i][s] == VOE_UI_NODE_NONE)
			continue;
		spread = voe_ui_slider_action(ui, sculpt->spread_sliders[i][s],
					      0.0, SPREAD_MAX[s]);
		sculpt->look_held = sculpt->look_held || spread.held;
		if (!spread.changed)
			continue;
		to = lround(spread.value);
		*spread_of(look_ask(sculpt, i), s) =
			(uint8_t)(to < 0 ? 0 : to > SPREAD_MAX[s] ? SPREAD_MAX[s] : to);
	}
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

	if (sculpt->foliage_sliders[0] != VOE_UI_NODE_NONE) {
		const float value =
			(float)voe_ui_slider_action(ui, sculpt->foliage_sliders[0],
						    VOE_EDITOR_SCULPT_DENSITY_MIN,
						    VOE_EDITOR_SCULPT_DENSITY_MAX)
				.value;

		sculpt->density = value < VOE_EDITOR_SCULPT_DENSITY_MIN ?
					  VOE_EDITOR_SCULPT_DENSITY_MIN :
				  value > VOE_EDITOR_SCULPT_DENSITY_MAX ?
					  VOE_EDITOR_SCULPT_DENSITY_MAX :
					  value;
	}

	for (uint32_t i = 0; i < sculpt->kinds_drawn; i++) {
		kind_size_read(ui, sculpt, i);
		kind_spreads_read(ui, sculpt, i);
	}
}
