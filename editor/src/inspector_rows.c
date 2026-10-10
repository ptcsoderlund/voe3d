// The row per described field and the record each control leaves behind for
// inspector_edit.c to read. A field's kind decides which control it gets and its
// offset decides where the bytes go; no table in here maps a component to a
// layout. See inspector_rows.h for how it is called.
//
// A LABEL'S TEXT IS FORMATTED INTO THE FRAME'S ARENA AND NEVER ONTO A STACK.
// voe_ui_label does not copy: the string is read at voe_ui_frame_end, long after
// every function here has returned. `text`, inspector_value.h's, is the only way
// a number reaches the screen from here.
//
// THE THREE ANGLES ARE SHOWN AND NEVER STORED. A rotation is a quaternion in the
// component and after the edit; the Z-Y-X decomposition lives for one frame so a
// person has three numbers to drag, and what the drag submits is the DIFFERENCE
// turned back into a rotation about a world axis. Keeping the angles would be
// two sources of one truth, which disagree the moment anything else writes it.
//
// EVERY ROW WRAPS (ADR-0153 point 11): a narrow Inspector puts a field's boxes
// under its label instead of past the column's edge.
//
// A LIST OF STRINGS IS A FIELD PER STRING. A CHAR field of rank 2 is dims[0]
// strings of size / dims[0] bytes each, and each is a row of its own, labelled
// with the field's title and its 1-based number, its control recording that
// string's offset and size so a commit writes it alone, and the field's name and
// index so the Inspector's drop can find the row under the pointer.
#include "inspector_rows.h"

#include "inspector_value.h"

#include <base/assert.h>

#include <math/float3.h>
#include <math/quat.h>

#include <ui/colour.h>
#include <ui/widgets.h>

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

// What one millimetre of horizontal drag is worth, by what the control writes.
// A real number moves in hundredths, a whole number in halves — so a millimetre
// is not quite a step and the dead zone still decides whether the drag began —
// and an angle in whole degrees.
#define PER_MM_REAL 0.01
#define PER_MM_WHOLE 0.5
#define PER_MM_DEGREE 1.0

#define DEGREES_PER_RADIAN 57.29577951308232

// A colour's swatch, in millimetres: wider than tall, about a line of text high.
#define SWATCH_WIDE 8.0f
#define SWATCH_HIGH 3.0f

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

static void row_begin(voe_ui_context *ui)
{
	voe_ui_row_begin(ui, (voe_ui_container){ .across = VOE_UI_ACROSS_CENTER,
						 .gap = ROW_GAP,
						 .wrap = true });
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

		row_begin(ui);
		voe_ui_label(ui, axis_name(axis));
		number_box(ui, inspector, type, field->name, axis,
			   field->offset, VOE_BASE_FIELD_QUAT, degrees,
			   text(inspector->arena, "%.3f", degrees));
		voe_ui_end(ui);
	}
}

// A rank-2 CHAR field's rows, one per string: a text field keyed by the field's
// name and the string's index, or the string as a label when `shown_only`.
static void string_rows(voe_ui_context *ui, voe_editor_inspector *inspector,
			const voe_ecs_world *world, voe_ecs_type type,
			bool shown_only,
			const voe_base_field_description *field,
			const uint8_t *bytes)
{
	const uint32_t strings = field->dims[0];
	const char *shown_title;
	size_t inner;

	VOE_BASE_ASSERT(field->kind == VOE_BASE_FIELD_CHAR && field->rank == 2 &&
				strings > 0,
			"string rows for a field that is not a list of strings");

	inner = field->size / strings;
	shown_title = title(inspector->arena, field->name);
	for (uint32_t i = 0; i < strings; i++) {
		const size_t at = (size_t)i * inner;
		const char *shown = value_text(inspector->arena, world, field,
					       bytes + at);
		voe_ui_node node;

		row_begin(ui);
		voe_ui_label(ui, text(inspector->arena, "%s %" PRIu32,
				      shown_title, i + 1));
		if (shown_only) {
			voe_ui_label(ui, shown);
			voe_ui_end(ui);
			continue;
		}
		node = voe_ui_field(ui, field->name, i, shown,
				    (voe_ui_sizing){
					    .along = { VOE_UI_SIZE_GROW, 1.0f } });
		voe_ui_end(ui);

		record(inspector, (voe_editor_inspector_control){
					  .node = node,
					  .type = type,
					  .offset = field->offset + at,
					  .writes = VOE_BASE_FIELD_CHAR,
					  .size = inner,
					  .name = field->name,
					  .index = i });
	}
}

// The other kinds' rows, after a colour, a named field, an entity and a list of
// strings have taken theirs: one text field, a rotation, or a row of a button or
// number boxes.
static void control_row(voe_ui_context *ui, voe_editor_inspector *inspector,
			voe_ecs_type type, uint32_t boxes,
			const voe_base_field_description *field,
			const uint8_t *bytes)
{
	if (field->kind == VOE_BASE_FIELD_CHAR) {
		voe_ui_node node;

		row_begin(ui);
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

	row_begin(ui);
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
		// A DOUBLE3 is a world position (ADR-0250): each lane is read
		// and written back as a double, so a typed 100000.001 stays so.
		bool wide = field->kind == VOE_BASE_FIELD_DOUBLE3;

		for (uint32_t lane = 0; lane < boxes; lane++) {
			double value = wide ? real64_at(bytes, lane) :
					      (double)real32_at(bytes, lane);

			number_box(ui, inspector, type, field->name, lane,
				   field->offset +
					   (size_t)lane * (wide ? sizeof(double) :
								  sizeof(float)),
				   wide ? VOE_BASE_FIELD_FLOAT64 :
					  VOE_BASE_FIELD_FLOAT32,
				   value, text(inspector->arena, "%.3f", value));
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

// One field: its name, then whatever it gets. A label when the description says
// read-only, when the kind has no control, or when the component has no intent
// to replace a row through — a dead control is worse than a number.
void voe_editor_inspector_field_row(voe_ui_context *ui,
				    voe_editor_inspector *inspector,
				    const voe_ecs_world *world,
				    voe_ecs_type type, bool editable,
				    const voe_base_struct_description *description,
				    const voe_base_field_description *field,
				    const uint8_t *row)
{
	const uint8_t *bytes = row + field->offset;
	bool char_kind = field->kind == VOE_BASE_FIELD_CHAR;
	bool strings = char_kind && field->rank == 2;
	bool text_box = char_kind && field->rank == 1;
	bool colour = field->kind == VOE_BASE_FIELD_COLOUR;
	const voe_base_field_names *names =
		field->kind == VOE_BASE_FIELD_UINT32 && field->rank == 0
			? voe_base_names_find(description, field->name)
			: NULL;
	bool entity = field->kind == VOE_BASE_FIELD_ENTITY && field->rank == 0;
	uint32_t boxes = strings		      ? field->dims[0]
			 : text_box || colour || entity ? 1
							: lanes(field->kind);
	bool shown_only = !editable || field->read_only || boxes == 0 ||
			  !room(inspector, boxes);
	voe_ui_sizing swatch = { .along = { VOE_UI_SIZE_FIXED, SWATCH_WIDE },
				 .across = { VOE_UI_SIZE_FIXED, SWATCH_HIGH } };
	voe_math_float3 linear;

	VOE_BASE_ASSERT(ui != NULL && inspector != NULL && field != NULL &&
				row != NULL,
			"a field row with nothing to draw it into or from");

	if (strings) {
		string_rows(ui, inspector, world, type, shown_only, field, bytes);
		return;
	}

	// A COLOUR IS A SWATCH AND NEVER THREE NUMBERS (card 16): inside a
	// button when it can be replaced, which opens the picker once the
	// frame has ended (voe_editor_inspector_buttons_read), and bare when
	// it cannot.
	if (colour) {
		voe_ui_node node = VOE_UI_NODE_NONE;

		memcpy(&linear, bytes, sizeof linear);
		row_begin(ui);
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
	// A value no entry names is the number it is. An ENTITY is the same
	// button, shown by the name of what it points at (entity_field.h).
	if (names != NULL || entity) {
		voe_ui_node node = VOE_UI_NODE_NONE;
		uint64_t value = entity ? 0 : whole_unsigned(field->kind, bytes);
		const char *shown =
			names != NULL && value < names->value_count &&
					names->values[value] != NULL
				? text(inspector->arena, "%s",
				       names->values[value])
				: value_text(inspector->arena, world, field,
					     bytes);

		row_begin(ui);
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
						  .writes = entity
								    ? VOE_BASE_FIELD_ENTITY
								    : VOE_BASE_FIELD_UINT32,
						  .names = names });
		return;
	}

	if (shown_only) {
		row_begin(ui);
		voe_ui_label(ui, field->name);
		voe_ui_label(ui, value_text(inspector->arena, world, field,
					    bytes));
		voe_ui_end(ui);
		return;
	}

	control_row(ui, inspector, type, boxes, field, bytes);
}
