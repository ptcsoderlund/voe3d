// The LOD section's one frame of `ui` calls. See the header for what each row
// says and why it asks the draws' own function.
//
// A row's LOD is shown counted from one, the full model LOD 1 (0434), so a
// model without levels reads "LOD 1 of 1" and the farthest of six "LOD 6 of 6".
#include "inspector_level.h"

#include "themes.h"

#include <3d/model_component.h>
#include <3d/model_levels.h>

#include <base/assert.h>

#include <scene/transform_component.h>

#include <ui/widgets.h>

#include <stdio.h>
#include <string.h>

// As inspector_layers.c's section: inside its edges, between its rows.
// Millimetres.
#define SECTION_PAD (2.0f * VOE_EDITOR_SPACING)
#define SECTION_GAP (1.5f * VOE_EDITOR_SPACING)

// The loaded model `entity`'s row names, NULL when it has none, its path is
// unterminated, or the entry is not loaded or is a picture, landscape or
// material.
static const voe_3d_model_entry *worn_model(const voe_3d_models *store,
					    const voe_ecs_world *world,
					    voe_ecs_entity entity)
{
	const voe_3d_model *row = voe_3d_model_get(world, entity);
	const voe_3d_model_entry *entry;

	if (row == NULL || memchr(row->path, '\0', VOE_3D_MODEL_PATH) == NULL ||
	    row->path[0] == '\0' || voe_scene_transform_get(world, entity) == NULL)
		return NULL;
	entry = voe_3d_models_find(store, row->path);
	if (entry == NULL || !entry->loaded || entry->picture ||
	    entry->landscape != NULL || entry->material)
		return NULL;
	return entry;
}

void voe_editor_inspector_level_draw(voe_ui_context *ui,
				     voe_editor_inspector_level *level,
				     const voe_3d_models *store,
				     const voe_ecs_world *world,
				     voe_ecs_entity entity,
				     const voe_editor_views *views)
{
	const voe_3d_model_entry *model;
	voe_scene_transform placed;

	VOE_BASE_ASSERT(ui != NULL, "drawing a Level section into no interface");
	VOE_BASE_ASSERT(level != NULL && store != NULL && world != NULL &&
				views != NULL,
			"drawing a Level section with no labels, store, world or views");
	VOE_BASE_ASSERT(views->count <= VOE_EDITOR_VIEWS,
			"more views in use than the editor has room for");

	model = worn_model(store, world, entity);
	if (model == NULL)
		return;
	VOE_BASE_ASSERT(model->level_count >= 1 &&
				model->level_count <= VOE_3D_MODEL_LEVELS,
			"a loaded model with no levels or too many");
	placed = voe_scene_transform_world(world, entity);

	voe_ui_panel_begin(ui, "level", 0, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = SECTION_GAP,
				   .pad = { SECTION_PAD, SECTION_PAD, SECTION_PAD,
					    SECTION_PAD } });
	voe_ui_label(ui, "LOD");

	for (uint32_t v = 0; v < views->count; v++) {
		const voe_editor_view *view = &views->views[v];
		const voe_render_view seen =
			voe_editor_view_pass_camera(view, (voe_render_light){ 0 })
				.view;
		const uint32_t k =
			voe_3d_model_level(model, &placed, view->eye, &seen);
		int written;

		if (model->level_count == 1)
			written = snprintf(level->labels[v],
					   VOE_EDITOR_INSPECTOR_LEVEL_LABEL,
					   "View %u: LOD 1 of 1", v + 1);
		else
			written = snprintf(level->labels[v],
					   VOE_EDITOR_INSPECTOR_LEVEL_LABEL,
					   "View %u: LOD %u of %u, %s", v + 1, k + 1,
					   model->level_count,
					   model->levels_own ? "own" : "made");
		VOE_BASE_ASSERT(written > 0 &&
					written < VOE_EDITOR_INSPECTOR_LEVEL_LABEL,
				"a level label longer than its buffer");
		voe_ui_label(ui, level->labels[v]);
	}

	voe_ui_end(ui);
}
