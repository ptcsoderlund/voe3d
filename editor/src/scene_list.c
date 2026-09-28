// The Scene list's one frame of `ui` calls: the heading, Add entity and a
// choice per identity row, drawn depth-first and indented by depth, each node
// handed to the scene to be asked after the frame, and the held row's drop
// read after it. See scene_list.h for the order, why the rows are the identity
// table, how they are keyed and what a drop does.
#include "scene_list.h"

#include "inspector_place.h"

#include <base/assert.h>
#include <scene/identity_component.h>
#include <scene/parent_component.h>
#include <scene/parent_system.h>
#include <scene/transform_component.h>

// How far each level of the tree is indented, in millimetres.
#define INDENT_PER_DEPTH 4.0f

// No identity parent: a root.
#define NO_PARENT UINT32_MAX

// A row waiting on the walk's stack, and how deep it sits.
struct pending {
	uint32_t index;
	uint32_t depth;
};

// The identity-table index of the row `entity`'s parent is, or NO_PARENT when
// it has none, or its parent is dead or has no identity.
static uint32_t parent_index(const voe_ecs_world *world,
			     const voe_ecs_entity *entities, uint32_t count,
			     voe_ecs_entity entity)
{
	const voe_scene_parent *parent = voe_scene_parent_get(world, entity);

	if (parent == NULL || !voe_ecs_entity_alive(world, parent->parent))
		return NO_PARENT;
	for (uint32_t i = 0; i < count; i++)
		if (entities[i].index == parent->parent.index &&
		    entities[i].generation == parent->parent.generation)
			return i;
	return NO_PARENT;
}

static void row_draw(voe_ui_context *ui, voe_editor_scene *scene,
		     uint32_t index, uint32_t depth)
{
	const voe_scene_identity *rows = voe_scene_identity_rows(scene->world);
	const voe_ecs_entity *entities =
		voe_scene_identity_entities(scene->world);

	voe_ui_row_begin(ui, (voe_ui_container){ 0 });
	// A fixed-size box is the spacer (ui/layout.h has no margin).
	if (depth > 0) {
		voe_ui_row_begin(
			ui, (voe_ui_container){
				    .size = { .along = { VOE_UI_SIZE_FIXED,
							 INDENT_PER_DEPTH *
								 (float)depth } } });
		voe_ui_end(ui);
	}

	// A selection is drawn inverted, as anything held or pressed is
	// (ADR-0194), so the row is a choice and its label plain. Keyed by the
	// identity index, so its highlight follows its entity when the order
	// changes.
	voe_ui_node row = voe_ui_choice_begin(
		ui, "entity", index,
		voe_editor_scene_is_selected(scene, entities[index]));

	voe_ui_label(ui, rows[index].name);
	voe_ui_end(ui);
	voe_ui_end(ui);

	// The click is answered after voe_ui_frame_end and this function has
	// to have returned by then, so the node is handed to the scene to be
	// asked later. See scene.h.
	voe_editor_scene_row_add(scene, row, entities[index]);
}

void voe_editor_scene_list_draw(voe_ui_context *ui, voe_editor_scene *scene)
{
	const voe_ecs_entity *entities;
	uint32_t parents[VOE_EDITOR_SCENE_ROWS];
	bool listed[VOE_EDITOR_SCENE_ROWS] = { 0 };
	// Each entity has one parent, so each is pushed at most once.
	struct pending stack[VOE_EDITOR_SCENE_ROWS];
	voe_ui_node add;
	uint32_t count;

	scene->heading = voe_ui_label(ui, "Scene");

	add = voe_ui_button_begin(ui, "add", 0);
	voe_ui_label(ui, "Add entity");
	voe_ui_end(ui);
	voe_editor_scene_add_record(scene, add);

	count = voe_scene_identity_count(scene->world);
	entities = voe_scene_identity_entities(scene->world);
	VOE_BASE_ASSERT(count <= VOE_EDITOR_SCENE_ROWS,
			"more identities than the Scene list has rows");

	for (uint32_t i = 0; i < count; i++)
		parents[i] = parent_index(scene->world, entities, count,
					  entities[i]);

	for (uint32_t root = 0; root < count; root++) {
		uint32_t top = 0;

		if (parents[root] != NO_PARENT)
			continue;
		stack[top++] = (struct pending){ root, 0 };
		while (top > 0) {
			uint32_t index = stack[--top].index;
			uint32_t depth = stack[top].depth;

			listed[index] = true;
			row_draw(ui, scene, index, depth);
			if (depth + 1 >= VOE_SCENE_PARENT_DEPTH_MAX)
				continue;
			// Pushed last to first, so popped in table order.
			for (uint32_t j = count; j-- > 0;)
				if (parents[j] == index)
					stack[top++] =
						(struct pending){ j, depth + 1 };
		}
	}

	// What is left sits in a loop, or past the depth cap: listed flat, so
	// no authored entity goes missing.
	for (uint32_t i = 0; i < count; i++)
		if (!listed[i])
			row_draw(ui, scene, i, 0);
}

static bool same_entity(voe_ecs_entity a, voe_ecs_entity b)
{
	return a.index == b.index && a.generation == b.generation;
}

// Whether `at` is on `node` as drawn, for a node the frame had room for.
static bool over(const voe_ui_context *ui, voe_ui_node node,
		 voe_math_float2 at)
{
	return node != VOE_UI_NODE_NONE &&
	       voe_editor_inspector_rect_contains(voe_ui_node_visible(ui, node),
						  at);
}

// The entity the drop lands `held` under: a row's, zeroed for the heading,
// or false when it lands nowhere or changes nothing.
static bool drop_target(const voe_editor_scene *scene,
			const voe_ui_context *ui, voe_ecs_entity held,
			voe_math_float2 at, voe_ecs_entity *target)
{
	const voe_scene_parent *parent =
		voe_scene_parent_get(scene->world, held);

	if (over(ui, scene->heading, at)) {
		*target = (voe_ecs_entity){ 0 };
		return parent != NULL;
	}
	for (uint32_t i = 0; i < scene->listed_count; i++) {
		const voe_ecs_entity row = scene->listed[i].entity;

		if (!over(ui, scene->listed[i].node, at))
			continue;
		*target = row;
		// Onto itself or something under it would be a loop.
		if (voe_scene_parent_within(scene->world, row, held))
			return false;
		if (voe_scene_transform_get(scene->world, row) == NULL)
			return false;
		return parent == NULL || !same_entity(parent->parent, row);
	}
	return false;
}

void voe_editor_scene_list_drop(voe_editor_scene *scene,
				const voe_ui_context *ui, bool down,
				voe_math_float2 at)
{
	voe_ecs_entity held;
	voe_ecs_entity target;

	VOE_BASE_ASSERT(scene != NULL, "dropping a row on no scene");
	VOE_BASE_ASSERT(ui != NULL, "dropping a row out of no interface");

	for (uint32_t i = 0; i < scene->listed_count; i++) {
		if (scene->listed[i].node == VOE_UI_NODE_NONE)
			continue;
		if (voe_ui_button_action(ui, scene->listed[i].node).held) {
			scene->list_held = scene->listed[i].entity;
			return;
		}
	}
	if (down)
		return;

	held = scene->list_held;
	scene->list_held = (voe_ecs_entity){ 0 };
	// A zeroed entity is never alive, so no drag in flight ends here too.
	if (!voe_ecs_entity_alive(scene->world, held) ||
	    voe_scene_transform_get(scene->world, held) == NULL)
		return;
	if (!drop_target(scene, ui, held, at, &target))
		return;
	if (voe_scene_parent_set(scene->world, held, target))
		scene->structural++;
	else
		scene->full = true;
}
