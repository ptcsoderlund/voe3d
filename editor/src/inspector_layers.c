// The Layers section's one frame of `ui` calls and the read of its rows
// afterwards. See the header for what a press and a drop do and why the nodes
// live in the sculpt state.
//
// Row i is layer i, so a row's index is the layer a press paints and a drop
// replaces; the add row sits at the grid's layer count, the index a drop there
// adds at.
#include "inspector_layers.h"

#include "themes.h"

#include <3d/model_component.h>

#include <base/assert.h>

#include <platform/path.h>

#include <ui/widgets.h>

#include <string.h>

// As inspector_sculpt.c's section: inside its edges, between its rows.
// Millimetres.
#define SECTION_PAD (2.0f * VOE_EDITOR_SPACING)
#define SECTION_GAP (1.5f * VOE_EDITOR_SPACING)

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
	sculpt->layers_drawn = 0;
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
		sculpt->layer_rows[i] = voe_ui_choice_begin(
			ui, "layer", i,
			sculpt->chosen && sculpt->kind == VOE_3D_BRUSH_PAINT &&
				sculpt->layer == i);
		voe_ui_label(ui, voe_platform_path_name(land->layers[i]));
		voe_ui_end(ui);
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

void voe_editor_inspector_layers_read(const voe_ui_context *ui,
				      voe_editor_sculpt *sculpt)
{
	VOE_BASE_ASSERT(ui != NULL, "reading a Layers section of no interface");
	VOE_BASE_ASSERT(sculpt != NULL, "reading no Layers section");
	VOE_BASE_ASSERT(sculpt->layers_drawn <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"more layer rows drawn than a grid holds");

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
