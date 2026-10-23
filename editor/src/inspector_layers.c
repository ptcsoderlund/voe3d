// The Layers section's one frame of `ui` calls and the read of its rows
// afterwards. See the header for what a press and a drop do and why the nodes
// live in the sculpt state.
//
// Row i is layer i, so a row's index is the layer a press paints and a drop
// replaces; the add row sits at the grid's layer count, the index a drop there
// adds at. A row is drawn chosen and pressed off only under PAINT, so while
// SCATTER or ERASE is chosen none is lit and any fired one chooses PAINT.
//
// The grow rows sit under the lit row, tick and slider k kind k of the layer
// `sculpt->layer` names; a kind's slider is drawn only while it is ticked, so
// a drawn slider is how the read knows the tick was on. The read takes them
// before the rows, while `layer` is still the one they were drawn for.
#include "inspector_layers.h"

#include "themes.h"

#include <3d/model_component.h>

#include <base/assert.h>

#include <platform/path.h>

#include <ui/slider.h>
#include <ui/widgets.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

// As inspector_sculpt.c's section: inside its edges, between its rows.
// Millimetres.
#define SECTION_PAD (2.0f * VOE_EDITOR_SPACING)
#define SECTION_GAP (1.5f * VOE_EDITOR_SPACING)

// As inspector_foliage.c's: between the things on one row, and a slider's
// track; and how far the grow rows sit in from their layer's row.
// Millimetres.
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)
#define SLIDER_WIDE 40.0f
#define GROWS_INDENT (4.0f * VOE_EDITOR_SPACING)

// A Density slider's range in copies per 100 m², from sculpt.h's tenths.
#define GROW_DENSITY_MIN (VOE_EDITOR_SCULPT_GROW_MIN / 10.0)
#define GROW_DENSITY_MAX (VOE_EDITOR_SCULPT_GROW_MAX / 10.0)

voe_editor_inspector_layers_worn voe_editor_inspector_layers_grid(
	const voe_3d_models *store, const voe_ecs_world *world,
	voe_ecs_entity entity)
{
	const voe_editor_inspector_layers_worn none = { 0 };
	const voe_3d_model_entry *entry;
	const char *worn;

	VOE_BASE_ASSERT(store != NULL && world != NULL,
			"a worn grid from no store or world");
	if (!voe_editor_sculpt_wears(world, entity))
		return none;
	worn = voe_3d_model_get(world, entity)->path;
	if (memchr(worn, '\0', VOE_3D_MODEL_PATH) == NULL)
		return none;
	entry = voe_3d_models_find(store, worn);
	if (entry == NULL || !entry->loaded || entry->landscape == NULL)
		return none;
	return (voe_editor_inspector_layers_worn){ .grid = entry->landscape,
						   .path = worn };
}

void voe_editor_inspector_layers_forget(voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(sculpt != NULL, "forgetting no Layers section");

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_LAYER_ROWS; i++)
		sculpt->layer_rows[i] = VOE_UI_NODE_NONE;
	for (uint32_t k = 0; k < VOE_ASSETS_LANDSCAPE_KINDS; k++) {
		sculpt->grow_ticks[k] = VOE_UI_NODE_NONE;
		sculpt->grow_sliders[k] = VOE_UI_NODE_NONE;
	}
	sculpt->layers_drawn = 0;

	VOE_BASE_ASSERT(sculpt->grow_ticks[0] == VOE_UI_NODE_NONE,
			"a Layers section left remembered");
}

// Layer `layer`'s grow rows, indented under its row: "Grows", then per kind
// a tick by its model's file name, chosen while the layer grows it, and for
// a ticked one a row of "Density", its slider in copies per 100 m² and the
// figure; a grid with no kinds says where to add one instead.
static void grows_draw(voe_ui_context *ui, voe_editor_sculpt *sculpt,
		       const voe_assets_landscape *land, uint32_t layer)
{
	VOE_BASE_ASSERT(layer < land->layer_count, "grow rows for no layer");
	VOE_BASE_ASSERT(land->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"a grid with more kinds than it holds");

	voe_ui_column_begin(ui, (voe_ui_container){
					.across = VOE_UI_ACROSS_FILL,
					.gap = SECTION_GAP,
					.pad = { .left = GROWS_INDENT } });
	voe_ui_label(ui, "Grows");
	if (land->kind_count == 0)
		voe_ui_label(ui, "Add a foliage kind below");
	for (uint32_t k = 0; k < land->kind_count; k++) {
		const uint16_t tenths = land->grows[layer][k];

		sculpt->grow_ticks[k] =
			voe_ui_choice_begin(ui, "grow kind", k, tenths > 0);
		voe_ui_label(ui, voe_platform_path_name(land->kinds[k]));
		voe_ui_end(ui);
		if (tenths == 0)
			continue;
		voe_ui_row_begin(ui,
				 (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						     .gap = ROW_GAP,
						     .wrap = true });
		voe_ui_label(ui, "Density");
		sculpt->grow_sliders[k] = voe_ui_slider(
			ui, "grow density", k, tenths / 10.0, GROW_DENSITY_MIN,
			GROW_DENSITY_MAX, SLIDER_WIDE);
		snprintf(sculpt->grow_figures[k], sizeof sculpt->grow_figures[k],
			 "%.1f", tenths / 10.0);
		voe_ui_label(ui, sculpt->grow_figures[k]);
		voe_ui_end(ui);
	}
	voe_ui_end(ui);
}

void voe_editor_inspector_layers_draw(voe_ui_context *ui,
				      voe_editor_sculpt *sculpt,
				      const voe_3d_models *store,
				      const voe_ecs_world *world,
				      voe_ecs_entity entity)
{
	const voe_assets_landscape *land;

	VOE_BASE_ASSERT(ui != NULL, "drawing a Layers section into no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "drawing no Layers section");

	land = voe_editor_inspector_layers_grid(store, world, entity).grid;
	if (land == NULL)
		return;
	VOE_BASE_ASSERT(land->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"a grid with more layers than it holds");

	voe_ui_panel_begin(ui, "layers", 0, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = SECTION_GAP,
				   .pad = { SECTION_PAD, SECTION_PAD, SECTION_PAD,
					    SECTION_PAD } });
	voe_ui_label(ui, "Layers");

	for (uint32_t i = 0; i < land->layer_count; i++) {
		const bool painted = sculpt->chosen &&
				     sculpt->kind == VOE_3D_BRUSH_PAINT &&
				     sculpt->layer == i;

		sculpt->layer_rows[i] =
			voe_ui_choice_begin(ui, "layer", i, painted);
		voe_ui_label(ui, voe_platform_path_name(land->layers[i]));
		voe_ui_end(ui);
		if (painted)
			grows_draw(ui, sculpt, land, i);
	}
	sculpt->layers_drawn = land->layer_count;

	if (land->layer_count < VOE_ASSETS_LANDSCAPE_LAYERS) {
		sculpt->layer_rows[land->layer_count] = voe_ui_row_begin(
			ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER });
		voe_ui_label(ui, "Drop a material to add a layer");
		voe_ui_end(ui);
	}

	voe_ui_end(ui);
}

// Kind `kind`'s growth on the layer the grow rows were drawn for asked, at
// `tenths`, for sculpt.c to settle.
static void grow_ask(voe_editor_sculpt *sculpt, uint32_t kind, long tenths)
{
	VOE_BASE_ASSERT(kind < VOE_ASSETS_LANDSCAPE_KINDS, "asking no kind's growth");
	VOE_BASE_ASSERT(tenths >= 0 && tenths <= VOE_EDITOR_SCULPT_GROW_MAX,
			"a growth asked past its range");

	sculpt->grow_asked = true;
	sculpt->grow_layer = sculpt->layer;
	sculpt->grow_kind = kind;
	sculpt->grow_to = (uint16_t)tenths;
}

// The grow rows: a fired tick asks 0 when drawn ticked, else the default,
// never held, so one step at once; a slider's hold is recorded and a change
// asked in tenths.
static void grows_read(const voe_ui_context *ui, voe_editor_sculpt *sculpt)
{
	for (uint32_t k = 0; k < VOE_ASSETS_LANDSCAPE_KINDS; k++) {
		const bool ticked = sculpt->grow_sliders[k] != VOE_UI_NODE_NONE;
		voe_ui_slider_result density;
		long to;

		if (sculpt->grow_ticks[k] != VOE_UI_NODE_NONE &&
		    voe_ui_button_action(ui, sculpt->grow_ticks[k]).fired)
			grow_ask(sculpt, k, ticked ? 0 : VOE_EDITOR_SCULPT_GROW);
		if (!ticked)
			continue;
		density = voe_ui_slider_action(ui, sculpt->grow_sliders[k],
					       GROW_DENSITY_MIN, GROW_DENSITY_MAX);
		sculpt->grow_held = sculpt->grow_held || density.held;
		if (!density.changed)
			continue;
		to = lround(density.value * 10.0);
		grow_ask(sculpt, k,
			 to < VOE_EDITOR_SCULPT_GROW_MIN ? VOE_EDITOR_SCULPT_GROW_MIN :
			 to > VOE_EDITOR_SCULPT_GROW_MAX ? VOE_EDITOR_SCULPT_GROW_MAX :
							   to);
	}
}

void voe_editor_inspector_layers_read(const voe_ui_context *ui,
				      voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a Layers section of no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "reading no Layers section");
	VOE_BASE_ASSERT(sculpt->layers_drawn <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"more layer rows drawn than a grid holds");

	grows_read(ui, sculpt);

	for (uint32_t i = 0; i < VOE_EDITOR_SCULPT_LAYER_ROWS; i++)
		sculpt->layers_seen[i] =
			sculpt->layer_rows[i] == VOE_UI_NODE_NONE ?
				(voe_ui_rect){ 0 } :
				voe_ui_node_visible(ui, sculpt->layer_rows[i]);

	for (uint32_t i = 0; i < sculpt->layers_drawn; i++) {
		if (sculpt->layer_rows[i] == VOE_UI_NODE_NONE ||
		    !voe_ui_button_action(ui, sculpt->layer_rows[i]).fired)
			continue;
		if (sculpt->chosen && sculpt->kind == VOE_3D_BRUSH_PAINT &&
		    sculpt->layer == i) {
			sculpt->chosen = false;
		} else {
			sculpt->chosen = true;
			sculpt->kind = VOE_3D_BRUSH_PAINT;
			sculpt->layer = i;
		}
	}
}
