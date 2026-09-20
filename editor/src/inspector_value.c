// The reading behind the Inspector: a field's bytes as a number, as three shown
// angles, or as the one string a label is given. See the header for why the two
// halves of the panel share it and why every string goes in the frame's arena.
//
// THE SWITCHES ARE EXHAUSTIVE AND NEVER DEFAULTED, so a kind added to
// base/describe.h is a build error in here rather than a field that quietly
// reads as nothing. A kind asked for something it is not — a signed whole number
// out of a float — is the caller's bug and asserts (rule 13).
#include "inspector_value.h"

#include <base/assert.h>

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// How close to straight up a rotation has to be before its first and third
// angles stop being separable. Beyond it the decomposition puts everything in
// the third, which is the ordinary answer for a degenerate one and costs the
// edit nothing: what is submitted is a difference about a world axis either way.
#define UPRIGHT 0.9999f

// The word a matrix says. A literal, for the reason the header gives.
#define MATRIX_TEXT "matrix"

// A quarter turn, in radians. Written out rather than taken from a header,
// because M_PI is not standard C and -std=c23 does not define it — the same
// reason math/tests/quat.c writes its own out.
#define QUARTER_TURN 1.5707963267948966f

// ------------------------------------------------------------------ text

// Formats into the frame's arena. Measured first and written second, because
// the length of a number is not something this file should be guessing at with
// a fixed buffer — and the arena is what the label is allowed to point into.
const char *text(voe_base_arena *arena, const char *format, ...)
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
const char *chars(voe_base_arena *arena, size_t size, const uint8_t *bytes)
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
const char *heading(voe_base_arena *arena, const voe_ecs_world *world,
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

float real32_at(const uint8_t *bytes, uint32_t lane)
{
	float value;

	memcpy(&value, bytes + (size_t)lane * sizeof value, sizeof value);
	return value;
}

int64_t whole_signed(voe_base_field_kind kind, const uint8_t *bytes)
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

uint64_t whole_unsigned(voe_base_field_kind kind, const uint8_t *bytes)
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

bool is_signed(voe_base_field_kind kind)
{
	return kind == VOE_BASE_FIELD_INT8 || kind == VOE_BASE_FIELD_INT16 ||
	       kind == VOE_BASE_FIELD_INT32 || kind == VOE_BASE_FIELD_INT64;
}

bool is_unsigned(voe_base_field_kind kind)
{
	return kind == VOE_BASE_FIELD_UINT8 || kind == VOE_BASE_FIELD_UINT16 ||
	       kind == VOE_BASE_FIELD_UINT32 || kind == VOE_BASE_FIELD_UINT64;
}

// What a number box is handed for a control that writes this kind at these
// bytes. A whole number reaches the box as the double it is dragged in and goes
// back rounded — see `whole_bytes`.
double dragged(voe_base_field_kind kind, const uint8_t *bytes)
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
voe_math_float3 shown_angles(voe_math_quat q)
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

float angle_of(voe_math_float3 angles, uint32_t axis)
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

voe_math_float3 world_axis(uint32_t axis)
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
const char *axis_name(uint32_t axis)
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
const char *value_text(voe_base_arena *arena,
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

// How many number boxes a kind is worth, and nought for the kinds that are only
// ever a label. A boolean is one thing on the row and it is a button, not a box
// — `field_row` is where the two part company.
uint32_t lanes(voe_base_field_kind kind)
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

bool is_vector(voe_base_field_kind kind)
{
	return kind == VOE_BASE_FIELD_FLOAT2 || kind == VOE_BASE_FIELD_FLOAT3 ||
	       kind == VOE_BASE_FIELD_FLOAT4;
}
