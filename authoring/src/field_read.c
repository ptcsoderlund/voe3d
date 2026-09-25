// One field's value read from a scene's text: the implementation behind
// field_read.h. The cursor and its blanks, tokens, brackets, the number
// spellings and the stores, CHAR strings, and a value walked against the
// field's whole shape.
//
// NOTHING RECURSES OVER THE TEXT. A value nests as deep as its field's shape
// says, up to seven of the field's own dimensions and one more for a vector
// kind's own bracket (ADR-0154 points 3 and 5) — shape_value() walks that with
// an explicit stack of one frame per open level, not a call for each, and
// BRACKET_DEPTH_MAX (8) is a hostile file's ceiling regardless of what the
// field declares, checked before anything about the array's shape is (rule
// 14).
//
// THE NUMBERS ARE READ BY THE C LIBRARY IN THE "C" LOCALE, as the writer writes
// them; the spelling is checked by hand first, so strtod never sees `inf`, `nan`,
// a hex float or a leading blank.
#include "field_read.h"

#include <base/assert.h>
#include <base/report.h>

#include <inttypes.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define MODULE "authoring"

// The longest number token a value may spell. The writer never needs more than
// a few dozen; this is room for a person's long decimals and nothing more.
#define NUMBER_MAX 400

// A place in one value's text. A struct and not a `const char **`, which rule 6
// forbids.
struct cursor {
	const char *at;
};

static bool blank(char c)
{
	return c == ' ' || c == '\t';
}

static void skip_blanks(struct cursor *cursor)
{
	while (blank(*cursor->at))
		cursor->at++;
}

// Consumes `c`, and any blanks before it.
static bool expect(struct cursor *cursor, char c)
{
	skip_blanks(cursor);
	if (*cursor->at != c)
		return false;
	cursor->at++;
	return true;
}

static bool ends_token(char c)
{
	return c == '\0' || blank(c) || c == ',' || c == '[' || c == ']';
}

// The bytes up to a blank, a comma, a bracket or the end, copied NUL-terminated.
static bool token(struct cursor *cursor, char out[NUMBER_MAX])
{
	size_t size = 0;

	skip_blanks(cursor);
	while (!ends_token(*cursor->at)) {
		if (size + 1 >= NUMBER_MAX)
			return false;
		out[size++] = *cursor->at++;
	}
	out[size] = '\0';
	return size > 0;
}

static bool digits_only(const char *text)
{
	if (*text == '\0')
		return false;
	for (; *text != '\0'; text++)
		if (*text < '0' || *text > '9')
			return false;
	return true;
}

static bool unsigned_of(const char *text, uint64_t max, uint64_t *out)
{
	uint64_t value = 0;

	if (!digits_only(text))
		return false;
	for (; *text != '\0'; text++) {
		uint64_t digit = (uint64_t)(*text - '0');

		if (value > (UINT64_MAX - digit) / 10)
			return false;
		value = value * 10 + digit;
	}
	if (value > max)
		return false;
	*out = value;
	return true;
}

static bool signed_of(const char *text, int64_t max, int64_t *out)
{
	bool negative = text[0] == '-';
	uint64_t magnitude;

	// Two's complement: the most negative value is one past -max.
	if (!unsigned_of(text + negative, (uint64_t)max + negative, &magnitude))
		return false;
	if (!negative)
		*out = (int64_t)magnitude;
	else if (magnitude == 0)
		*out = 0;
	else
		*out = -(int64_t)(magnitude - 1) - 1;
	return true;
}

// `-? digits (. digits)? ((e|E) (+|-)? digits)?`, and finite once read.
static bool float_of(const char *text, bool single, double *out)
{
	const char *at = text;

	if (*at == '-')
		at++;
	if (*at < '0' || *at > '9')
		return false;
	while (*at >= '0' && *at <= '9')
		at++;
	if (*at == '.') {
		at++;
		if (*at < '0' || *at > '9')
			return false;
		while (*at >= '0' && *at <= '9')
			at++;
	}
	if (*at == 'e' || *at == 'E') {
		at++;
		if (*at == '+' || *at == '-')
			at++;
		if (*at < '0' || *at > '9')
			return false;
		while (*at >= '0' && *at <= '9')
			at++;
	}
	if (*at != '\0')
		return false;

	double value = single ? (double)strtof(text, NULL) : strtod(text, NULL);

	if (!isfinite(value))
		return false;
	*out = value;
	return true;
}

// The most bracket levels a value may nest, a vector kind's own bracket
// counted the same as one the field's shape opens (ADR-0154 point 3).
// Refused, past this, before anything about the array's shape is checked —
// an explicit count kept beside the cursor, not the call stack, because rule
// 14 forbids recursing over data read from a file, and it is at least
// possible to write a value nested past any bound this reader is not one to
// recurse itself to find out (see shape_value() below, which is the one place
// that ever does nest).
#define BRACKET_DEPTH_MAX 8

// Consumes '[', counting it against BRACKET_DEPTH_MAX. `depth` is one call's
// own counter, passed down through every function that may open a bracket —
// the field's own shape and a vector kind's leaf alike — so the two share one
// limit.
static bool open_bracket(struct cursor *cursor, uint32_t *depth,
			 const voe_authoring_site *site)
{
	if (!expect(cursor, '['))
		return false;
	if (*depth >= BRACKET_DEPTH_MAX) {
		VOE_BASE_ERROR(MODULE,
			       "line %u: %s.%s nests more than %d levels of "
			       "brackets; nothing was loaded",
			       site->line, site->section, site->field,
			       BRACKET_DEPTH_MAX);
		return false;
	}
	(*depth)++;
	return true;
}

static bool close_bracket(struct cursor *cursor, uint32_t *depth)
{
	if (!expect(cursor, ']'))
		return false;
	(*depth)--;
	return true;
}

static bool floats_of(struct cursor *cursor, uint8_t *bytes, uint32_t count,
		      uint32_t *depth, const voe_authoring_site *site)
{
	char number[NUMBER_MAX];

	if (!open_bracket(cursor, depth, site))
		return false;
	for (uint32_t i = 0; i < count; i++) {
		double value;

		if (i > 0 && !expect(cursor, ','))
			return false;
		if (!token(cursor, number) || !float_of(number, true, &value))
			return false;

		float single = (float)value;

		memcpy(bytes + i * sizeof(single), &single, sizeof(single));
	}
	return close_bracket(cursor, depth);
}

// Written at `size` bytes, which the field's kind has already checked `max` fits.
static bool store_signed(const char *number, int64_t max, uint8_t *bytes,
			 size_t size)
{
	int64_t value;

	if (!signed_of(number, max, &value))
		return false;
	if (size == 1) {
		int8_t narrow = (int8_t)value;

		memcpy(bytes, &narrow, size);
	} else if (size == 2) {
		int16_t narrow = (int16_t)value;

		memcpy(bytes, &narrow, size);
	} else if (size == 4) {
		int32_t narrow = (int32_t)value;

		memcpy(bytes, &narrow, size);
	} else {
		memcpy(bytes, &value, sizeof(value));
	}
	return true;
}

static bool store_unsigned(const char *number, uint64_t max, uint8_t *bytes,
			   size_t size)
{
	uint64_t value;

	if (!unsigned_of(number, max, &value))
		return false;
	if (size == 1) {
		uint8_t narrow = (uint8_t)value;

		memcpy(bytes, &narrow, size);
	} else if (size == 2) {
		uint16_t narrow = (uint16_t)value;

		memcpy(bytes, &narrow, size);
	} else if (size == 4) {
		uint32_t narrow = (uint32_t)value;

		memcpy(bytes, &narrow, size);
	} else {
		memcpy(bytes, &value, sizeof(value));
	}
	return true;
}

// One element of a field that is neither CHAR nor ENUM. An ENTITY element holds
// nothing yet; its id goes out through `entity_id`, and 0 is no entity. `depth`
// is the bracket-nesting counter a vector kind's own `[` is checked against,
// same as the field's own shape (BRACKET_DEPTH_MAX above).
static bool element_of(struct cursor *cursor, voe_base_field_kind kind,
		       uint8_t *bytes, uint64_t *entity_id, uint32_t *depth,
		       const voe_authoring_site *site)
{
	char number[NUMBER_MAX];

	switch (kind) {
	case VOE_BASE_FIELD_FLOAT2:
		return floats_of(cursor, bytes, 2, depth, site);
	case VOE_BASE_FIELD_FLOAT3:
	case VOE_BASE_FIELD_COLOUR:
		return floats_of(cursor, bytes, 3, depth, site);
	case VOE_BASE_FIELD_FLOAT4:
	case VOE_BASE_FIELD_QUAT:
		return floats_of(cursor, bytes, 4, depth, site);
	case VOE_BASE_FIELD_FLOAT4X4:
		return floats_of(cursor, bytes, 16, depth, site);
	default:
		break;
	}

	if (!token(cursor, number))
		return false;

	switch (kind) {
	case VOE_BASE_FIELD_INT8:
		return store_signed(number, INT8_MAX, bytes, 1);
	case VOE_BASE_FIELD_INT16:
		return store_signed(number, INT16_MAX, bytes, 2);
	case VOE_BASE_FIELD_INT32:
		return store_signed(number, INT32_MAX, bytes, 4);
	case VOE_BASE_FIELD_INT64:
		return store_signed(number, INT64_MAX, bytes, 8);
	case VOE_BASE_FIELD_UINT8:
		return store_unsigned(number, UINT8_MAX, bytes, 1);
	case VOE_BASE_FIELD_UINT16:
		return store_unsigned(number, UINT16_MAX, bytes, 2);
	case VOE_BASE_FIELD_UINT32:
		return store_unsigned(number, UINT32_MAX, bytes, 4);
	case VOE_BASE_FIELD_UINT64:
		return store_unsigned(number, UINT64_MAX, bytes, 8);
	case VOE_BASE_FIELD_FLOAT32:
	case VOE_BASE_FIELD_FLOAT64: {
		bool single = kind == VOE_BASE_FIELD_FLOAT32;
		double value;

		if (!float_of(number, single, &value))
			return false;
		if (single) {
			float narrow = (float)value;

			memcpy(bytes, &narrow, sizeof(narrow));
		} else {
			memcpy(bytes, &value, sizeof(value));
		}
		return true;
	}
	case VOE_BASE_FIELD_BOOL:
		if (strcmp(number, "true") != 0 && strcmp(number, "false") != 0)
			return false;
		bytes[0] = number[0] == 't';
		return true;
	case VOE_BASE_FIELD_ENTITY:
		return unsigned_of(number, UINT64_MAX, entity_id);
	default:
		break;
	}

	VOE_BASE_ASSERT(false, "a field kind this reader does not know");
	return false;
}

// How a kind is spelled, for a refusal.
static const char *spelling(voe_base_field_kind kind)
{
	switch (kind) {
	case VOE_BASE_FIELD_INT8:
		return "an integer from -128 to 127";
	case VOE_BASE_FIELD_INT16:
		return "an integer from -32768 to 32767";
	case VOE_BASE_FIELD_INT32:
		return "a 32-bit signed integer";
	case VOE_BASE_FIELD_INT64:
		return "a 64-bit signed integer";
	case VOE_BASE_FIELD_UINT8:
		return "an integer from 0 to 255";
	case VOE_BASE_FIELD_UINT16:
		return "an integer from 0 to 65535";
	case VOE_BASE_FIELD_UINT32:
		return "a 32-bit unsigned integer";
	case VOE_BASE_FIELD_UINT64:
		return "a 64-bit unsigned integer";
	case VOE_BASE_FIELD_FLOAT32:
		return "a finite decimal that fits a float";
	case VOE_BASE_FIELD_FLOAT64:
		return "a finite decimal";
	case VOE_BASE_FIELD_BOOL:
		return "true or false";
	case VOE_BASE_FIELD_FLOAT2:
		return "[x, y]";
	case VOE_BASE_FIELD_FLOAT3:
	case VOE_BASE_FIELD_COLOUR:
		return "[x, y, z]";
	case VOE_BASE_FIELD_FLOAT4:
	case VOE_BASE_FIELD_QUAT:
		return "[x, y, z, w]";
	case VOE_BASE_FIELD_FLOAT4X4:
		return "16 decimals in brackets";
	case VOE_BASE_FIELD_ENTITY:
		return "an authored id, or 0";
	case VOE_BASE_FIELD_CHAR:
		return "a quoted string";
	case VOE_BASE_FIELD_ENUM:
		break;
	}
	return "a value this reader does not know";
}

static bool char_value(const voe_authoring_site *site,
		       const voe_base_field_description *field,
		       const char *value, uint8_t *bytes)
{
	size_t length = strlen(value);

	if (length >= field->count) {
		VOE_BASE_ERROR(MODULE,
			       "line %u: %s.%s is %zu bytes, and it holds %u "
			       "beside its terminating zero; nothing was loaded",
			       site->line, site->section, site->field, length,
			       field->count - 1);
		return false;
	}
	for (size_t i = 0; i < length; i++) {
		if ((unsigned char)value[i] >= 0x20)
			continue;
		VOE_BASE_ERROR(MODULE,
			       "line %u: %s.%s holds byte 0x%02x at %zu, and a "
			       "scene cannot carry a control character; nothing "
			       "was loaded",
			       site->line, site->section, site->field,
			       (unsigned char)value[i], i);
		return false;
	}
	memcpy(bytes, value, length);
	return true;
}

// A string inside an array: `"…"`, with the escapes `\"` and `\\` and nothing
// else; `,` and `]` inside it are bytes, and a control character refuses it.
// Unlike a bare value — which the sectioned reader unquotes and unescapes
// before this ever sees it — an array's text is not the whole value, so the
// sectioned reader has left this one exactly as the file spelled it: this
// function strips the quotes and undoes the escapes itself (ADR-0154 point 4).
// `max` is the slot's byte count, the field's own innermost dimension.
static bool char_leaf(struct cursor *cursor, size_t max, uint8_t *bytes)
{
	size_t length = 0;

	skip_blanks(cursor);
	if (*cursor->at != '"')
		return false;
	cursor->at++;
	for (;;) {
		char c = *cursor->at;

		if (c == '"') {
			cursor->at++;
			return true;
		}
		if (c == '\\') {
			cursor->at++;
			c = *cursor->at;
			if (c != '"' && c != '\\')
				return false;
		} else if (c == '\0' || (unsigned char)c < 0x20) {
			return false;
		}
		if (length + 1 >= max)
			return false;
		bytes[length++] = (uint8_t)c;
		cursor->at++;
	}
}

// Reads one element, and holds it for pass two if it is a reference. `depth`
// is the bracket-nesting counter, shared with the field's own shape.
static bool held_element(voe_authoring_field_refs *refs,
			 const voe_authoring_site *site, struct cursor *cursor,
			 voe_base_field_kind kind, uint8_t *bytes,
			 uint32_t *depth)
{
	uint64_t id = 0;

	if (!element_of(cursor, kind, bytes, &id, depth, site))
		return false;
	if (kind != VOE_BASE_FIELD_ENTITY || id == 0)
		return true;

	if (voe_authoring_authored_find(refs->authored,
					refs->authored_count, id) == NULL) {
		VOE_BASE_WARNING(MODULE,
				 "line %u: %s.%s names entity %" PRIu64 ", which "
				 "the file does not hold; loaded as no entity",
				 site->line, site->section, site->field, id);
		return true;
	}
	refs->patches[refs->patch_count++] = (voe_authoring_patch){
		.bytes = bytes,
		.id = id,
	};
	return true;
}

// Reads a value against `bracket_rank` levels of the field's own brackets,
// then a leaf, with an explicit stack of one frame per open level — `index`
// and `base` below — rather than a recursive call, and BRACKET_DEPTH_MAX
// enforced through every '[' this or a vector kind's own leaf consumes (rule
// 14; open_bracket()/close_bracket() above are the shared counter).
// `strides[level]` is the byte span of one whole item at `level`;
// `strides[bracket_rank]` is one leaf: a kind element, or for CHAR the
// string's own byte count. Every level's count is exactly its dimension —
// a short, long or ragged level fails the comma or bracket it expects next,
// caught here and reported by voe_authoring_field_value(), the caller.
static bool shape_value(voe_authoring_field_refs *refs,
			const voe_authoring_site *site,
			const voe_base_field_description *field,
			uint32_t bracket_rank, const size_t *strides,
			struct cursor *cursor, uint8_t *bytes, uint32_t *depth)
{
	uint32_t index[VOE_BASE_FIELD_RANK_MAX];
	uint8_t *base[VOE_BASE_FIELD_RANK_MAX + 1];
	uint32_t level = 0;

	base[0] = bytes;
	if (!open_bracket(cursor, depth, site))
		return false;
	index[0] = 0;

	for (;;) {
		if (index[level] > 0 && !expect(cursor, ','))
			return false;

		uint8_t *at =
			base[level] + (size_t)index[level] * strides[level + 1];

		if (level + 1 == bracket_rank) {
			if (field->kind == VOE_BASE_FIELD_CHAR) {
				if (!char_leaf(cursor,
						field->dims[field->rank - 1],
						at))
					return false;
			} else if (!held_element(refs, site, cursor,
						 field->kind, at, depth)) {
				return false;
			}
		} else {
			base[level + 1] = at;
			if (!open_bracket(cursor, depth, site))
				return false;
			level++;
			index[level] = 0;
			continue;
		}

		index[level]++;
		while (index[level] == field->dims[level]) {
			if (!close_bracket(cursor, depth))
				return false;
			if (level == 0)
				return true;
			level--;
			index[level]++;
		}
	}
}

// One value against the field's whole shape: a bare leaf for a rank-0,
// non-CHAR field (a plain value, or a vector kind's own bracket), the field's
// nested brackets otherwise.
static bool elements_of(voe_authoring_field_refs *refs,
			const voe_authoring_site *site,
			const voe_base_field_description *field,
			struct cursor *cursor, uint8_t *bytes, uint32_t *depth)
{
	// Every dimension is a bracket level, except for CHAR, whose innermost
	// dimension is the string's own bytes and not a level (ADR-0154
	// point 8).
	uint32_t bracket_rank = field->kind == VOE_BASE_FIELD_CHAR
					 ? field->rank - 1
					 : field->rank;

	if (bracket_rank == 0)
		return held_element(refs, site, cursor, field->kind, bytes,
				    depth);

	size_t strides[VOE_BASE_FIELD_RANK_MAX + 1];

	strides[bracket_rank] = field->kind == VOE_BASE_FIELD_CHAR
					 ? field->dims[field->rank - 1]
					 : field->size / field->count;
	for (uint32_t l = bracket_rank; l > 0; l--)
		strides[l - 1] = strides[l] * field->dims[l - 1];

	return shape_value(refs, site, field, bracket_rank, strides, cursor,
			   bytes, depth);
}

bool voe_authoring_field_value(voe_authoring_field_refs *refs,
			       const voe_authoring_site *site,
			       const voe_base_field_description *field,
			       const char *value, uint8_t *row)
{
	uint8_t *bytes = row + field->offset;
	struct cursor cursor = { .at = value };
	uint32_t depth = 0;

	VOE_BASE_ASSERT(field->count > 0, "a field of no elements");
	VOE_BASE_ASSERT(field->kind != VOE_BASE_FIELD_CHAR || field->rank >= 1,
			"a CHAR field with no length dimension");
	if (field->kind == VOE_BASE_FIELD_CHAR && field->rank <= 1)
		return char_value(site, field, value, bytes);
	if (field->kind == VOE_BASE_FIELD_ENUM) {
		VOE_BASE_ERROR(MODULE,
			       "line %u: %s.%s is an enum, and no enum value has "
			       "a name to be read with yet; nothing was loaded",
			       site->line, site->section, site->field);
		return false;
	}

	if (elements_of(refs, site, field, &cursor, bytes, &depth)) {
		skip_blanks(&cursor);
		if (*cursor.at == '\0')
			return true;
	}

	if (field->count == 1)
		VOE_BASE_ERROR(MODULE,
			       "line %u: %s.%s = %s is not %s; nothing was "
			       "loaded",
			       site->line, site->section, site->field, value,
			       spelling(field->kind));
	else
		VOE_BASE_ERROR(MODULE,
			       "line %u: %s.%s = %s is not %u of %s, in "
			       "brackets; nothing was loaded",
			       site->line, site->section, site->field, value,
			       field->count, spelling(field->kind));
	return false;
}
