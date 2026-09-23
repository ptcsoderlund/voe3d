// The replace intent a moved control becomes, and the scene.h call a fired
// button is. See the header for why an edit is never a write and why all of this
// happens after the frame has ended.
//
// THE BYTES ARE BUILT BY WHAT THE CONTROL WRITES AND NOT BY WHAT THE FIELD IS.
// Every function below reads one recorded control, narrows the number the box
// came back with to the width that control writes, and hands the bytes to
// `submit`; the switch over the kinds is exhaustive rather than defaulted, so a
// kind added to base is a build error in here rather than an edit that quietly
// does nothing.
//
// A ROW THAT WENT WHILE THE FRAME WAS IN THE AIR IS NOT AN ERROR. The controls
// were recorded before voe_ui_frame_end and the entity could have been destroyed
// since, so every read asks for the row again and returns quietly when it has
// gone — and a widget the frame refused hands back VOE_UI_NODE_NONE, which is
// skipped for the same reason.
#include "inspector_edit.h"

#include "entities.h"
#include "inspector_value.h"
#include "scene.h"

#include <base/assert.h>
#include <base/report.h>

#include <math/float2.h>
#include <math/float3.h>
#include <math/quat.h>

#include <ui/widgets.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// What a degree of a dragged or typed angle is in radians, the unit the row's
// rotation is in.
#define RADIANS_PER_DEGREE 0.017453292519943295

// ------------------------------------------------------------- the edit

// Puts the new bytes into a copy of the row and submits it as the component's
// replace intent. The entity is at offset zero and the row is wherever the
// declaring folder said (ecs/component.h), so neither this function nor anything
// else in this file names the intent's type.
static void submit(voe_ecs_world *world, voe_ecs_entity entity,
		   const voe_editor_inspector_control *control,
		   const void *bytes, size_t size)
{
	voe_ecs_replace replace = voe_ecs_component_replace(world,
							    control->type);
	const void *row = voe_ecs_component_get(world, control->type, entity);
	uint8_t value[VOE_EDITOR_INSPECTOR_INTENT] = { 0 };

	// The row went while the frame was in the air — an entity destroyed
	// between the draw and the read. Nothing to replace and nothing to say.
	if (!replace.set || row == NULL)
		return;

	VOE_BASE_ASSERT(replace.value_size <= sizeof value,
			"a replace intent wider than VOE_EDITOR_INSPECTOR_INTENT");
	VOE_BASE_ASSERT(control->offset + size <= replace.row_size,
			"a control writing past the end of the row it came from");

	memcpy(value, &entity, sizeof entity);
	memcpy(value + replace.row_offset, row, replace.row_size);
	memcpy(value + replace.row_offset + control->offset, bytes, size);

	// DEVIATION: card 062 "every one of these is VOE_BASE_ERROR", read as not
	// covering this line, which already said `warning` before the card and is not
	// in the folders the card names; keeping the word keeps the output unchanged.
	if (!voe_ecs_intent_submit(world, replace.intent, value))
		VOE_BASE_WARNING("editor",
				 "the replace queue for %s is full; that edit was dropped",
				 voe_ecs_component_key(world, control->type)->name);
}

// The new bytes for one control, and the submit that follows them. `value` is
// the number the box came back with — for a rotation an angle in degrees, and
// for a boolean nothing at all, the button having only fired.
static void apply(voe_ecs_world *world, voe_ecs_entity entity,
		  const voe_editor_inspector_control *control, double value)
{
	const uint8_t *row = voe_ecs_component_get(world, control->type,
						   entity);
	const uint8_t *bytes;

	if (row == NULL)
		return;

	bytes = row + control->offset;

	if (is_signed(control->writes)) {
		int64_t whole = (int64_t)llround(value);

		switch (control->writes) {
		case VOE_BASE_FIELD_INT8: {
			int8_t narrow = (int8_t)whole;

			submit(world, entity, control, &narrow, sizeof narrow);
			return;
		}
		case VOE_BASE_FIELD_INT16: {
			int16_t narrow = (int16_t)whole;

			submit(world, entity, control, &narrow, sizeof narrow);
			return;
		}
		case VOE_BASE_FIELD_INT32: {
			int32_t narrow = (int32_t)whole;

			submit(world, entity, control, &narrow, sizeof narrow);
			return;
		}
		default:
			submit(world, entity, control, &whole, sizeof whole);
			return;
		}
	}

	if (is_unsigned(control->writes)) {
		uint64_t whole = value <= 0.0 ? 0u :
					       (uint64_t)llround(value);

		switch (control->writes) {
		case VOE_BASE_FIELD_UINT8: {
			uint8_t narrow = (uint8_t)whole;

			submit(world, entity, control, &narrow, sizeof narrow);
			return;
		}
		case VOE_BASE_FIELD_UINT16: {
			uint16_t narrow = (uint16_t)whole;

			submit(world, entity, control, &narrow, sizeof narrow);
			return;
		}
		case VOE_BASE_FIELD_UINT32: {
			uint32_t narrow = (uint32_t)whole;

			submit(world, entity, control, &narrow, sizeof narrow);
			return;
		}
		default:
			submit(world, entity, control, &whole, sizeof whole);
			return;
		}
	}

	switch (control->writes) {
	case VOE_BASE_FIELD_FLOAT32: {
		float narrow = (float)value;

		submit(world, entity, control, &narrow, sizeof narrow);
		return;
	}
	case VOE_BASE_FIELD_FLOAT64:
		submit(world, entity, control, &value, sizeof value);
		return;
	case VOE_BASE_FIELD_BOOL: {
		bool flipped = bytes[0] == 0;

		submit(world, entity, control, &flipped, sizeof flipped);
		return;
	}
	case VOE_BASE_FIELD_QUAT: {
		// THE DIFFERENCE, ABOUT A WORLD AXIS, AND NEVER THE ANGLE. What
		// the box came back with less what it was showing, turned into a
		// rotation and composed onto the one in the row — so the three
		// numbers are a reading of the rotation and never a second copy
		// of it.
		voe_math_quat rotation;
		voe_math_quat turned;

		memcpy(&rotation, bytes, sizeof rotation);
		turned = voe_math_quat_mul(
			voe_math_quat_from_axis_angle(
				world_axis(control->axis),
				(float)((value - control->shown) *
					RADIANS_PER_DEGREE)),
			rotation);

		submit(world, entity, control, &turned, sizeof turned);
		return;
	}
	default:
		break;
	}

	VOE_BASE_ASSERT(false, "a control writing a kind no control is drawn for");
}

// A text field's commit, as the row's CHAR bytes: the text truncated to leave
// its terminating zero, the rest zeroed, submitted only when it differs from
// what the row holds now. True when an intent was submitted.
static bool typed(voe_ecs_world *world, voe_ecs_entity entity,
		  const voe_editor_inspector_control *control,
		  voe_ui_field_result result)
{
	const uint8_t *row = voe_ecs_component_get(world, control->type, entity);
	uint8_t value[VOE_EDITOR_INSPECTOR_INTENT] = { 0 };
	const uint8_t *end;
	size_t length;

	if (!result.committed || row == NULL || control->size == 0)
		return false;

	VOE_BASE_ASSERT(control->size <= sizeof value,
			"a text field wider than VOE_EDITOR_INSPECTOR_INTENT");

	length = strlen(result.text);
	if (length > control->size - 1)
		length = control->size - 1;
	memcpy(value, result.text, length);

	// The row's own text ends at its first zero or at its last byte.
	end = memchr(row + control->offset, 0, control->size);
	if ((end != NULL ? (size_t)(end - (row + control->offset)) :
			   control->size) == length &&
	    memcmp(row + control->offset, value, length) == 0)
		return false;

	submit(world, entity, control, value, control->size);
	return true;
}

void voe_editor_inspector_edits_read(voe_editor_inspector *inspector,
				     const voe_ui_context *ui,
				     voe_ecs_world *world)
{
	VOE_BASE_ASSERT(inspector != NULL, "reading the edits of no inspector");
	VOE_BASE_ASSERT(ui != NULL, "reading edits out of no interface");
	VOE_BASE_ASSERT(world != NULL, "reading edits into no world");

	if (!voe_ecs_entity_alive(world, inspector->entity))
		return;

	for (uint32_t i = 0; i < inspector->control_count; i++) {
		const voe_editor_inspector_control *control =
			&inspector->controls[i];

		// A refused frame hands back VOE_UI_NODE_NONE for every widget
		// past the node budget, and asking one of those what the
		// pointer did is the caller's bug — so they are skipped, the
		// same way the Scene panel's rows are: a full frame is `ui`'s
		// to report and not this file's to fail on.
		if (control->node == VOE_UI_NODE_NONE)
			continue;

		if (control->writes == VOE_BASE_FIELD_CHAR) {
			if (typed(world, inspector->entity, control,
				  voe_ui_field_action(ui, control->node)))
				inspector->replaced++;
			continue;
		}

		// A swatch's button opens the picker, which is a button and not
		// an edit — voe_editor_inspector_buttons_read's.
		if (control->writes == VOE_BASE_FIELD_COLOUR)
			continue;

		// A named field's control is that same kind of button: it opens
		// the list and the choice comes back through
		// voe_editor_inspector_named_submit. Asking a button what a
		// number box did asserts (ui/widgets.h).
		if (control->names != NULL)
			continue;

		if (control->writes == VOE_BASE_FIELD_BOOL) {
			if (voe_ui_button_action(ui, control->node).fired) {
				apply(world, inspector->entity, control, 0.0);
				inspector->replaced++;
			}
			continue;
		}

		voe_ui_number_result result = voe_ui_number_action(ui,
								   control->node);

		if (result.changed) {
			apply(world, inspector->entity, control, result.value);
			inspector->replaced++;
		}
	}
}

// What the pointer did to a recorded button, or nothing for one not drawn or
// past a refused frame's node budget, which the controls above skip too.
static voe_ui_action action_of(const voe_ui_context *ui, voe_ui_node node)
{
	if (node == VOE_UI_NODE_NONE)
		return (voe_ui_action){ 0 };

	return voe_ui_button_action(ui, node);
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

// Whether `at` is on `rect`, the two comparisons per axis a press-outside test
// is.
static bool contains(voe_ui_rect rect, voe_math_float2 at)
{
	return at.x >= rect.min.x && at.y >= rect.min.y &&
	       at.x < rect.min.x + rect.size.x &&
	       at.y < rect.min.y + rect.size.y;
}

// A list's top edge and its rows' cap, in the surface's millimetres.
typedef struct {
	float top;
	float cap;
} list_fit;

// The side-and-cap rule (ADR-0200) for the drawn `list` with its `rows` inside
// visible room `w`: its top at `down` when the whole list fits below that,
// else its bottom at `up` when it fits above, else on the roomier side capped,
// never to fewer than `least` millimetres of rows. A dropdown's `down` is its
// button's bottom and `up` its top; a submenu's are its row's top and bottom.
static list_fit side_and_cap(const voe_ui_context *ui, voe_ui_rect w,
			     voe_ui_node list, voe_ui_node rows, float down,
			     float up, float least)
{
	voe_ui_rect p = voe_ui_node_rect(ui, list);
	voe_ui_rect r = voe_ui_node_rect(ui, rows);
	// The panel's own padding and border round its rows, whether or not
	// the rows are capped.
	float chrome = p.size.y - r.size.y;
	// What the whole list would be, uncapped: what the rows wanted, which
	// voe_ui_node_measured reports even while they are capped
	// (ui/layout.h).
	float want = chrome + voe_ui_node_measured(ui, rows).y;
	float below = w.min.y + w.size.y - down;
	float above = up - w.min.y;
	list_fit fit = { .top = down, .cap = 0.0f };

	VOE_BASE_ASSERT(list != VOE_UI_NODE_NONE && rows != VOE_UI_NODE_NONE,
			"fitting a list that was not drawn");

	if (want <= below) {
		// below, the natural side
	} else if (want <= above) {
		fit.top = up - want;
	} else {
		float room = below >= above ? below : above;

		fit.cap = room - chrome;
		// A panel with almost no room either way still shows a row to
		// pick and scroll from.
		if (fit.cap < least)
			fit.cap = least;
		if (below < above)
			fit.top = up - (fit.cap + chrome);
	}
	VOE_BASE_ASSERT(fit.cap >= 0.0f, "a list capped to less than nothing");
	return fit;
}

voe_editor_inspector_place
voe_editor_inspector_overlay_place(const voe_editor_inspector *inspector,
				   const voe_ui_context *ui, voe_ui_node button,
				   voe_ui_node list, voe_ui_node rows)
{
	voe_ui_rect b;
	voe_ui_rect c;
	list_fit fit;

	VOE_BASE_ASSERT(inspector != NULL && ui != NULL,
			"placing a list on no inspector or interface");
	VOE_BASE_ASSERT(button != VOE_UI_NODE_NONE &&
				inspector->content != VOE_UI_NODE_NONE,
			"placing a list under no button or in no column");

	b = voe_ui_node_rect(ui, button);
	c = voe_ui_node_rect(ui, inspector->content);
	fit = (list_fit){ .top = b.min.y + b.size.y, .cap = 0.0f };
	if (list != VOE_UI_NODE_NONE && inspector->area != VOE_UI_NODE_NONE)
		fit = side_and_cap(ui, voe_ui_node_visible(ui, inspector->area),
				   list, rows, b.min.y + b.size.y, b.min.y,
				   b.size.y);

	return (voe_editor_inspector_place){ .left = b.min.x - c.min.x,
					     .top = fit.top - c.min.y,
					     .height = fit.cap };
}

// Where a submenu goes this frame (ADR-0221): its left edge at the right edge
// of `from`, the list it opened from, and its top at `row`'s top; its right
// edge at `from`'s left edge instead when the panel's area has no room for it
// on the right; and vertically side_and_cap with the row as the widget. `list`
// VOE_UI_NODE_NONE, not drawn yet, goes to the right uncapped.
static voe_editor_inspector_place
submenu_place(const voe_editor_inspector *inspector, const voe_ui_context *ui,
	      voe_ui_node from, voe_ui_node row, voe_ui_node list,
	      voe_ui_node rows)
{
	voe_ui_rect f;
	voe_ui_rect r;
	voe_ui_rect c;
	float left;
	list_fit fit;

	VOE_BASE_ASSERT(from != VOE_UI_NODE_NONE && row != VOE_UI_NODE_NONE,
			"placing a submenu beside no list or row");
	VOE_BASE_ASSERT(inspector->content != VOE_UI_NODE_NONE,
			"placing a submenu in no column");

	f = voe_ui_node_rect(ui, from);
	r = voe_ui_node_rect(ui, row);
	c = voe_ui_node_rect(ui, inspector->content);
	left = f.min.x + f.size.x;
	fit = (list_fit){ .top = r.min.y, .cap = 0.0f };
	if (list != VOE_UI_NODE_NONE && inspector->area != VOE_UI_NODE_NONE) {
		voe_ui_rect w = voe_ui_node_visible(ui, inspector->area);
		float width = voe_ui_node_rect(ui, list).size.x;

		if (left + width > w.min.x + w.size.x)
			left = f.min.x - width;
		fit = side_and_cap(ui, w, list, rows, r.min.y,
				   r.min.y + r.size.y, r.size.y);
	}

	return (voe_editor_inspector_place){ .left = left - c.min.x,
					     .top = fit.top - c.min.y,
					     .height = fit.cap };
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
		voe_editor_inspector_named_submit(inspector, scene->world,
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
				      contains(voe_ui_node_visible(ui, panel),
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

			if (control->names == NULL ||
			    !action_of(ui, control->node).fired)
				continue;

			voe_editor_scene_dropdown_open(
				scene,
				(voe_editor_dropdown){
					.entity = inspector->entity,
					.type = control->type,
					.offset = control->offset,
					.names = control->names });
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
			       contains(voe_ui_node_visible(ui,
							    inspector->list),
					at);

		for (uint32_t i = 0; i < inspector->row_count; i++)
			on_list = on_list ||
				  action_of(ui, inspector->rows[i].node).held;
		for (uint32_t i = 0; i < inspector->control_count; i++)
			on_list = on_list ||
				  (inspector->controls[i].names != NULL &&
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

			if (control->names == NULL ||
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
			inspector->open_at[level - 1] = submenu_place(
				inspector, ui, from->panel, row,
				inspector->menu_lists[level].panel,
				inspector->menu_lists[level].area);
		}
	}

	inspector->pointer_was_down = down;
}

void voe_editor_inspector_colour_submit(voe_editor_inspector *inspector,
					voe_ecs_world *world,
					voe_ecs_entity entity,
					voe_ecs_type type, size_t offset,
					voe_math_float3 colour)
{
	voe_editor_inspector_control control = {
		.type = type,
		.offset = offset,
		.writes = VOE_BASE_FIELD_COLOUR
	};

	VOE_BASE_ASSERT(inspector != NULL, "a colour submitted by no inspector");
	VOE_BASE_ASSERT(world != NULL, "a colour submitted into no world");

	if (!voe_ecs_entity_alive(world, entity) ||
	    voe_ecs_component_get(world, type, entity) == NULL)
		return;

	submit(world, entity, &control, &colour, sizeof colour);
	inspector->replaced++;
}

void voe_editor_inspector_named_submit(voe_editor_inspector *inspector,
				       voe_ecs_world *world,
				       voe_ecs_entity entity,
				       voe_ecs_type type, size_t offset,
				       uint32_t value)
{
	voe_editor_inspector_control control = {
		.type = type,
		.offset = offset,
		.writes = VOE_BASE_FIELD_UINT32
	};

	VOE_BASE_ASSERT(inspector != NULL, "a choice submitted by no inspector");
	VOE_BASE_ASSERT(world != NULL, "a choice submitted into no world");

	if (!voe_ecs_entity_alive(world, entity) ||
	    voe_ecs_component_get(world, type, entity) == NULL)
		return;

	submit(world, entity, &control, &value, sizeof value);
	inspector->replaced++;
}
