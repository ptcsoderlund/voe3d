// The Scene list's one frame of `ui` calls: the heading, Add entity and a
// choice per identity row, each node handed to the scene to be asked after the
// frame. See scene_list.h for why the rows are the identity table and how they
// are keyed.
#include "scene_list.h"

#include <scene/identity_component.h>

void voe_editor_scene_list_draw(voe_ui_context *ui, voe_editor_scene *scene)
{
	const voe_scene_identity *rows;
	const voe_ecs_entity *entities;
	voe_ui_node add;
	uint32_t count;

	voe_ui_label(ui, "Scene");

	add = voe_ui_button_begin(ui, "add", 0);
	voe_ui_label(ui, "Add entity");
	voe_ui_end(ui);
	voe_editor_scene_add_record(scene, add);

	count = voe_scene_identity_count(scene->world);
	rows = voe_scene_identity_rows(scene->world);
	entities = voe_scene_identity_entities(scene->world);

	for (uint32_t i = 0; i < count; i++) {
		// A selection is drawn inverted, as anything held or pressed
		// is (ADR-0194), so the row is a choice and its label plain.
		voe_ui_node row = voe_ui_choice_begin(
			ui, "entity", i,
			voe_editor_scene_is_selected(scene, entities[i]));

		voe_ui_label(ui, rows[i].name);
		voe_ui_end(ui);

		// The click is answered after voe_ui_frame_end and this
		// function has to have returned by then, so the node is handed
		// to the scene to be asked later. See scene.h.
		voe_editor_scene_row_add(scene, row, entities[i]);
	}
}
