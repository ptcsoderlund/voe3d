// The selection, the colour picker's and the open dropdown's targets, the
// gizmo's mode, the open material's close and follow, and the rows the Scene
// panel drew. See the header for why
// building a project's entities is not this file's job, and why the rows
// outlive the call that drew them.
//
// Delete takes a whole tree and Duplicate one entity; both refuse a part and
// the camera, and a light is copied as a second sun (0357 point 5). The colour picker and dropdown
// open, close and are placed where the Inspector measured them. A fold is
// submitted as the entity's identity with `folded` flipped; a reveal's unfold
// as its ancestors' with `folded` false, then its row centred a frame later
// through ui/widgets.h's voe_ui_scroll_centre.
//
// NOTHING IN HERE DRAWS AND NOTHING IN HERE LAYS ANYTHING OUT. It holds `ui`
// nodes because that is what a widget answers through, and after the frame has
// ended it asks `ui` one question — what did the pointer do to this button —
// and makes one request, a reveal's scroll. What Add entity makes, and what Delete and Duplicate queue, is
// entities.h's.
#include "scene.h"

#include "entities.h"

#include <base/assert.h>

#include <scene/camera_component.h>
#include <scene/identity_system.h>
#include <scene/parent_component.h>

#include <ui/widgets.h>

#include <stdio.h>
#include <string.h>

voe_ecs_entity voe_editor_scene_selected(const voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "asking what no scene has selected");
	VOE_BASE_ASSERT(scene->world != NULL,
			"asking what a scene with no world has selected");

	if (!voe_ecs_entity_alive(scene->world, scene->selected))
		return (voe_ecs_entity){ 0 };

	return scene->selected;
}

void voe_editor_scene_select(voe_editor_scene *scene, voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(scene != NULL, "selecting in no scene");
	VOE_BASE_ASSERT(scene->world != NULL,
			"selecting in a scene with no world");

	scene->selected = voe_ecs_entity_alive(scene->world, entity)
				  ? entity
				  : (voe_ecs_entity){ 0 };
	if (scene->selected.generation != 0)
		scene->material_open[0] = '\0';
}

void voe_editor_scene_material_follow(voe_editor_scene *scene,
				      const char *from, const char *to)
{
	static const char assets[] = "Assets/";
	char followed[VOE_ASSETS_MATERIAL_PATH];
	const char *open;
	size_t length;
	int written;

	VOE_BASE_ASSERT(scene != NULL && from != NULL,
			"following a move in no scene or of no path");
	if (scene->material_open[0] == '\0')
		return;
	open = scene->material_open + sizeof assets - 1;
	length = strlen(from);
	VOE_BASE_ASSERT(strncmp(scene->material_open, assets,
				sizeof assets - 1) == 0,
			"an open material outside Assets/");
	// Itself, or under `from/`; a sibling sharing its start is neither.
	if (length == 0 || strncmp(open, from, length) != 0 ||
	    (open[length] != '\0' && open[length] != '/'))
		return;
	written = to == NULL ? -1 :
			       snprintf(followed, sizeof followed, "%s%s%s",
					assets, to, open + length);
	if (written < 0 || (size_t)written >= sizeof followed) {
		scene->material_open[0] = '\0';
		return;
	}
	memcpy(scene->material_open, followed, (size_t)written + 1);
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
	scene->heading = VOE_UI_NODE_NONE;
	scene->list_area = VOE_UI_NODE_NONE;
	scene->structural = 0;
	scene->full = false;
	scene->unfolded = 0;
}

void voe_editor_scene_add_record(voe_editor_scene *scene, voe_ui_node add)
{
	VOE_BASE_ASSERT(scene != NULL, "recording Add entity on no scene");

	scene->add = add;
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
			      voe_ecs_entity entity, voe_ui_node fold)
{
	VOE_BASE_ASSERT(scene != NULL, "recording a row on no scene");

	if (scene->listed_count == VOE_EDITOR_SCENE_ROWS)
		return;

	scene->listed[scene->listed_count++] = (voe_editor_scene_row){
		.node = node, .entity = entity, .fold = fold
	};
}

// A fired fold: the entity's identity row submitted whole with `folded`
// flipped, one structural change so it is an undo step and unsaved.
static void fold_flip(voe_editor_scene *scene, voe_ecs_entity entity)
{
	const voe_scene_identity *identity =
		voe_scene_identity_get(scene->world, entity);
	voe_scene_identity flipped;

	VOE_BASE_ASSERT(scene->world != NULL, "folding a row of no world");
	if (identity == NULL)
		return;
	flipped = *identity;
	flipped.folded = !flipped.folded;
	if (!voe_scene_identity_submit(scene->world,
				       (voe_scene_identity_intent){
					       .entity = entity,
					       .identity = flipped }))
		scene->full = true;
	else
		scene->structural++;
}

bool voe_editor_scene_clicks_read(voe_editor_scene *scene,
				  const voe_ui_context *ui)
{
	voe_ecs_entity made;

	VOE_BASE_ASSERT(scene != NULL, "reading the clicks of no scene");
	VOE_BASE_ASSERT(ui != NULL, "reading clicks out of no interface");

	// A refused frame hands back VOE_UI_NODE_NONE for every widget past the
	// node budget, and asking one of those what the pointer did is the
	// caller's bug — so they are skipped rather than asserted on, because a
	// full frame is `ui`'s to report and not this file's to fail on.
	for (uint32_t i = 0; i < scene->listed_count; i++) {
		if (scene->listed[i].node == VOE_UI_NODE_NONE)
			continue;
		if (voe_ui_button_action(ui, scene->listed[i].node).fired &&
		    !scene->list_dragging && !scene->list_cancelled) {
			scene->selected = scene->listed[i].entity;
			scene->revealed = scene->listed[i].entity;
			scene->material_open[0] = '\0';
		}
		if (action_of(ui, scene->listed[i].fold).fired)
			fold_flip(scene, scene->listed[i].entity);
	}

	if (!action_of(ui, scene->add).fired)
		return true;
	if (!voe_editor_entities_add(scene->world, &made))
		return false;
	scene->selected = made;
	scene->material_open[0] = '\0';
	scene->structural++;
	return true;
}

// Every folded ancestor of `entity` submitted with `folded` false, as
// fold_flip submits; how many were, a refused one setting `full`.
static uint32_t ancestors_unfold(voe_editor_scene *scene, voe_ecs_entity entity)
{
	uint32_t submitted = 0;

	for (uint32_t depth = 0; depth < VOE_SCENE_PARENT_DEPTH_MAX; depth++) {
		const voe_scene_parent *link =
			voe_scene_parent_get(scene->world, entity);
		const voe_scene_identity *identity;
		voe_scene_identity opened;

		if (link == NULL || !voe_ecs_entity_alive(scene->world,
							  link->parent))
			break;
		entity = link->parent;
		identity = voe_scene_identity_get(scene->world, entity);
		if (identity == NULL || !identity->folded)
			continue;
		opened = *identity;
		opened.folded = false;
		if (!voe_scene_identity_submit(scene->world,
					       (voe_scene_identity_intent){
						       .entity = entity,
						       .identity = opened }))
			scene->full = true;
		else
			submitted++;
	}

	VOE_BASE_ASSERT(submitted <= VOE_SCENE_PARENT_DEPTH_MAX,
			"more parents unfolded than a walk follows");
	return submitted;
}

void voe_editor_scene_reveal(voe_editor_scene *scene, voe_ui_context *ui)
{
	voe_ecs_entity selected = voe_editor_scene_selected(scene);
	uint32_t opened;

	VOE_BASE_ASSERT(ui != NULL, "revealing into no interface");

	// A new entity's identity is still queued: its row is drawn a frame on.
	if (selected.generation == 0 ||
	    (selected.index == scene->revealed.index &&
	     selected.generation == scene->revealed.generation) ||
	    voe_scene_identity_get(scene->world, selected) == NULL)
		return;

	// The row is drawn next frame, under its parents opened; scroll then.
	opened = ancestors_unfold(scene, selected);
	scene->unfolded += opened;
	if (opened > 0)
		return;

	// No row drawn (the Scene panel closed) gives the reveal up.
	for (uint32_t i = 0; i < scene->listed_count; i++)
		if (scene->listed[i].entity.index == selected.index &&
		    scene->listed[i].entity.generation == selected.generation &&
		    scene->listed[i].node != VOE_UI_NODE_NONE &&
		    scene->list_area != VOE_UI_NODE_NONE) {
			voe_ui_scroll_centre(ui, scene->list_area,
					     scene->listed[i].node,
					     (voe_ui_scroll_axes){ .y = true });
			break;
		}
	scene->revealed = selected;
}

// The camera anywhere in the selection's tree refuses the whole delete.
static bool tree_holds_camera(const voe_ecs_world *world, voe_ecs_entity root)
{
	voe_ecs_entity tree[VOE_EDITOR_SCENE_ROWS];
	uint32_t count = voe_scene_parent_tree(world, root, tree,
					       VOE_EDITOR_SCENE_ROWS);

	VOE_BASE_ASSERT(count > 0, "a tree without its root");
	for (uint32_t i = 0; i < count; i++)
		if (voe_scene_camera_get(world, tree[i]) != NULL)
			return true;
	return false;
}

void voe_editor_scene_delete(voe_editor_scene *scene)
{
	voe_ecs_entity selected = voe_editor_scene_selected(scene);

	if (selected.generation == 0 ||
	    voe_editor_inspector_is_part(scene->world, selected, NULL) ||
	    tree_holds_camera(scene->world, selected))
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

	if (selected.generation == 0 ||
	    voe_editor_inspector_is_part(scene->world, selected, NULL) ||
	    voe_scene_camera_get(scene->world, selected) != NULL)
		return;
	if (!voe_editor_entities_duplicate(scene->world, selected, &made)) {
		scene->full = true;
		return;
	}
	scene->selected = made;
	scene->structural++;
}

void voe_editor_scene_gizmo_switch(voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "switching the gizmo of no scene");

	scene->rings = !scene->rings;
}

void voe_editor_scene_picker_open(voe_editor_scene *scene,
				  voe_editor_picking picking)
{
	VOE_BASE_ASSERT(scene != NULL, "opening a colour picker on no scene");

	picking.open = true;
	scene->picking = picking;
	scene->dropdown.open = false;
}

void voe_editor_scene_picker_close(voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "closing the colour picker of no scene");

	scene->picking.open = false;
}

bool voe_editor_scene_picker_showing(voe_editor_scene *scene,
				     voe_math_float3 *colour)
{
	const uint8_t *row;

	VOE_BASE_ASSERT(scene != NULL, "asking after the picker of no scene");
	VOE_BASE_ASSERT(colour != NULL,
			"asking the picker's colour with nowhere to put it");

	if (!scene->picking.open)
		return false;

	if (scene->picking.material) {
		if (scene->material_open[0] == '\0') {
			scene->picking.open = false;
			return false;
		}
		*colour = (voe_math_float3){ scene->material.colour[0],
					     scene->material.colour[1],
					     scene->material.colour[2] };
		return true;
	}

	row =voe_editor_scene_is_selected(scene, scene->picking.entity)
		      ? voe_ecs_component_get(scene->world,
					      scene->picking.type,
					      scene->picking.entity)
		      : NULL;
	if (row == NULL) {
		scene->picking.open = false;
		return false;
	}

	memcpy(colour, row + scene->picking.offset, sizeof *colour);
	return true;
}

void voe_editor_scene_dropdown_open(voe_editor_scene *scene,
				    voe_editor_dropdown dropdown)
{
	VOE_BASE_ASSERT(scene != NULL, "opening a dropdown on no scene");

	dropdown.open = true;
	scene->dropdown = dropdown;
	scene->picking.open = false;
}

void voe_editor_scene_dropdown_close(voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "closing the dropdown of no scene");

	scene->dropdown.open = false;
}

void voe_editor_scene_dropdown_place(voe_editor_scene *scene, float left,
				     float top, float height)
{
	VOE_BASE_ASSERT(scene != NULL, "placing the dropdown of no scene");

	if (!scene->dropdown.open)
		return;

	scene->dropdown.left = left;
	scene->dropdown.top = top;
	scene->dropdown.height = height;
}
