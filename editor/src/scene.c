// The selection, and the rows the Scene panel drew. See the header for why
// building a project's entities is not this file's job, and why the rows
// outlive the call that drew them.
//
// NOTHING IN HERE DRAWS AND NOTHING IN HERE LAYS ANYTHING OUT. It holds `ui`
// nodes because that is what a widget answers through, and it asks `ui` exactly
// one question — what did the pointer do to this button — after the frame has
// ended. What an Add choice makes, and what Delete and Duplicate queue, is
// entities.h's.
#include "scene.h"

#include "entities.h"

#include <base/assert.h>

#include <ui/widgets.h>

voe_ecs_entity voe_editor_scene_selected(const voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "asking what no scene has selected");
	VOE_BASE_ASSERT(scene->world != NULL,
			"asking what a scene with no world has selected");

	if (!voe_ecs_entity_alive(scene->world, scene->selected))
		return (voe_ecs_entity){ 0 };

	return scene->selected;
}

bool voe_editor_scene_is_selected(const voe_editor_scene *scene,
				  voe_ecs_entity entity)
{
	voe_ecs_entity selected = voe_editor_scene_selected(scene);

	return selected.generation == entity.generation &&
	       selected.index == entity.index &&
	       selected.generation != 0;
}

void voe_editor_scene_rows_clear(voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "clearing the rows of no scene");

	scene->listed_count = 0;
	scene->add = VOE_UI_NODE_NONE;
	for (uint32_t i = 0; i < VOE_EDITOR_SCENE_ADD_CHOICES; i++)
		scene->add_choices[i] = VOE_UI_NODE_NONE;
	scene->structural = 0;
	scene->full = false;
}

void voe_editor_scene_add_menu_record(voe_editor_scene *scene, voe_ui_node add,
				      const voe_ui_node *choices)
{
	VOE_BASE_ASSERT(scene != NULL, "recording the Add menu on no scene");

	scene->add = add;
	if (choices != NULL)
		for (uint32_t i = 0; i < VOE_EDITOR_SCENE_ADD_CHOICES; i++)
			scene->add_choices[i] = choices[i];
}

// What the pointer did to a recorded button, or nothing for one the frame had no
// room for — see voe_editor_scene_clicks_read on why that is skipped.
static voe_ui_action action_of(const voe_ui_context *ui, voe_ui_node node)
{
	if (node == VOE_UI_NODE_NONE)
		return (voe_ui_action){ 0 };

	return voe_ui_button_action(ui, node);
}

void voe_editor_scene_row_add(voe_editor_scene *scene, voe_ui_node node,
			      voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(scene != NULL, "recording a row on no scene");

	if (scene->listed_count == VOE_EDITOR_SCENE_ROWS)
		return;

	scene->listed[scene->listed_count++] =
		(voe_editor_scene_row){ .node = node, .entity = entity };
}

bool voe_editor_scene_clicks_read(voe_editor_scene *scene,
				  const voe_ui_context *ui, bool down)
{
	bool pressed = down && !scene->pointer_was_down;
	bool on_menu;
	bool added = true;

	VOE_BASE_ASSERT(scene != NULL, "reading the clicks of no scene");
	VOE_BASE_ASSERT(ui != NULL, "reading clicks out of no interface");

	// A refused frame hands back VOE_UI_NODE_NONE for every widget past the
	// node budget, and asking one of those what the pointer did is the
	// caller's bug — so they are skipped rather than asserted on, because a
	// full frame is `ui`'s to report and not this file's to fail on.
	for (uint32_t i = 0; i < scene->listed_count; i++) {
		if (scene->listed[i].node == VOE_UI_NODE_NONE)
			continue;
		if (voe_ui_button_action(ui, scene->listed[i].node).fired)
			scene->selected = scene->listed[i].entity;
	}

	scene->pointer_was_down = down;

	// A press that armed Add or a choice is the menu's own; any other hides
	// the choices, on the press and not the release.
	on_menu = action_of(ui, scene->add).held;
	for (uint32_t i = 0; i < VOE_EDITOR_SCENE_ADD_CHOICES; i++)
		on_menu = on_menu || action_of(ui, scene->add_choices[i]).held;
	if (pressed && !on_menu)
		scene->adding = false;

	if (action_of(ui, scene->add).fired)
		scene->adding = !scene->adding;

	for (uint32_t i = 0; i < VOE_EDITOR_SCENE_ADD_CHOICES; i++) {
		voe_ecs_entity made;

		if (!action_of(ui, scene->add_choices[i]).fired)
			continue;
		scene->adding = false;
		added = voe_editor_entities_add(scene->world, (voe_editor_add)i,
						&made);
		if (added) {
			scene->selected = made;
			scene->structural++;
		}
	}

	return added;
}

void voe_editor_scene_delete(voe_editor_scene *scene)
{
	voe_ecs_entity selected = voe_editor_scene_selected(scene);

	if (selected.generation == 0)
		return;
	if (!voe_editor_entities_delete(scene->world, selected)) {
		scene->full = true;
		return;
	}
	scene->selected = (voe_ecs_entity){ 0 };
	scene->structural++;
}

void voe_editor_scene_duplicate(voe_editor_scene *scene)
{
	voe_ecs_entity selected = voe_editor_scene_selected(scene);
	voe_ecs_entity made;

	if (selected.generation == 0)
		return;
	if (!voe_editor_entities_duplicate(scene->world, selected, &made)) {
		scene->full = true;
		return;
	}
	scene->selected = made;
	scene->structural++;
}
