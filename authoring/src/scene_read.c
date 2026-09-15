// The scene reader. Two passes, and only the second one touches the world.
//
// PASS ONE READS EVERYTHING INTO SCRATCH. Every section's rows are pushed zeroed
// and filled from the text, every entity reference is held as an authored id, and
// every kept section is copied out — so every refusal happens while the world is
// still exactly as it was handed over. Pass two creates the entities, maps the ids
// to them, patches the references and adds the rows, and can only fail on the
// world's capacity.
//
// THE TEXT IS WALKED A SECOND TIME, BESIDE THE SECTIONED READER, for two things it
// does not hand back: the line each section and key came from, which every
// refusal names, and a kept section's lines as the file spelled them, which is the
// only way they can go back out byte for byte — the parsed value has lost its
// quotes. That walk only classifies lines: blank or `//` is nothing, `[` is a
// header, anything else is a key. It does not validate, because the sectioned
// reader already refused every line that is none of those, and the k-th header or
// key it finds is the k-th the sectioned reader returned; an assert holds both
// counts to that.
//
// A ROW IS PUSHED AT THE SIZE ITS DESCRIPTION IMPLIES, NOT AT ITS REGISTERED SIZE.
// DEVIATION: card 071 scope 2, "scratch bytes of the type's size" is read as the
// end of the description's last field rounded up to 8, because ecs hands a type's
// size back only through _replace and not every type has one, and ecs may not be
// edited on this card. A struct written through VOE_BASE_DESCRIBE_STRUCT describes
// every member, its alignment is at most 8 for every kind, so that is at least its
// sizeof; where _replace does know the size an assert holds the two to it.
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
#include <authoring/scene_read.h>

#include "authored.h"

#include <assets/sectioned.h>
#include <base/assert.h>
#include <base/describe.h>
#include <base/report.h>
#include <ecs/component.h>
#include <scene/identity_component.h>

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
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

struct span {
	const char *bytes;
	size_t size;
};

// What one section is, decided before any value is read.
enum section_role {
	ROLE_ENTITY,	// `[N]`
	ROLE_COMPONENT, // `[N.<key>]`, a described type
	ROLE_KEPT,	// `[N.<key>]`, a name nothing registered
};

struct section {
	enum section_role role;
	uint64_t id;
	voe_ecs_type type;
	const voe_base_struct_description *description;
	// The scratch row; NULL for a kept section.
	uint8_t *row;
};

// An ENTITY element, held as an authored id until the entity exists. Only ids
// the file holds are held.
struct patch {
	uint8_t *bytes;
	uint64_t id;
};

struct reader {
	voe_ecs_world *world;
	voe_base_arena *arena;
	voe_assets_sectioned doc;

	// One per section and one per key, as the sectioned reader numbers them.
	uint32_t *section_line;
	uint32_t *key_line;
	struct span *key_span;

	bool has_identity;
	voe_ecs_type identity;

	struct section *sections;

	// One per `[N]`, ascending by id; the entities are filled in pass two.
	voe_authoring_authored *authored;
	uint32_t authored_count;

	struct patch *patches;
	uint32_t patch_count;

	voe_authoring_kept_section *kept;
	uint32_t kept_count;
};

// Where a value sits, for a report.
struct site {
	uint32_t line;
	const char *section;
	const char *field;
};

static bool blank(char c)
{
	return c == ' ' || c == '\t';
}

static void index_lines(struct reader *reader, const char *text, size_t size)
{
	uint32_t line = 1;
	uint32_t section = 0;
	uint32_t key = 0;
	size_t at = 0;

	while (at < size) {
		const char *newline = memchr(text + at, '\n', size - at);
		size_t end = newline != NULL ? (size_t)(newline - text) : size;
		size_t first = at;
		size_t last = end;

		if (last > first && text[last - 1] == '\r')
			last--;
		while (first < last && blank(text[first]))
			first++;
		while (last > first && blank(text[last - 1]))
			last--;

		if (first == last ||
		    (last - first >= 2 && text[first] == '/' &&
		     text[first + 1] == '/')) {
			// Blank or a comment.
		} else if (text[first] == '[') {
			VOE_BASE_ASSERT(section < reader->doc.section_count,
					"more headers than the sectioned reader found");
			reader->section_line[section++] = line;
		} else {
			VOE_BASE_ASSERT(key < reader->doc.key_count,
					"more keys than the sectioned reader found");
			reader->key_line[key] = line;
			reader->key_span[key] = (struct span){
				.bytes = text + first,
				.size = last - first,
			};
			key++;
		}

		at = end + 1;
		line++;
	}

	VOE_BASE_ASSERT(section == reader->doc.section_count &&
				key == reader->doc.key_count,
			"the line walk and the sectioned reader disagree");
}

// A decimal from 1, no sign, no leading zero, fitting 64 bits, running exactly
// `size` bytes.
static bool authored_id(const char *digits, size_t size, uint64_t *out)
{
	uint64_t value = 0;

	if (size == 0 || digits[0] == '0')
		return false;
	for (size_t i = 0; i < size; i++) {
		if (digits[i] < '0' || digits[i] > '9')
			return false;
		uint64_t digit = (uint64_t)(digits[i] - '0');

		if (value > (UINT64_MAX - digit) / 10)
			return false;
		value = value * 10 + digit;
	}
	*out = value;
	return true;
}

static bool type_by_name(const voe_ecs_world *world, const char *name,
			 voe_ecs_type *out)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (strcmp(voe_ecs_component_key(world, type)->name, name) == 0) {
			*out = type;
			return true;
		}
	}
	return false;
}

// See the file header's DEVIATION.
static size_t row_size(const voe_ecs_world *world, voe_ecs_type type,
		       const voe_base_struct_description *description)
{
	size_t end = 1;

	for (uint32_t f = 0; f < description->field_count; f++) {
		const voe_base_field_description *field = &description->fields[f];

		if (field->offset + field->size > end)
			end = field->offset + field->size;
	}
	end = (end + 7) & ~(size_t)7;

	voe_ecs_replace replace = voe_ecs_component_replace(world, type);

	VOE_BASE_ASSERT(!replace.set || replace.row_size <= end,
			"a description that stops short of its struct");
	return end;
}

// Pass one, first half: every section's role, its id, and its `[N]`.
static bool classify(struct reader *reader)
{
	const voe_assets_sectioned *doc = &reader->doc;
	uint32_t authored = 0;

	for (uint32_t s = 0; s < doc->section_count; s++) {
		const char *name = doc->sections[s].name;
		const char *dot = strchr(name, '.');
		size_t digits = dot != NULL ? (size_t)(dot - name) : strlen(name);
		struct section *section = &reader->sections[s];

		if (!authored_id(name, digits, &section->id) ||
		    (dot != NULL && dot[1] == '\0')) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is not a section of a scene, "
				       "which is [N] or [N.<key name>] with N a "
				       "decimal from 1 and no leading zero; nothing "
				       "was loaded",
				       reader->section_line[s], name);
			return false;
		}
		if (dot != NULL)
			continue;

		if (!reader->has_identity) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is an authored entity, and "
				       "this world registered no %s to give it "
				       "one with; nothing was loaded",
				       reader->section_line[s], name,
				       voe_scene_identity_key.name);
			return false;
		}
		section->role = ROLE_ENTITY;
		section->type = reader->identity;
		section->description =
			voe_ecs_component_description(reader->world,
						      reader->identity);
		reader->authored[authored++] = (voe_authoring_authored){
			.id = section->id,
		};
	}

	voe_authoring_authored *scratch = voe_base_arena_push(
		reader->arena, (authored + 1) * sizeof(*scratch));

	reader->authored_count = authored;
	voe_authoring_authored_sort(reader->authored, scratch, authored);

	for (uint32_t s = 0; s < doc->section_count; s++) {
		struct section *section = &reader->sections[s];
		const char *name = doc->sections[s].name;
		const char *dot = strchr(name, '.');

		if (dot == NULL)
			continue;
		if (voe_authoring_authored_find(reader->authored,
						reader->authored_count,
						section->id) == NULL) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] belongs to entity %" PRIu64
				       ", and the file has no [%" PRIu64 "]; "
				       "nothing was loaded",
				       reader->section_line[s], name, section->id,
				       section->id);
			return false;
		}

		const char *key = dot + 1;

		if (!type_by_name(reader->world, key, &section->type)) {
			section->role = ROLE_KEPT;
			continue;
		}
		if (reader->has_identity &&
		    section->type.value == reader->identity.value) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is an identity, and an "
				       "identity is written as [%" PRIu64 "] "
				       "itself; nothing was loaded",
				       reader->section_line[s], name, section->id);
			return false;
		}
		if (voe_ecs_component_runtime_only(reader->world,
						   section->type)) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is %s, which is runtime-only "
				       "and never authored, so a file cannot hold "
				       "one; nothing was loaded",
				       reader->section_line[s], name, key);
			return false;
		}
		section->role = ROLE_COMPONENT;
		section->description =
			voe_ecs_component_description(reader->world,
						      section->type);
	}
	return true;
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
			 const struct site *site)
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
		      uint32_t *depth, const struct site *site)
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
		       const struct site *site)
{
	char number[NUMBER_MAX];

	switch (kind) {
	case VOE_BASE_FIELD_FLOAT2:
		return floats_of(cursor, bytes, 2, depth, site);
	case VOE_BASE_FIELD_FLOAT3:
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

static bool char_value(const struct site *site,
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
static bool held_element(struct reader *reader, const struct site *site,
			 struct cursor *cursor, voe_base_field_kind kind,
			 uint8_t *bytes, uint32_t *depth)
{
	uint64_t id = 0;

	if (!element_of(cursor, kind, bytes, &id, depth, site))
		return false;
	if (kind != VOE_BASE_FIELD_ENTITY || id == 0)
		return true;

	if (voe_authoring_authored_find(reader->authored,
					reader->authored_count, id) == NULL) {
		VOE_BASE_WARNING(MODULE,
				 "line %u: %s.%s names entity %" PRIu64 ", which "
				 "the file does not hold; loaded as no entity",
				 site->line, site->section, site->field, id);
		return true;
	}
	reader->patches[reader->patch_count++] = (struct patch){
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
// caught here and reported by field_value(), the caller.
static bool shape_value(struct reader *reader, const struct site *site,
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
			} else if (!held_element(reader, site, cursor,
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
static bool elements_of(struct reader *reader, const struct site *site,
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
		return held_element(reader, site, cursor, field->kind, bytes,
				    depth);

	size_t strides[VOE_BASE_FIELD_RANK_MAX + 1];

	strides[bracket_rank] = field->kind == VOE_BASE_FIELD_CHAR
					 ? field->dims[field->rank - 1]
					 : field->size / field->count;
	for (uint32_t l = bracket_rank; l > 0; l--)
		strides[l - 1] = strides[l] * field->dims[l - 1];

	return shape_value(reader, site, field, bracket_rank, strides, cursor,
			   bytes, depth);
}

static bool field_value(struct reader *reader, const struct site *site,
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

	if (elements_of(reader, site, field, &cursor, bytes, &depth)) {
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

static const voe_base_field_description *
field_by_name(const voe_base_struct_description *description, const char *name,
	      bool skip_id)
{
	for (uint32_t f = 0; f < description->field_count; f++) {
		const voe_base_field_description *field = &description->fields[f];

		if (skip_id && field->offset == offsetof(voe_scene_identity, id))
			continue;
		if (strcmp(field->name, name) == 0)
			return field;
	}
	return NULL;
}

// Pass one, second half, for one `[N]` or `[N.<key>]` of a registered type.
static bool read_section(struct reader *reader, uint32_t s)
{
	const voe_assets_sectioned_section *parsed = &reader->doc.sections[s];
	struct section *section = &reader->sections[s];
	const voe_base_struct_description *description = section->description;
	bool entity = section->role == ROLE_ENTITY;
	struct site site = {
		.line = reader->section_line[s],
		.section = parsed->name,
	};

	if (description == NULL) {
		VOE_BASE_ERROR(MODULE,
			       "line %u: [%s] is %s, whose description this "
			       "build compiled out, so its fields cannot be "
			       "read; nothing was loaded",
			       site.line, parsed->name,
			       voe_ecs_component_key(reader->world,
						     section->type)->name);
		return false;
	}

	size_t size = entity ? sizeof(voe_scene_identity)
			     : row_size(reader->world, section->type,
					description);

	section->row = voe_base_arena_push(reader->arena, size);
	memset(section->row, 0, size);

	for (uint32_t k = 0; k < parsed->key_count; k++) {
		uint32_t index = parsed->first_key + k;
		const voe_assets_sectioned_key *key = &reader->doc.keys[index];
		const voe_base_field_description *field =
			field_by_name(description, key->name, entity);

		site.line = reader->key_line[index];
		if (field == NULL) {
			VOE_BASE_WARNING(MODULE,
					 "line %u: [%s] has no field %s; the "
					 "line is ignored",
					 site.line, parsed->name, key->name);
			continue;
		}
		site.field = field->name;
		if (!field_value(reader, &site, field, key->value, section->row))
			return false;
	}

	for (uint32_t f = 0; f < description->field_count; f++) {
		const voe_base_field_description *field = &description->fields[f];

		if (entity && field->offset == offsetof(voe_scene_identity, id))
			continue;
		if (voe_assets_sectioned_value(&reader->doc, s, field->name) !=
		    NULL)
			continue;
		VOE_BASE_WARNING(MODULE,
				 "line %u: [%s] does not say %s; loaded as zero",
				 reader->section_line[s], parsed->name,
				 field->name);
	}

	if (entity) {
		voe_scene_identity identity;

		memcpy(&identity, section->row, sizeof(identity));
		identity.id = section->id;
		memcpy(section->row, &identity, sizeof(identity));
	}
	return true;
}

static void keep_section(struct reader *reader, uint32_t s)
{
	const voe_assets_sectioned_section *parsed = &reader->doc.sections[s];
	size_t size = 0;
	char *lines;

	for (uint32_t k = 0; k < parsed->key_count; k++)
		size += reader->key_span[parsed->first_key + k].size + 1;

	lines = voe_base_arena_push(reader->arena, size + 1);
	size = 0;
	for (uint32_t k = 0; k < parsed->key_count; k++) {
		const struct span *span = &reader->key_span[parsed->first_key + k];

		memcpy(lines + size, span->bytes, span->size);
		size += span->size;
		lines[size++] = '\n';
	}

	reader->kept[reader->kept_count++] = (voe_authoring_kept_section){
		.id = reader->sections[s].id,
		.key = strchr(parsed->name, '.') + 1,
		.lines = lines,
		.size = size,
	};
}

// The most ENTITY elements the file's rows could hold, which is how many patches
// pass one may make.
static uint32_t patch_capacity(const struct reader *reader)
{
	uint32_t count = 0;

	for (uint32_t s = 0; s < reader->doc.section_count; s++) {
		const struct section *section = &reader->sections[s];

		if (section->role == ROLE_KEPT || section->description == NULL)
			continue;
		for (uint32_t f = 0; f < section->description->field_count; f++)
			if (section->description->fields[f].kind ==
			    VOE_BASE_FIELD_ENTITY)
				count += section->description->fields[f].count;
	}
	return count;
}

static void find_identity(struct reader *reader)
{
	uint32_t count = voe_ecs_component_type_count(reader->world);

	// Walked rather than voe_ecs_component_type, which asserts on a world
	// that registered none.
	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(reader->world, i);

		if (voe_ecs_component_key(reader->world, type) ==
		    &voe_scene_identity_key) {
			reader->has_identity = true;
			reader->identity = type;
		}
	}
}

// Pass two. Only the world's capacity can fail it.
static bool create(struct reader *reader)
{
	const voe_assets_sectioned *doc = &reader->doc;

	for (uint32_t s = 0; s < doc->section_count; s++) {
		struct section *section = &reader->sections[s];

		if (section->role != ROLE_ENTITY)
			continue;

		voe_authoring_authored *authored = voe_authoring_authored_find(
			reader->authored, reader->authored_count, section->id);

		VOE_BASE_ASSERT(authored != NULL, "an [N] missing from its map");
		if (!voe_ecs_entity_create(reader->world, &authored->entity)) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: the world is full at [%" PRIu64
				       "]; it holds part of the scene and must "
				       "be discarded",
				       reader->section_line[s], section->id);
			return false;
		}
	}

	for (uint32_t p = 0; p < reader->patch_count; p++) {
		const struct patch *patch = &reader->patches[p];
		voe_authoring_authored *authored = voe_authoring_authored_find(
			reader->authored, reader->authored_count, patch->id);

		VOE_BASE_ASSERT(authored != NULL, "a held id missing from the map");
		memcpy(patch->bytes, &authored->entity, sizeof(authored->entity));
	}

	// The identities first, then every other row, each in file order.
	for (uint32_t pass = 0; pass < 2; pass++) {
		for (uint32_t s = 0; s < doc->section_count; s++) {
			const struct section *section = &reader->sections[s];

			if (section->role == ROLE_KEPT ||
			    (section->role == ROLE_ENTITY) != (pass == 0))
				continue;

			voe_authoring_authored *authored =
				voe_authoring_authored_find(
					reader->authored,
					reader->authored_count, section->id);

			VOE_BASE_ASSERT(authored != NULL,
					"a section's [N] missing from the map");
			if (voe_ecs_component_add(reader->world, section->type,
						  authored->entity, section->row))
				continue;
			VOE_BASE_ERROR(MODULE,
				       "line %u: the world has no room for [%s]; "
				       "it holds part of the scene and must be "
				       "discarded",
				       reader->section_line[s],
				       doc->sections[s].name);
			return false;
		}
	}
	return true;
}

bool voe_authoring_scene_read(const char *text, size_t size,
			      voe_ecs_world *world, voe_base_arena *arena,
			      voe_authoring_kept *out_kept)
{
	struct reader reader = { .world = world, .arena = arena };

	VOE_BASE_ASSERT(text != NULL || size == 0, "reading NULL text");
	VOE_BASE_ASSERT(world != NULL, "reading into a NULL world");
	VOE_BASE_ASSERT(arena != NULL, "reading with a NULL arena");
	VOE_BASE_ASSERT(out_kept != NULL, "nowhere to put the kept sections");

	find_identity(&reader);
	VOE_BASE_ASSERT(!reader.has_identity ||
				voe_ecs_component_count(world, reader.identity) == 0,
			"loading a scene into a world that already holds an "
			"authored entity");

	if (!voe_assets_sectioned_parse(text, size, arena, &reader.doc))
		return false;

	uint32_t sections = reader.doc.section_count;
	uint32_t keys = reader.doc.key_count;

	// One more than needed of each, so that an empty file pushes something.
	reader.section_line = voe_base_arena_push(
		arena, (sections + 1) * sizeof(*reader.section_line));
	reader.key_line =
		voe_base_arena_push(arena, (keys + 1) * sizeof(*reader.key_line));
	reader.key_span =
		voe_base_arena_push(arena, (keys + 1) * sizeof(*reader.key_span));
	reader.sections = voe_base_arena_push(
		arena, (sections + 1) * sizeof(*reader.sections));
	memset(reader.sections, 0, (sections + 1) * sizeof(*reader.sections));
	reader.authored = voe_base_arena_push(
		arena, (sections + 1) * sizeof(*reader.authored));
	reader.kept =
		voe_base_arena_push(arena, (sections + 1) * sizeof(*reader.kept));

	index_lines(&reader, text, size);
	if (!classify(&reader))
		return false;

	reader.patches = voe_base_arena_push(
		arena, (patch_capacity(&reader) + 1) * sizeof(*reader.patches));

	for (uint32_t s = 0; s < sections; s++) {
		if (reader.sections[s].role == ROLE_KEPT)
			keep_section(&reader, s);
		else if (!read_section(&reader, s))
			return false;
	}

	if (!create(&reader))
		return false;

	*out_kept = (voe_authoring_kept){
		.sections = reader.kept_count > 0 ? reader.kept : NULL,
		.count = reader.kept_count,
	};
	return true;
}
