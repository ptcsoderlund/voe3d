// The Scene list's one frame of `ui` calls: the heading, Add entity and a
// choice per identity row, drawn depth-first and indented by depth, each node
// handed to the scene to be asked after the frame, and the held row's drop
// read after it. See scene_list.h for the order, why the rows are the identity
// table, how they are keyed and what a drop does. The rows sit in one gapless
// column under the rows' spacing.
#include "scene_list.h"

#include "drag_ghost.h"
#include "inspector_place.h"
#include "themes.h"

#include <base/assert.h>
#include <scene/identity_component.h>
#include <scene/parent_component.h>
#include <scene/parent_system.h>
#include <scene/prefab_component.h>

#include <string.h>

// How far each level of the tree is indented, in millimetres.
#define INDENT_PER_DEPTH (4.0f * VOE_EDITOR_SPACING)

// The gap between an entity's name and its prefab's file name, in millimetres.
#define PREFAB_NAME_GAP (2.0f * VOE_EDITOR_SPACING)

// How wide the drop rim round every row and the heading is, in millimetres.
#define RIM_WIDTH 0.5f

// The `spacing` of the theme the entity rows are drawn under, so a row's
// button pad is 0.5 mm, the rim's width, and rows sit a text line apart
// (ADR-0344).
#define LIST_ROW_SPACING 0.8f

// No identity parent: a root.
#define NO_PARENT UINT32_MAX

// A row waiting on the walk's stack, how deep it sits, and whether a folded
// row above it hides it.
struct pending {
	uint32_t index;
	uint32_t depth;
	bool hidden;
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

static bool same_entity(voe_ecs_entity a, voe_ecs_entity b)
{
	return a.index == b.index && a.generation == b.generation;
}

// A part: made from a prefab for a placed copy other than itself (0283 point
// 5). A copy's root names itself, so it is not one.
static bool is_part(const voe_ecs_world *world, voe_ecs_entity entity)
{
	const voe_scene_prefab_part *part =
		voe_scene_prefab_part_get(world, entity);

	return part != NULL && !same_entity(part->instance, entity);
}

// The last name of a prefab's path, pointing into the row, which outlives the
// frame as the identity's name does.
static const char *prefab_file_name(const voe_scene_prefab *prefab)
{
	const char *slash = strrchr(prefab->path, '/');

	return slash != NULL ? slash + 1 : prefab->path;
}

// Opens the keyed wrapper every row and the heading sit in: padded by the
// rim, drawn only when `lit`, then under list_rim for its panel alone. Closed
// by voe_ui_end.
static voe_ui_node rim_begin(voe_ui_context *ui, voe_editor_scene *scene,
			     const char *name, uint32_t index, bool lit)
{
	voe_ui_node rim;

	if (lit)
		voe_ui_theme_push(ui, &scene->list_rim);
	rim = voe_ui_panel_begin(
		ui, name, index, lit ? VOE_UI_SURFACE_RAISED : VOE_UI_SURFACE_NONE,
		(voe_ui_container){ .across = VOE_UI_ACROSS_FILL,
				    .pad = { RIM_WIDTH, RIM_WIDTH, RIM_WIDTH,
					     RIM_WIDTH } });
	if (lit)
		voe_ui_theme_pop(ui);
	VOE_BASE_ASSERT(rim != VOE_UI_NODE_NONE, "a rim with no node");
	return rim;
}

// `children` is whether the row has any, which gives it a fold button keyed
// by the identity index, "+" folded and "-" open.
static void row_draw(voe_ui_context *ui, voe_editor_scene *scene,
		     uint32_t index, uint32_t depth, bool children)
{
	voe_ui_node fold = VOE_UI_NODE_NONE;
	const voe_scene_identity *rows = voe_scene_identity_rows(scene->world);
	const voe_ecs_entity *entities =
		voe_scene_identity_entities(scene->world);
	const bool dim = scene->list_dragging &&
			 same_entity(entities[index], scene->list_held);
	const voe_scene_prefab *prefab =
		voe_scene_prefab_get(scene->world, entities[index]);

	voe_ui_theme_push(ui, dim ? &scene->list_dim : &scene->list_row);
	rim_begin(ui, scene, "row_rim", index,
		  same_entity(entities[index], scene->list_target));
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
	if (children) {
		fold = voe_ui_button_begin(ui, "fold", index);
		voe_ui_label(ui, rows[index].folded ? "+" : "-");
		voe_ui_end(ui);
	}

	// A selection is drawn inverted, as anything held or pressed is
	// (ADR-0194), so the row is a choice and its label plain. Keyed by the
	// identity index, so its highlight follows its entity when the order
	// changes.
	voe_ui_node row = voe_ui_choice_begin(
		ui, "entity", index,
		voe_editor_scene_is_selected(scene, entities[index]));

	if (prefab == NULL) {
		voe_ui_label(ui, rows[index].name);
	} else {
		voe_ui_row_begin(ui, (voe_ui_container){ .gap = PREFAB_NAME_GAP });
		voe_ui_label(ui, rows[index].name);
		voe_ui_label_role(ui, prefab_file_name(prefab),
				  VOE_UI_TEXT_ROLE_SECONDARY);
		voe_ui_end(ui);
	}
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_end(ui);
	voe_ui_theme_pop(ui);

	// The click is answered after voe_ui_frame_end and this function has
	// to have returned by then, so the node is handed to the scene to be
	// asked later. See scene.h.
	voe_editor_scene_row_add(scene, row, entities[index], fold);
}

// Whether any row's parent is `index`.
static bool has_children(const uint32_t *parents, uint32_t count,
			 uint32_t index)
{
	for (uint32_t j = 0; j < count; j++)
		if (parents[j] == index)
			return true;
	return false;
}

// The rows' theme and the drag's two, copies of `palette` rebuilt each frame
// (ADR-0282): rows at LIST_ROW_SPACING; the held row from that, so it keeps
// its height, as a flat control with the dimmest text; the rim is `inverse`
// all through, and its text is `inverse_ink` so the heading's label, which has
// no fill of its own, reads on it.
static void marks_derive(voe_editor_scene *scene, const voe_ui_theme *palette)
{
	scene->list_row = *palette;
	scene->list_row.spacing = LIST_ROW_SPACING;

	scene->list_dim = scene->list_row;
	scene->list_dim.inverse = palette->control;
	scene->list_dim.control_hovered = palette->control;
	scene->list_dim.text_primary = palette->text_disabled;
	scene->list_dim.text_secondary = palette->text_disabled;
	scene->list_dim.inverse_ink = palette->text_disabled;

	scene->list_rim = *palette;
	scene->list_rim.border = palette->inverse;
	scene->list_rim.surface_raised = palette->inverse;
	scene->list_rim.text_primary = palette->inverse_ink;
}

void voe_editor_scene_list_draw(voe_ui_context *ui,
				const voe_ui_theme *palette,
				voe_editor_scene *scene)
{
	const voe_ecs_entity *entities;
	const voe_scene_identity *rows;
	uint32_t parents[VOE_EDITOR_SCENE_ROWS];
	bool listed[VOE_EDITOR_SCENE_ROWS] = { 0 };
	// Each entity has one parent, so each is pushed at most once.
	struct pending stack[VOE_EDITOR_SCENE_ROWS];
	voe_ui_node add;
	uint32_t count;

	VOE_BASE_ASSERT(palette != NULL, "drawing the Scene list with no palette");
	marks_derive(scene, palette);

	// Its label is under list_rim too while lit, as it has no fill.
	scene->heading = rim_begin(ui, scene, "heading_rim", 0,
				   scene->list_target_heading);
	if (scene->list_target_heading)
		voe_ui_theme_push(ui, &scene->list_rim);
	voe_ui_label(ui, "Scene");
	if (scene->list_target_heading)
		voe_ui_theme_pop(ui);
	voe_ui_end(ui);

	add = voe_ui_button_begin(ui, "add", 0);
	voe_ui_label(ui, "Add entity");
	voe_ui_end(ui);
	voe_editor_scene_add_record(scene, add);

	count = voe_scene_identity_count(scene->world);
	entities = voe_scene_identity_entities(scene->world);
	rows = voe_scene_identity_rows(scene->world);
	VOE_BASE_ASSERT(count <= VOE_EDITOR_SCENE_ROWS,
			"more identities than the Scene list has rows");

	for (uint32_t i = 0; i < count; i++)
		parents[i] = parent_index(scene->world, entities, count,
					  entities[i]);

	// One gapless column round both walks, so no panel gap sits between
	// rows.
	voe_ui_column_begin(ui, (voe_ui_container){
					.gap = 0, .across = VOE_UI_ACROSS_FILL });

	for (uint32_t root = 0; root < count; root++) {
		uint32_t top = 0;

		if (parents[root] != NO_PARENT)
			continue;
		stack[top++] = (struct pending){ root, 0, false };
		while (top > 0) {
			const struct pending at = stack[--top];
			const bool hides = at.hidden || rows[at.index].folded;

			// A folded tree is still walked, undrawn, so its rows
			// count as listed and are not listed flat below.
			listed[at.index] = true;
			if (!at.hidden)
				row_draw(ui, scene, at.index, at.depth,
					 has_children(parents, count,
						      at.index));
			if (at.depth + 1 >= VOE_SCENE_PARENT_DEPTH_MAX)
				continue;
			// Pushed last to first, so popped in table order.
			for (uint32_t j = count; j-- > 0;)
				if (parents[j] == at.index)
					stack[top++] = (struct pending){
						j, at.depth + 1, hides
					};
		}
	}

	// What is left sits in a loop, or past the depth cap: listed flat, so
	// no authored entity goes missing.
	for (uint32_t i = 0; i < count; i++)
		if (!listed[i])
			row_draw(ui, scene, i, 0,
				 has_children(parents, count, i));
	voe_ui_end(ui);
}

// Whether `at` is on `node` as drawn, for a node the frame had room for.
static bool over(const voe_ui_context *ui, voe_ui_node node,
		 voe_math_float2 at)
{
	return node != VOE_UI_NODE_NONE &&
	       voe_editor_inspector_rect_contains(voe_ui_node_visible(ui, node),
						  at);
}

// What a release of `held` at `at` would do, the one answer the release and
// the drawn target both use: true with `target` a row's entity to parent onto,
// or zeroed to unparent over the heading; false when it lands nowhere or is
// refused (held dead, onto itself or under it, onto a part or already its
// parent, the heading for a root). Either may lack a transform (0300):
// voe_scene_parent_set keeps a child's world place under a bare parent.
static bool drop_target(const voe_editor_scene *scene,
			const voe_ui_context *ui, voe_ecs_entity held,
			voe_math_float2 at, voe_ecs_entity *target)
{
	const voe_scene_parent *parent;

	VOE_BASE_ASSERT(target != NULL, "a drop target into nowhere");
	// A zeroed entity is never alive, so no drag in flight ends here too.
	if (!voe_ecs_entity_alive(scene->world, held))
		return false;
	parent = voe_scene_parent_get(scene->world, held);
	if (over(ui, scene->heading, at)) {
		*target = (voe_ecs_entity){ 0 };
		return parent != NULL;
	}
	for (uint32_t i = 0; i < scene->listed_count; i++) {
		const voe_ecs_entity row = scene->listed[i].entity;

		if (!over(ui, scene->listed[i].node, at))
			continue;
		// Onto itself or something under it would be a loop.
		if (voe_scene_parent_within(scene->world, row, held))
			return false;
		if (is_part(scene->world, row))
			return false;
		if (parent != NULL && same_entity(parent->parent, row))
			return false;
		*target = row;
		return true;
	}
	return false;
}

// One held frame: the start remembered on a newly held row, the threshold
// crossed unless cancelled, and the target a release now would land on, none
// over the Assets panel; refused when dragging onto no target and not over an
// Assets panel that takes it.
static void drag_follow(voe_editor_scene *scene, const voe_ui_context *ui,
			voe_ecs_entity row, voe_math_float2 at,
			bool over_assets, bool assets_take)
{
	voe_ecs_entity target = { 0 };
	bool lands;

	if (!same_entity(scene->list_held, row)) {
		scene->list_held = row;
		scene->list_from = at;
	}
	if (!scene->list_cancelled &&
	    voe_math_float2_length(voe_math_float2_sub(
		    at, scene->list_from)) >= VOE_EDITOR_SCENE_DRAG_START)
		scene->list_dragging = true;
	lands = scene->list_dragging && !over_assets &&
		drop_target(scene, ui, row, at, &target);
	scene->list_target = lands ? target : (voe_ecs_entity){ 0 };
	scene->list_target_heading =
		lands && same_entity(target, (voe_ecs_entity){ 0 });
	scene->list_refused = scene->list_dragging && !lands &&
			      !(over_assets && assets_take);
	VOE_BASE_ASSERT(!scene->list_dragging || !scene->list_cancelled,
			"a cancelled drag still dragging");
}

void voe_editor_scene_list_drop(voe_editor_scene *scene,
				const voe_ui_context *ui, bool down,
				voe_math_float2 at, bool over_assets,
				bool assets_take)
{
	voe_ecs_entity held;
	voe_ecs_entity target = { 0 };
	bool dropped;
	bool lands;

	VOE_BASE_ASSERT(scene != NULL, "dropping a row on no scene");
	VOE_BASE_ASSERT(ui != NULL, "dropping a row out of no interface");

	for (uint32_t i = 0; i < scene->listed_count; i++) {
		if (scene->listed[i].node == VOE_UI_NODE_NONE)
			continue;
		if (voe_ui_button_action(ui, scene->listed[i].node).held) {
			// A held part is only a click: it selects on release.
			if (!is_part(scene->world, scene->listed[i].entity))
				drag_follow(scene, ui,
					    scene->listed[i].entity, at,
					    over_assets, assets_take);
			return;
		}
	}
	if (down)
		return;

	held = scene->list_held;
	dropped = scene->list_dragging && !scene->list_cancelled;
	lands = dropped && !over_assets &&
		drop_target(scene, ui, held, at, &target);
	if (dropped && over_assets &&
	    voe_ecs_entity_alive(scene->world, held))
		scene->list_made = held;
	scene->list_held = (voe_ecs_entity){ 0 };
	scene->list_from = (voe_math_float2){ 0 };
	scene->list_dragging = false;
	scene->list_cancelled = false;
	scene->list_target = (voe_ecs_entity){ 0 };
	scene->list_target_heading = false;
	scene->list_refused = false;
	if (!lands)
		return;
	if (voe_scene_parent_set(scene->world, held, target))
		scene->structural++;
	else
		scene->full = true;
}

bool voe_editor_scene_list_cancel(voe_editor_scene *scene)
{
	VOE_BASE_ASSERT(scene != NULL, "cancelling a drag on no scene");
	if (!scene->list_dragging)
		return false;
	scene->list_cancelled = true;
	scene->list_dragging = false;
	scene->list_target = (voe_ecs_entity){ 0 };
	scene->list_target_heading = false;
	scene->list_refused = false;
	VOE_BASE_ASSERT(!scene->list_dragging, "a cancelled drag still dragging");
	return true;
}

void voe_editor_scene_list_ghost_draw(voe_ui_context *ui,
				      const voe_editor_scene *scene,
				      voe_math_float2 at)
{
	const voe_scene_identity *identity;

	VOE_BASE_ASSERT(ui != NULL, "drawing a ghost into no interface");
	VOE_BASE_ASSERT(scene != NULL, "drawing a ghost with no scene");
	if (!scene->list_dragging ||
	    !voe_ecs_entity_alive(scene->world, scene->list_held))
		return;
	identity = voe_scene_identity_get(scene->world, scene->list_held);
	if (identity == NULL)
		return;
	voe_editor_drag_ghost_draw(ui, &scene->list_dim, identity->name,
				   scene->list_refused, at);
}
