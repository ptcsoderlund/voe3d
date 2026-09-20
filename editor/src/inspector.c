// The walk over the world's component types, the row per described field, and
// the record each control leaves behind for inspector_edit.c to read. See the
// header for why this file names no component; the replace intent a moved
// control becomes is inspector_edit.c's.
//
// IT IS DRIVEN BY base/describe.h AND BY NOTHING ELSE. A field's kind decides
// which control it gets and a field's offset decides where the bytes go; there
// is no table in here mapping a component to a layout, and adding one would be
// the thing the description exists to make unnecessary. The one switch over the
// kinds is deliberately exhaustive rather than defaulted, so a kind added to
// base is a build error here rather than a field that quietly draws nothing.
//
// A LABEL'S TEXT IS FORMATTED INTO THE FRAME'S ARENA AND NEVER ONTO A STACK.
// voe_ui_label does not copy: the string is read at voe_ui_frame_end, long after
// every function in this file has returned, so a buffer on the stack would be a
// label pointing at whatever the next call put there. `text`, which is
// inspector_value.h's, is the only way a number reaches the screen from here.
//
// THE THREE ANGLES ARE SHOWN AND NEVER STORED. A rotation is a quaternion in the
// component and a quaternion after the edit; the Z-Y-X decomposition exists for
// the length of one frame so that a person has three numbers to drag, and what
// the drag submits is the DIFFERENCE turned back into a rotation about a world
// axis. Keeping the angles instead is the bug this shape exists to prevent — two
// sources of one truth, which disagree the moment anything else writes the
// rotation.
//
// EVERY ROW FILLS THE COLUMN AND WRAPS (ADR-0153 point 11). A component's panel
// stretches its rows to the column's width and each row breaks onto further
// lines, so a narrow Inspector puts a field's number boxes under its label
// instead of past the column's edge. That needs a width from outside — see
// ui/layout.h, A RUN THAT WRAPS — which is what the ACROSS_FILL on the panel
// below and on the scroll area the leaf puts round it are both for. A single box
// wider than the whole column still has its line to itself at its full size, and
// the scroll area is what reaches it.
#include "inspector.h"

#include "inspector_value.h"

#include <base/assert.h>

#include <math/float3.h>
#include <math/quat.h>

#include <ui/colour.h>
#include <ui/widgets.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// A component's fields sit on the theme's RAISED surface, a shade above the
// panel's own SURFACE, so that two components read as two blocks.

// Inside a component's four edges, between its rows, and between the things on
// one row. Millimetres.
#define COMPONENT_PAD 2.0f
#define COMPONENT_GAP 1.5f
#define ROW_GAP 2.0f

// What one millimetre of horizontal drag is worth, by what the control writes.
// A real number moves in hundredths, a whole number in halves — so a millimetre
// is not quite a step and the dead zone still decides whether the drag began —
// and an angle in whole degrees.
#define PER_MM_REAL 0.01
#define PER_MM_WHOLE 0.5
#define PER_MM_DEGREE 1.0

// The word an empty panel says. A literal, because a label's text is read
// after this file has returned and a literal is still there then.
#define NOTHING_TEXT "Nothing selected"

#define DEGREES_PER_RADIAN 57.29577951308232

// A colour's swatch, in millimetres: wider than tall, about a line of text high.
#define SWATCH_WIDE 8.0f
#define SWATCH_HIGH 3.0f

// ---------------------------------------------------------- what it draws

static double per_millimetre(voe_base_field_kind writes)
{
	if (is_signed(writes) || is_unsigned(writes))
		return PER_MM_WHOLE;
	if (writes == VOE_BASE_FIELD_QUAT)
		return PER_MM_DEGREE;

	return PER_MM_REAL;
}

static bool room(const voe_editor_inspector *inspector, uint32_t wanted)
{
	return inspector->control_count + wanted <=
	       VOE_EDITOR_INSPECTOR_CONTROLS;
}

static void record(voe_editor_inspector *inspector,
		   voe_editor_inspector_control control)
{
	VOE_BASE_ASSERT(room(inspector, 1),
			"recording a control the inspector has no room for — field_row asks before it draws");

	inspector->controls[inspector->control_count++] = control;
}

// One number box, its figure inside it, and the record the read finds it again
// by. `name` and `lane` are the key: a field's name cannot repeat inside one
// component, so the pair is unique under the component's own panel.
static void number_box(voe_ui_context *ui, voe_editor_inspector *inspector,
		       voe_ecs_type type, const char *name, uint32_t lane,
		       size_t offset, voe_base_field_kind writes, double value,
		       const char *figure)
{
	voe_ui_node node = voe_ui_number_begin(ui, name, lane, value,
					       per_millimetre(writes));

	voe_ui_label(ui, figure);
	voe_ui_end(ui);

	record(inspector, (voe_editor_inspector_control){
				  .node = node,
				  .type = type,
				  .offset = offset,
				  .writes = writes,
				  .axis = lane,
				  .shown = value });
}

// The three rows a rotation is: the field's name, then an angle in degrees per
// world axis. The angle is this frame's decomposition and the record keeps it,
// because what the read submits is the difference between what the box came
// back with and what it was showing.
static void rotation_rows(voe_ui_context *ui, voe_editor_inspector *inspector,
			  voe_ecs_type type,
			  const voe_base_field_description *field,
			  const uint8_t *bytes)
{
	voe_math_quat rotation;
	voe_math_float3 angles;

	memcpy(&rotation, bytes, sizeof rotation);
	angles = shown_angles(rotation);

	voe_ui_label(ui, field->name);

	for (uint32_t axis = 0; axis < 3; axis++) {
		double degrees = (double)angle_of(angles, axis) *
				 DEGREES_PER_RADIAN;

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, axis_name(axis));
		number_box(ui, inspector, type, field->name, axis,
			   field->offset, VOE_BASE_FIELD_QUAT, degrees,
			   text(inspector->arena, "%.3f", degrees));
		voe_ui_end(ui);
	}
}

// One field: its name, then whatever it gets. A label when the description says
// read-only, when the kind has no control, or when the component has no intent
// to replace a row through — see the header on why a dead control is worse than
// a number. `description` is the walk's, for the names a field's values may have.
static void field_row(voe_ui_context *ui, voe_editor_inspector *inspector,
		      voe_ecs_type type, bool editable,
		      const voe_base_struct_description *description,
		      const voe_base_field_description *field,
		      const uint8_t *row)
{
	const uint8_t *bytes = row + field->offset;
	bool text_box = field->kind == VOE_BASE_FIELD_CHAR && field->rank == 1;
	bool colour = field->kind == VOE_BASE_FIELD_COLOUR;
	const voe_base_field_names *names =
		field->kind == VOE_BASE_FIELD_UINT32 && field->rank == 0
			? voe_base_names_find(description, field->name)
			: NULL;
	uint32_t boxes = text_box || colour ? 1 : lanes(field->kind);
	bool shown_only = !editable || field->read_only || boxes == 0 ||
			  !room(inspector, boxes);
	voe_ui_sizing swatch = { .along = { VOE_UI_SIZE_FIXED, SWATCH_WIDE },
				 .across = { VOE_UI_SIZE_FIXED, SWATCH_HIGH } };
	voe_math_float3 linear;

	// A COLOUR IS A SWATCH AND NEVER THREE NUMBERS (card 16): inside a
	// button when it can be replaced, which opens the picker once the
	// frame has ended (voe_editor_inspector_buttons_read), and bare when
	// it cannot.
	if (colour) {
		voe_ui_node node = VOE_UI_NODE_NONE;

		memcpy(&linear, bytes, sizeof linear);
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		if (!shown_only)
			node = voe_ui_button_begin(ui, field->name, 0);
		voe_ui_swatch(ui, linear, swatch);
		if (!shown_only)
			voe_ui_end(ui);
		voe_ui_end(ui);

		if (!shown_only)
			record(inspector, (voe_editor_inspector_control){
						  .node = node,
						  .type = type,
						  .offset = field->offset,
						  .writes = VOE_BASE_FIELD_COLOUR });
		return;
	}

	// A NAMED FIELD IS A DROPDOWN AND NEVER A NUMBER (ADR-0198): the name
	// of the value it holds, in a button that only opens the list once the
	// frame has ended when it can be replaced, and a label when it cannot.
	// A value no entry names is the number it is.
	if (names != NULL) {
		voe_ui_node node = VOE_UI_NODE_NONE;
		uint64_t value = whole_unsigned(field->kind, bytes);
		const char *shown =
			value < names->value_count &&
					names->values[value] != NULL
				? text(inspector->arena, "%s",
				       names->values[value])
				: value_text(inspector->arena, field, bytes);

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		if (!shown_only)
			node = voe_ui_button_begin(ui, field->name, 0);
		voe_ui_label(ui, shown);
		if (!shown_only)
			voe_ui_end(ui);
		voe_ui_end(ui);

		if (!shown_only)
			record(inspector, (voe_editor_inspector_control){
						  .node = node,
						  .type = type,
						  .offset = field->offset,
						  .writes = VOE_BASE_FIELD_UINT32,
						  .names = names });
		return;
	}

	if (shown_only) {
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		voe_ui_label(ui, value_text(inspector->arena, field, bytes));
		voe_ui_end(ui);
		return;
	}

	if (text_box) {
		voe_ui_node node;

		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		node = voe_ui_field(ui, field->name, 0,
				    chars(inspector->arena, field->size, bytes),
				    (voe_ui_sizing){
					    .along = { VOE_UI_SIZE_GROW, 1.0f } });
		voe_ui_end(ui);

		record(inspector, (voe_editor_inspector_control){
					  .node = node,
					  .type = type,
					  .offset = field->offset,
					  .writes = VOE_BASE_FIELD_CHAR,
					  .size = field->size });
		return;
	}

	if (field->kind == VOE_BASE_FIELD_QUAT) {
		rotation_rows(ui, inspector, type, field, bytes);
		return;
	}

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	voe_ui_label(ui, field->name);

	if (field->kind == VOE_BASE_FIELD_BOOL) {
		voe_ui_node node = voe_ui_button_begin(ui, field->name, 0);

		voe_ui_label(ui, bytes[0] != 0 ? TRUE_TEXT : FALSE_TEXT);
		voe_ui_end(ui);

		record(inspector, (voe_editor_inspector_control){
					  .node = node,
					  .type = type,
					  .offset = field->offset,
					  .writes = VOE_BASE_FIELD_BOOL });
	} else if (is_vector(field->kind)) {
		for (uint32_t lane = 0; lane < boxes; lane++) {
			double value = (double)real32_at(bytes, lane);

			number_box(ui, inspector, type, field->name, lane,
				   field->offset +
					   (size_t)lane * sizeof(float),
				   VOE_BASE_FIELD_FLOAT32, value,
				   text(inspector->arena, "%.3f", value));
		}
	} else {
		double value = dragged(field->kind, bytes);
		const char *figure;

		if (is_signed(field->kind))
			figure = text(inspector->arena, "%" PRId64,
				      whole_signed(field->kind, bytes));
		else if (is_unsigned(field->kind))
			figure = text(inspector->arena, "%" PRIu64,
				      whole_unsigned(field->kind, bytes));
		else
			figure = text(inspector->arena, "%.3f", value);

		number_box(ui, inspector, type, field->name, 0, field->offset,
			   field->kind, value, figure);
	}

	voe_ui_end(ui);
}

// One component: its heading with a Remove button beside it — none for the
// identity — the line saying what it needs when the entity lacks that, and a row
// per described field. A panel and not a column because the key is what makes
// every widget beneath it unique — two components with a field of the same name
// would otherwise be one widget sharing one highlight (ui/widgets.h).
static void component_panel(voe_ui_context *ui,
			    voe_editor_inspector *inspector,
			    voe_ecs_world *world, uint32_t index,
			    voe_ecs_type type, bool removable,
			    const uint8_t *row)
{
	const voe_base_struct_description *description;
	bool editable = voe_ecs_component_replace(world, type).set;
	voe_ecs_type needed;

	voe_ui_panel_begin(ui, "component", index, VOE_UI_SURFACE_RAISED,
			   (voe_ui_container){
				   .across = VOE_UI_ACROSS_FILL,
				   .gap = COMPONENT_GAP,
				   .pad = { COMPONENT_PAD, COMPONENT_PAD,
					    COMPONENT_PAD, COMPONENT_PAD } });

	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
	voe_ui_label(ui, heading(inspector->arena, world, type));
	if (removable &&
	    inspector->remove_count < VOE_EDITOR_INSPECTOR_SECTIONS) {
		voe_ui_node node = voe_ui_button_begin(ui, "remove", 0);

		voe_ui_label(ui, "Remove");
		voe_ui_end(ui);
		inspector->removes[inspector->remove_count++] =
			(voe_editor_inspector_type_button){ .node = node,
							    .type = type };
	}
	voe_ui_end(ui);

	if (voe_ecs_component_needs(world, type, &needed) &&
	    voe_ecs_component_get(world, needed, inspector->entity) == NULL)
		voe_ui_label(ui, text(inspector->arena, "Needs %s",
				      heading(inspector->arena, world,
					      needed)));

	description = voe_ecs_component_description(world, type);
	if (description != NULL)
		for (uint32_t i = 0; i < description->field_count; i++)
			field_row(ui, inspector, type, editable, description,
				  &description->fields[i], row);

	voe_ui_end(ui);
}

// Add component, and while it is choosing one button per described type the
// entity has no row of, each headed as its section would be.
static void add_component(voe_ui_context *ui, voe_editor_inspector *inspector,
			  voe_ecs_world *world)
{
	uint32_t types = voe_ecs_component_type_count(world);

	inspector->add_component = voe_ui_button_begin(ui, "add component", 0);
	voe_ui_label(ui, "Add component");
	voe_ui_end(ui);

	if (!inspector->choosing)
		return;

	for (uint32_t i = 0; i < types; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);
		voe_ui_node node;

		if (voe_ecs_component_runtime_only(world, type) ||
		    voe_ecs_component_get(world, type, inspector->entity) !=
			    NULL ||
		    inspector->choice_count == VOE_EDITOR_INSPECTOR_SECTIONS)
			continue;

		node = voe_ui_button_begin(ui, "component choice", i);
		voe_ui_label(ui, heading(inspector->arena, world, type));
		voe_ui_end(ui);
		inspector->choices[inspector->choice_count++] =
			(voe_editor_inspector_type_button){ .node = node,
							    .type = type };
	}
}

// ----------------------------------------------------------- the surface

void voe_editor_inspector_frame_begin(voe_editor_inspector *inspector,
				      voe_base_arena *arena)
{
	VOE_BASE_ASSERT(inspector != NULL, "opening a frame on no inspector");
	VOE_BASE_ASSERT(arena != NULL, "an inspector frame with no arena");

	inspector->arena = arena;
	inspector->control_count = 0;
	inspector->replaced = 0;
	inspector->entity = (voe_ecs_entity){ 0 };
	inspector->duplicate = VOE_UI_NODE_NONE;
	inspector->remove = VOE_UI_NODE_NONE;
	inspector->remove_count = 0;
	inspector->add_component = VOE_UI_NODE_NONE;
	inspector->choice_count = 0;
}

void voe_editor_inspector_draw(voe_ui_context *ui,
			       voe_editor_inspector *inspector,
			       voe_ecs_world *world, voe_ecs_entity selected,
			       voe_ecs_type identity)
{
	uint32_t types;

	VOE_BASE_ASSERT(ui != NULL, "drawing an inspector into no interface");
	VOE_BASE_ASSERT(inspector != NULL, "drawing no inspector");
	VOE_BASE_ASSERT(inspector->arena != NULL,
			"drawing an inspector before its frame was opened");
	VOE_BASE_ASSERT(world != NULL, "drawing an inspector on no world");

	inspector->entity = selected;

	if (!voe_ecs_entity_alive(world, selected)) {
		voe_ui_label(ui, NOTHING_TEXT);
		return;
	}

	voe_ui_row_begin(ui, (voe_ui_container){ .gap = COMPONENT_GAP });
	inspector->duplicate = voe_ui_button_begin(ui, "duplicate", 0);
	voe_ui_label(ui, "Duplicate");
	voe_ui_end(ui);
	inspector->remove = voe_ui_button_begin(ui, "delete", 0);
	voe_ui_label(ui, "Delete");
	voe_ui_end(ui);
	voe_ui_end(ui);

	// THE WALK, AND THE WHOLE OF WHAT THIS PANEL KNOWS ABOUT COMPONENTS.
	// Every described type the world holds, asked whether this entity has
	// a row of it.
	types = voe_ecs_component_type_count(world);
	for (uint32_t i = 0; i < types; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);
		const void *row = voe_ecs_component_get(world, type, selected);

		if (row == NULL || voe_ecs_component_runtime_only(world, type))
			continue;

		component_panel(ui, inspector, world, i, type,
				type.value != identity.value,
				(const uint8_t *)row);
	}

	add_component(ui, inspector, world);
}
