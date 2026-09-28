// The Scene list's one frame of `ui` calls: the heading, Add entity and a
// choice per identity row, drawn depth-first and indented by depth, each node
// handed to the scene to be asked after the frame. See scene_list.h for the
// order, why the rows are the identity table and how they are keyed.
#include "scene_list.h"

#include <base/assert.h>
#include <scene/identity_component.h>
#include <scene/parent_component.h>

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
