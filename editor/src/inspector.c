// The walk over the world's component types, the row per described field, and
// the replace intent a moved control becomes. See the header for why this file
// names no component and why an edit is never a write.
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
// label pointing at whatever the next call put there. `text` below is the only
// way a number reaches the screen from here.
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

#include "entities.h"
#include "scene.h"

#include <base/assert.h>
#include <base/report.h>

#include <math/float3.h>
#include <math/quat.h>

#include <ui/widgets.h>

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
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

// How close to straight up a rotation has to be before its first and third
// angles stop being separable. Beyond it the decomposition puts everything in
// the third, which is the ordinary answer for a degenerate one and costs the
// edit nothing: what is submitted is a difference about a world axis either way.
#define UPRIGHT 0.9999f

// The words a boolean, a matrix and an empty panel say. Literals, because a
// label's text is read after this file has returned and a literal is still
// there then.
#define TRUE_TEXT "true"
#define FALSE_TEXT "false"
#define MATRIX_TEXT "matrix"
#define NOTHING_TEXT "Nothing selected"

#define DEGREES_PER_RADIAN 57.29577951308232
#define RADIANS_PER_DEGREE 0.017453292519943295

// A quarter turn, in radians. Written out rather than taken from a header,
// because M_PI is not standard C and -std=c23 does not define it — the same
// reason math/tests/quat.c writes its own out.
#define QUARTER_TURN 1.5707963267948966f

// ------------------------------------------------------------------ text

// Formats into the frame's arena. Measured first and written second, because
// the length of a number is not something this file should be guessing at with
// a fixed buffer — and the arena is what the label is allowed to point into.
static const char *text(voe_base_arena *arena, const char *format, ...)
{
	va_list measuring;
	va_list writing;
	int length;
	char *out;

	va_start(measuring, format);
	va_copy(writing, measuring);
	length = vsnprintf(NULL, 0, format, measuring);
	va_end(measuring);

	VOE_BASE_ASSERT(length >= 0, "a label this panel could not format");

	out = voe_base_arena_push(arena, (size_t)length + 1);
	(void)vsnprintf(out, (size_t)length + 1, format, writing);
	va_end(writing);

	return out;
}

// A fixed-size array of bytes, up to the first zero or up to its whole count.
// Copied into the arena so that the terminator exists even when the array is
// full to its last byte.
static const char *chars(voe_base_arena *arena, size_t size, const uint8_t *bytes)
{
	size_t length = 0;
	char *out;

	while (length < size && bytes[length] != 0)
		length++;

	out = voe_base_arena_push(arena, length + 1);
	memcpy(out, bytes, length);
	out[length] = 0;

	return out;
}

// A type's heading: its key name's last `_` word, first letter capitalised, in
// the frame's arena (ADR-0193).
static const char *heading(voe_base_arena *arena, const voe_ecs_world *world,
			   voe_ecs_type type)
{
	const char *name = voe_ecs_component_key(world, type)->name;
	const char *last = strrchr(name, '_');
	size_t length;
	char *out;

	last = last != NULL ? last + 1 : name;
	length = strlen(last);
	out = voe_base_arena_push(arena, length + 1);
	memcpy(out, last, length + 1);
	if (length > 0)
		out[0] = (char)toupper((unsigned char)out[0]);

	return out;
}

// ---------------------------------------------------------------- reading

static float real32_at(const uint8_t *bytes, uint32_t lane)
{
	float value;

	memcpy(&value, bytes + (size_t)lane * sizeof value, sizeof value);
	return value;
}

static int64_t whole_signed(voe_base_field_kind kind, const uint8_t *bytes)
{
	switch (kind) {
	case VOE_BASE_FIELD_INT8: {
		int8_t value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	case VOE_BASE_FIELD_INT16: {
		int16_t value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	case VOE_BASE_FIELD_INT32: {
		int32_t value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	case VOE_BASE_FIELD_INT64: {
		int64_t value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	default:
		break;
	}

	VOE_BASE_ASSERT(false, "reading a signed whole number out of a field that is not one");
	return 0;
}

static uint64_t whole_unsigned(voe_base_field_kind kind, const uint8_t *bytes)
{
	switch (kind) {
	case VOE_BASE_FIELD_UINT8: {
		uint8_t value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	case VOE_BASE_FIELD_UINT16: {
		uint16_t value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	case VOE_BASE_FIELD_UINT32: {
		uint32_t value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	case VOE_BASE_FIELD_UINT64: {
		uint64_t value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	default:
		break;
	}

	VOE_BASE_ASSERT(false, "reading an unsigned whole number out of a field that is not one");
	return 0;
}

static bool is_signed(voe_base_field_kind kind)
{
	return kind == VOE_BASE_FIELD_INT8 || kind == VOE_BASE_FIELD_INT16 ||
	       kind == VOE_BASE_FIELD_INT32 || kind == VOE_BASE_FIELD_INT64;
}

static bool is_unsigned(voe_base_field_kind kind)
{
	return kind == VOE_BASE_FIELD_UINT8 || kind == VOE_BASE_FIELD_UINT16 ||
	       kind == VOE_BASE_FIELD_UINT32 || kind == VOE_BASE_FIELD_UINT64;
}

// What a number box is handed for a control that writes this kind at these
// bytes. A whole number reaches the box as the double it is dragged in and goes
// back rounded — see `whole_bytes`.
static double dragged(voe_base_field_kind kind, const uint8_t *bytes)
{
	if (is_signed(kind))
		return (double)whole_signed(kind, bytes);
	if (is_unsigned(kind))
		return (double)whole_unsigned(kind, bytes);

	switch (kind) {
	case VOE_BASE_FIELD_FLOAT32:
		return (double)real32_at(bytes, 0);
	case VOE_BASE_FIELD_FLOAT64: {
		double value;

		memcpy(&value, bytes, sizeof value);
		return value;
	}
	default:
		break;
	}

	VOE_BASE_ASSERT(false, "dragging a field that is not a number");
	return 0.0;
}

// ------------------------------------------------------------- the angles

// The three angles a rotation is shown as, in radians, as a Z-Y-X
// decomposition: the rotation is Rz then Ry then Rx read right to left, which is
// the composition voe_math_quat_mul spells the same way round. Straight out of
// the matrix the quaternion would become, rather than through float4x4 — this
// file wants three numbers and not a matrix, and building one to read three
// entries out of it is the longer road to the same trigonometry.
static voe_math_float3 shown_angles(voe_math_quat q)
{
	float upward = 2.0f * (q.y * q.w - q.x * q.z);
	voe_math_float3 angles;

	if (upward > UPRIGHT || upward < -UPRIGHT) {
		// Straight up or straight down: the first and third angles turn
		// about the same line and only their sum is a fact. All of it
		// goes in the third, and the first is nought.
		angles.x = 0.0f;
		angles.y = upward > 0.0f ? QUARTER_TURN : -QUARTER_TURN;
		angles.z = -atan2f(2.0f * (q.x * q.y - q.z * q.w),
				   1.0f - 2.0f * (q.x * q.x + q.z * q.z));
		return angles;
	}

	angles.x = atan2f(2.0f * (q.y * q.z + q.x * q.w),
			  1.0f - 2.0f * (q.x * q.x + q.y * q.y));
	angles.y = asinf(upward);
	angles.z = atan2f(2.0f * (q.x * q.y + q.z * q.w),
			  1.0f - 2.0f * (q.y * q.y + q.z * q.z));
	return angles;
}

static float angle_of(voe_math_float3 angles, uint32_t axis)
{
	switch (axis) {
	case 0:
		return angles.x;
	case 1:
		return angles.y;
	default:
		return angles.z;
	}
}

static voe_math_float3 world_axis(uint32_t axis)
{
	switch (axis) {
	case 0:
		return (voe_math_float3){ 1.0f, 0.0f, 0.0f };
	case 1:
		return (voe_math_float3){ 0.0f, 1.0f, 0.0f };
	default:
		return (voe_math_float3){ 0.0f, 0.0f, 1.0f };
	}
}

// The name of one of the three rows. Literals, for the reason every label in
// here is one.
static const char *axis_name(uint32_t axis)
{
	switch (axis) {
	case 0:
		return "x";
	case 1:
		return "y";
	default:
		return "z";
	}
}

// ------------------------------------------------------------ what it says

// The whole field as one string. Every kind reaches this, because a field
// marked read-only is a label whatever it is (ADR-0139 point 2) and so is every
// field of a component nothing can replace.
static const char *value_text(voe_base_arena *arena,
			      const voe_base_field_description *field,
			      const uint8_t *bytes)
{
	switch (field->kind) {
	case VOE_BASE_FIELD_INT8:
	case VOE_BASE_FIELD_INT16:
	case VOE_BASE_FIELD_INT32:
	case VOE_BASE_FIELD_INT64:
		return text(arena, "%" PRId64, whole_signed(field->kind, bytes));
	case VOE_BASE_FIELD_UINT8:
	case VOE_BASE_FIELD_UINT16:
	case VOE_BASE_FIELD_UINT32:
	case VOE_BASE_FIELD_UINT64:
		return text(arena, "%" PRIu64,
			    whole_unsigned(field->kind, bytes));
	case VOE_BASE_FIELD_FLOAT32:
	case VOE_BASE_FIELD_FLOAT64:
		return text(arena, "%.3f", dragged(field->kind, bytes));
	case VOE_BASE_FIELD_BOOL:
		return bytes[0] != 0 ? TRUE_TEXT : FALSE_TEXT;
	case VOE_BASE_FIELD_FLOAT2:
		return text(arena, "%.3f, %.3f", (double)real32_at(bytes, 0),
			    (double)real32_at(bytes, 1));
	case VOE_BASE_FIELD_FLOAT3:
	case VOE_BASE_FIELD_COLOUR:
		return text(arena, "%.3f, %.3f, %.3f",
			    (double)real32_at(bytes, 0),
			    (double)real32_at(bytes, 1),
			    (double)real32_at(bytes, 2));
	case VOE_BASE_FIELD_FLOAT4:
	case VOE_BASE_FIELD_QUAT:
		return text(arena, "%.3f, %.3f, %.3f, %.3f",
			    (double)real32_at(bytes, 0),
			    (double)real32_at(bytes, 1),
			    (double)real32_at(bytes, 2),
			    (double)real32_at(bytes, 3));
	case VOE_BASE_FIELD_FLOAT4X4:
		return MATRIX_TEXT;
	case VOE_BASE_FIELD_ENUM: {
		int32_t value;

		memcpy(&value, bytes, sizeof value);
		return text(arena, "%" PRId32, value);
	}
	case VOE_BASE_FIELD_CHAR:
		return chars(arena, field->size, bytes);
	case VOE_BASE_FIELD_ENTITY: {
		voe_ecs_entity entity;

		memcpy(&entity, bytes, sizeof entity);
		return text(arena, "%" PRIu32 "v%" PRIu32, entity.index,
			    entity.generation);
	}
	}

	VOE_BASE_ASSERT(false, "a described field of no kind at all");
	return "";
}

// ---------------------------------------------------------- what it draws

// How many number boxes a kind is worth, and nought for the kinds that are only
// ever a label. A boolean is one thing on the row and it is a button, not a box
// — `field_row` is where the two part company.
static uint32_t lanes(voe_base_field_kind kind)
{
	switch (kind) {
	case VOE_BASE_FIELD_INT8:
	case VOE_BASE_FIELD_INT16:
	case VOE_BASE_FIELD_INT32:
	case VOE_BASE_FIELD_INT64:
	case VOE_BASE_FIELD_UINT8:
	case VOE_BASE_FIELD_UINT16:
	case VOE_BASE_FIELD_UINT32:
	case VOE_BASE_FIELD_UINT64:
	case VOE_BASE_FIELD_FLOAT32:
	case VOE_BASE_FIELD_FLOAT64:
	case VOE_BASE_FIELD_BOOL:
		return 1;
	case VOE_BASE_FIELD_FLOAT2:
		return 2;
	case VOE_BASE_FIELD_FLOAT3:
	case VOE_BASE_FIELD_COLOUR:
	case VOE_BASE_FIELD_QUAT:
		return 3;
	case VOE_BASE_FIELD_FLOAT4:
		return 4;
	case VOE_BASE_FIELD_FLOAT4X4:
	case VOE_BASE_FIELD_ENUM:
	case VOE_BASE_FIELD_CHAR:
	case VOE_BASE_FIELD_ENTITY:
		return 0;
	}

	VOE_BASE_ASSERT(false, "a described field of no kind at all");
	return 0;
}

static bool is_vector(voe_base_field_kind kind)
{
	return kind == VOE_BASE_FIELD_FLOAT2 || kind == VOE_BASE_FIELD_FLOAT3 ||
	       kind == VOE_BASE_FIELD_FLOAT4;
}

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
// a number.
static void field_row(voe_ui_context *ui, voe_editor_inspector *inspector,
		      voe_ecs_type type, bool editable,
		      const voe_base_field_description *field,
		      const uint8_t *row)
{
	const uint8_t *bytes = row + field->offset;
	uint32_t boxes = lanes(field->kind);

	if (!editable || field->read_only || boxes == 0 ||
	    !room(inspector, boxes)) {
		voe_ui_row_begin(ui, (voe_ui_container){
					     .across = VOE_UI_ACROSS_CENTER,
					     .gap = ROW_GAP,
					     .wrap = true });
		voe_ui_label(ui, field->name);
		voe_ui_label(ui, value_text(inspector->arena, field, bytes));
		voe_ui_end(ui);
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
			field_row(ui, inspector, type, editable,
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

void voe_editor_inspector_buttons_read(voe_editor_inspector *inspector,
				       const voe_ui_context *ui,
				       struct voe_editor_scene *scene,
				       bool down)
{
	bool pressed = down && !inspector->pointer_was_down;
	bool on_menu;

	VOE_BASE_ASSERT(inspector != NULL, "reading the buttons of no inspector");
	VOE_BASE_ASSERT(ui != NULL, "reading buttons out of no interface");
	VOE_BASE_ASSERT(scene != NULL, "carrying out a button on no scene");

	inspector->pointer_was_down = down;

	if (action_of(ui, inspector->duplicate).fired)
		voe_editor_scene_duplicate(scene);
	if (action_of(ui, inspector->remove).fired)
		voe_editor_scene_delete(scene);

	// A press that armed Add component or a choice is the list's own; any
	// other hides it, on the press and not the release — the Add menu's
	// rule (scene.h).
	on_menu = action_of(ui, inspector->add_component).held;
	for (uint32_t i = 0; i < inspector->choice_count; i++)
		on_menu = on_menu ||
			  action_of(ui, inspector->choices[i].node).held;
	if (pressed && !on_menu)
		inspector->choosing = false;
	if (action_of(ui, inspector->add_component).fired)
		inspector->choosing = !inspector->choosing;

	// The entity the buttons were drawn for, when it is no longer alive,
	// has nothing to give or take.
	if (!voe_ecs_entity_alive(scene->world, inspector->entity))
		return;

	for (uint32_t i = 0; i < inspector->remove_count; i++)
		if (action_of(ui, inspector->removes[i].node).fired)
			counted(scene, voe_editor_entities_component_remove(
					       scene->world, inspector->entity,
					       inspector->removes[i].type));

	for (uint32_t i = 0; i < inspector->choice_count; i++) {
		if (!action_of(ui, inspector->choices[i].node).fired)
			continue;
		inspector->choosing = false;
		counted(scene, voe_editor_entities_component_add(
				       scene->world, inspector->entity,
				       inspector->choices[i].type));
	}
}
