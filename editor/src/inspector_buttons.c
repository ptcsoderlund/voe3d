// What a fired button does: Duplicate, Delete, Remove, a swatch, a dropdown
// control, a row of the open list and Add component's rows, carried out
// through scene.h and entities.h, and the open lists closed and placed. See
// inspector_edit.h for when it runs and what closes each list.
//
// A BUTTON NEVER WRITES A ROW ITSELF. A structural change is entities.h's and
// is counted on the scene; a choice from the open list is submitted by
// voe_editor_inspector_named_submit or _entity_submit (inspector_edit.c), as a
// replace intent; an overlay opens, closes and is placed through scene.h.
// Where a list goes is inspector_place.h's.
#include "inspector_edit.h"

#include "entities.h"
#include "inspector_place.h"
#include "scene.h"

#include <base/assert.h>

#include <ui/widgets.h>

#include <stdbool.h>
#include <stdint.h>

// What the pointer did to a recorded button, or nothing for one not drawn or
// past a refused frame's node budget, which the controls above skip too.
static voe_ui_action action_of(const voe_ui_context *ui, voe_ui_node node)
{
	if (node == VOE_UI_NODE_NONE)
		return (voe_ui_action){ 0 };

	return voe_ui_button_action(ui, node);
}

// Whether `control` is a dropdown's closed button, a named field's or an
// ENTITY field's, which opens the list and writes nothing itself.
static bool opens_list(const voe_editor_inspector_control *control)
{
	return control->names != NULL ||
	       control->writes == VOE_BASE_FIELD_ENTITY;
}

// One structural change's result, counted the way Delete and Duplicate count
// theirs (scene.h).
static void counted(struct voe_editor_scene *scene, bool done)
{
	if (done)
		scene->structural++;
	else
		scene->full = true;
}

// The button `list` drew for `entry`, VOE_UI_NODE_NONE when it drew none.
static voe_ui_node row_of(const voe_editor_add_menu_list *list, uint32_t entry)
{
	VOE_BASE_ASSERT(list != NULL, "finding a row in no list");
	VOE_BASE_ASSERT(list->row_count <= VOE_EDITOR_ADD_MENU_ENTRIES,
			"a list with more rows than entries");

	for (uint32_t i = 0; i < list->row_count; i++)
		if (list->rows[i].entry == entry)
			return list->rows[i].node;
	return VOE_UI_NODE_NONE;
}

// Opens `group`, a row of the list at `level`, at that level, closing every
// group open at it or deeper. The deeper lists drawn this frame showed other
// groups, so they are forgotten: nothing measures or reads them again.
static void group_open(voe_editor_inspector *inspector, uint32_t level,
		       uint32_t group)
{
	VOE_BASE_ASSERT(level + 1 < VOE_EDITOR_ADD_MENU_DEPTH,
			"a group at the deepest level of Add component");
	VOE_BASE_ASSERT(inspector->menu.entries[group].group,
			"opening an entry that is not a group");

	inspector->open_groups[level] = group;
	inspector->open_at[level] = (voe_editor_inspector_place){ 0 };
	inspector->open_count = level + 1;
	for (uint32_t k = level + 1; k < VOE_EDITOR_ADD_MENU_DEPTH; k++)
		inspector->menu_lists[k] = (voe_editor_add_menu_list){
			.panel = VOE_UI_NODE_NONE, .area = VOE_UI_NODE_NONE
		};
}

void voe_editor_inspector_add_close(voe_editor_inspector *inspector)
{
	VOE_BASE_ASSERT(inspector != NULL, "closing Add component on nothing");

	inspector->adding = false;
	VOE_BASE_ASSERT(!inspector->adding, "Add component stayed open");
}

void voe_editor_inspector_buttons_read(voe_editor_inspector *inspector,
				       const voe_ui_context *ui,
				       struct voe_editor_scene *scene,
				       bool down, voe_math_float2 at)
{
	bool pressed = down && !inspector->pointer_was_down;
	bool on_menu;

	VOE_BASE_ASSERT(inspector != NULL, "reading the buttons of no inspector");
	VOE_BASE_ASSERT(ui != NULL, "reading buttons out of no interface");
	VOE_BASE_ASSERT(scene != NULL, "carrying out a button on no scene");

	// A ROW OF THE OPEN LIST IS THE FIELD'S NEW VALUE AND CLOSES THE LIST,
	// at most one of them firing. It is submitted on this frame's copy of
	// the dropdown and never on the live one, for the reason
	// inspector->entity is read instead of the selection: the choice
	// belongs to the list as it was drawn.
	for (uint32_t i = 0; i < inspector->row_count; i++) {
		if (!action_of(ui, inspector->rows[i].node).fired)
			continue;
		if (inspector->dropdown.entities)
			voe_editor_inspector_entity_submit(
				inspector, scene->world,
				inspector->dropdown.entity,
				inspector->dropdown.type,
				inspector->dropdown.offset,
				inspector->rows[i].entity);
		else
			voe_editor_inspector_named_submit(
				inspector, scene->world,
				inspector->dropdown.entity,
				inspector->dropdown.type,
				inspector->dropdown.offset,
				inspector->rows[i].value);
		voe_editor_scene_dropdown_close(scene);
	}

	if (action_of(ui, inspector->duplicate).fired)
		voe_editor_scene_duplicate(scene);
	if (action_of(ui, inspector->remove).fired)
		voe_editor_scene_delete(scene);

	// ADD COMPONENT'S LIST CLOSES the way the open dropdown's does: on a
	// frame that drew another entity than the one it opened for, and on a
	// press outside its visible rectangle and its button, on the press and
	// not the release. The button toggles it, and opening it closes the
	// dropdown and the picker, only one overlay being open at a time.
	if (inspector->adding &&
	    (inspector->adding_for.index != inspector->entity.index ||
	     inspector->adding_for.generation !=
		     inspector->entity.generation))
		inspector->adding = false;
	on_menu = action_of(ui, inspector->add_component).held;
	for (uint32_t level = 0; level < VOE_EDITOR_ADD_MENU_DEPTH; level++) {
		const voe_ui_node panel = inspector->menu_lists[level].panel;

		on_menu = on_menu || (panel != VOE_UI_NODE_NONE &&
				      voe_editor_inspector_rect_contains(
					      voe_ui_node_visible(ui, panel),
					      at));
	}
	if (pressed && !on_menu)
		inspector->adding = false;
	if (action_of(ui, inspector->add_component).fired) {
		inspector->adding = !inspector->adding;
		inspector->adding_for = inspector->entity;
		inspector->adding_at = (voe_editor_inspector_place){ 0 };
		inspector->open_count = 0;
		if (inspector->adding) {
			voe_editor_scene_dropdown_close(scene);
			voe_editor_scene_picker_close(scene);
		}
	}

	// The entity the buttons were drawn for, when it is no longer alive,
	// has nothing to give or take. It guards these four rather than
	// returning, because a list left open on it is still closed and placed
	// below — that is the frame in which its field went off the panel.
	if (voe_ecs_entity_alive(scene->world, inspector->entity)) {
		// A fired swatch opens the picker on its row's colour, beside
		// the column: Duplicate, the first thing drawn, starts at its
		// left edge.
		for (uint32_t i = 0; i < inspector->control_count; i++) {
			const voe_editor_inspector_control *control =
				&inspector->controls[i];

			if (control->writes != VOE_BASE_FIELD_COLOUR ||
			    !action_of(ui, control->node).fired)
				continue;
			voe_editor_scene_picker_open(
				scene,
				(voe_editor_picking){
					.entity = inspector->entity,
					.type = control->type,
					.offset = control->offset,
					.left = inspector->duplicate !=
							VOE_UI_NODE_NONE
							? voe_ui_node_rect(
								  ui,
								  inspector->duplicate)
								  .min.x
							: 0.0f });
		}

		// A fired dropdown opens its list instead of writing anything.
		// Where it hangs is nought here and measured at the end of
		// this function, on this very frame, so it is drawn under its
		// button from the first frame it shows.
		for (uint32_t i = 0; i < inspector->control_count; i++) {
			const voe_editor_inspector_control *control =
				&inspector->controls[i];

			if (!opens_list(control) ||
			    !action_of(ui, control->node).fired)
				continue;

			voe_editor_scene_dropdown_open(
				scene,
				(voe_editor_dropdown){
					.entity = inspector->entity,
					.type = control->type,
					.offset = control->offset,
					.names = control->names,
					.entities = control->writes ==
						    VOE_BASE_FIELD_ENTITY });
		}

		for (uint32_t i = 0; i < inspector->remove_count; i++)
			if (action_of(ui, inspector->removes[i].node).fired)
				counted(scene,
					voe_editor_entities_component_remove(
						scene->world, inspector->entity,
						inspector->removes[i].type));

		// A type's row fired, at any level, adds it and closes the
		// whole menu; a group's opens its submenu at its level.
		for (uint32_t level = 0; level < VOE_EDITOR_ADD_MENU_DEPTH;
		     level++) {
			const voe_editor_add_menu_list *list =
				&inspector->menu_lists[level];

			for (uint32_t i = 0; i < list->row_count; i++) {
				const voe_editor_add_menu_row *row =
					&list->rows[i];
				const voe_editor_add_menu_entry *entry =
					&inspector->menu.entries[row->entry];

				if (!action_of(ui, row->node).fired)
					continue;
				if (entry->group) {
					group_open(inspector, level,
						   row->entry);
					continue;
				}
				inspector->adding = false;
				counted(scene,
					voe_editor_entities_component_add(
						scene->world, inspector->entity,
						entry->type));
			}
		}
	}

	// A PRESS OUTSIDE THE LIST'S OUTLINE CLOSES IT — the same press edge
	// Add component's list closes on, and read the same way.
	// Everything inside what can be seen of the list's panel is the
	// list's: its rows and a dropdown control answer for themselves, and
	// the gaps between the rows, the padding at its edges and the
	// scrollbar of a capped one are in there too and leave it open
	// (ADR-0199). So does a frame in which it was open and drew no rows
	// close it: its field was not on the panel at all, nothing being
	// selected, another entity being selected, or the component having
	// gone.
	if (inspector->dropdown.open) {
		bool on_list = inspector->list != VOE_UI_NODE_NONE &&
			       voe_editor_inspector_rect_contains(
				       voe_ui_node_visible(ui, inspector->list),
				       at);

		for (uint32_t i = 0; i < inspector->row_count; i++)
			on_list = on_list ||
				  action_of(ui, inspector->rows[i].node).held;
		for (uint32_t i = 0; i < inspector->control_count; i++)
			on_list = on_list ||
				  (opens_list(&inspector->controls[i]) &&
				   action_of(ui, inspector->controls[i].node)
					   .held);
		if (inspector->row_count == 0 || (pressed && !on_list))
			voe_editor_scene_dropdown_close(scene);
	}

	// WHERE THE OPEN LIST SITS IS MEASURED EVERY FRAME IT IS OPEN AND
	// NEVER ONCE WHEN IT OPENED (ADR-0199): its button's left edge and
	// just under its bottom, both taken in the Inspector's content
	// column's space. Button and column are moved by the same scroll
	// offset, so this pair says nothing about scrolling and the list stays
	// under its button however far the panel has gone. The live dropdown,
	// so a list opened just above is placed on the frame it opened. No
	// control of its own is no placement.
	//
	// WHICH SIDE IT OPENS ON AND HOW TALL ITS ROWS MAY BE ARE DECIDED HERE
	// TOO, EVERY FRAME (ADR-0200): below the button when the whole list
	// fits in what is left of the panel's scroll area, above it when it
	// does not but fits there, and on the roomier side capped and
	// scrolling when it fits neither. The default is today's answer —
	// below, uncapped — which is also what a frame that drew no list gets,
	// the frame the button fired on.
	if (scene->dropdown.open && inspector->content != VOE_UI_NODE_NONE) {
		for (uint32_t i = 0; i < inspector->control_count; i++) {
			const voe_editor_inspector_control *control =
				&inspector->controls[i];
			voe_editor_inspector_place place;

			if (!opens_list(control) ||
			    control->node == VOE_UI_NODE_NONE ||
			    control->type.value != scene->dropdown.type.value ||
			    control->offset != scene->dropdown.offset)
				continue;

			place = voe_editor_inspector_overlay_place(
				inspector, ui, control->node, inspector->list,
				inspector->list_rows);
			voe_editor_scene_dropdown_place(scene, place.left,
							place.top,
							place.height);
		}
	}

	// Add component's list is placed by the same rule every frame it is
	// open, under its one button, and each open group's submenu beside
	// its row in the list above it, as far down as those were drawn.
	if (inspector->adding && inspector->content != VOE_UI_NODE_NONE &&
	    inspector->add_component != VOE_UI_NODE_NONE) {
		inspector->adding_at = voe_editor_inspector_overlay_place(
			inspector, ui, inspector->add_component,
			inspector->menu_lists[0].panel,
			inspector->menu_lists[0].area);
		for (uint32_t level = 1; level <= inspector->open_count;
		     level++) {
			const voe_editor_add_menu_list *from =
				&inspector->menu_lists[level - 1];
			const voe_ui_node row = row_of(
				from, inspector->open_groups[level - 1]);

			if (from->panel == VOE_UI_NODE_NONE ||
			    row == VOE_UI_NODE_NONE)
				break;
			inspector->open_at[level - 1] =
				voe_editor_inspector_submenu_place(
					inspector, ui, from->panel, row,
					inspector->menu_lists[level].panel,
					inspector->menu_lists[level].area);
		}
	}

	inspector->pointer_was_down = down;
}
