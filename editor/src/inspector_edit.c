// The replace intent a moved control becomes, and the picker's colour and the
// open list's choice submitted the same way; what a fired button does is
// inspector_buttons.c's. See the header for why an edit is never a write and
// why all of this happens after the frame has ended.
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

#include "inspector_value.h"

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
