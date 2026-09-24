// The cook: a world walked once into C source text (authoring/scene_cook.h).
//
// The authored entities are copied out of the identity table and merge-sorted by
// id (authored.h), which also gives an ENTITY field the index of its cooked
// entity by binary search. The described types are collected and sorted by key
// name once; then every entity is written type by type into a growing buffer
// in the arena.
//
// THE BUFFER DOUBLES AND THE OLD ONE STAYS IN THE ARENA. The arena frees nothing
// one at a time; the caller rewinds the lot, as the header's contract says.
//
// A FIELD'S DIMENSIONS ARE WALKED BY RECURSION, one level per dimension, so at
// most VOE_BASE_FIELD_RANK_MAX deep: the descriptions are compiled into the
// engine, not read from a file, which is the data rule 14 allows it over.
#include <authoring/scene_cook.h>

#include "authored.h"

#include <base/assert.h>
#include <base/describe.h>
#include <base/report.h>
#include <ecs/component.h>
#include <scene/identity_component.h>

#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MODULE "authoring"

typedef struct {
	const char *key;
	voe_ecs_type type;
	// NULL for a described type this build compiled out.
	const voe_base_struct_description *description;
} cook_type;

typedef struct {
	const voe_ecs_world *world;
	voe_base_arena *arena;
	char *bytes;
	size_t size;
	size_t capacity;
	voe_ecs_type identity;
	voe_authoring_authored *authored;
	uint32_t authored_count;
	cook_type *types;
	uint32_t type_count;
	// Where a refusal is, for its report.
	uint64_t id;
	const char *component;
	const char *field;
} cook;

static void put(cook *c, const char *s, size_t n)
{
	if (c->size + n + 1 > c->capacity) {
		size_t capacity = c->capacity * 2 > c->size + n + 1 ?
					  c->capacity * 2 :
					  c->size + n + 1;
		char *bytes = voe_base_arena_push(c->arena, capacity);

		if (c->size > 0)
			memcpy(bytes, c->bytes, c->size);
		c->bytes = bytes;
		c->capacity = capacity;
	}
	memcpy(c->bytes + c->size, s, n);
	c->size += n;
	c->bytes[c->size] = '\0';
}

static void puts_(cook *c, const char *s)
{
	put(c, s, strlen(s));
}

[[gnu::format(printf, 2, 3)]]
static void putf(cook *c, const char *format, ...)
{
	char buffer[128];
	va_list args;

	va_start(args, format);
	int n = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	VOE_BASE_ASSERT(n >= 0 && (size_t)n < sizeof(buffer),
			"a number longer than the buffer");
	put(c, buffer, (size_t)n);
}

static bool put_float(cook *c, double value, bool single)
{
	if (!isfinite(value)) {
		VOE_BASE_ERROR(MODULE,
			       "entity %" PRIu64 ", component %s, field %s: a NaN "
			       "or infinite float cannot be cooked",
			       c->id, c->component, c->field);
		return false;
	}
	putf(c, single ? "%af" : "%a", value);
	return true;
}

static bool put_floats(cook *c, const uint8_t *at, uint32_t count)
{
	puts_(c, "{ ");
	for (uint32_t i = 0; i < count; i++) {
		float value;

		memcpy(&value, at + i * sizeof(float), sizeof(value));
		if (i > 0)
			puts_(c, ", ");
		if (!put_float(c, value, true))
			return false;
	}
	puts_(c, " }");
	return true;
}

// Bytes up to the first NUL, or all `size` of them.
static void put_string(cook *c, const uint8_t *at, size_t size)
{
	puts_(c, "\"");
	for (size_t i = 0; i < size && at[i] != '\0'; i++) {
		if (at[i] == '"' || at[i] == '\\')
			putf(c, "\\%c", at[i]);
		else if (at[i] < 0x20 || at[i] > 0x7e)
			putf(c, "\\%03o", at[i]);
		else
			put(c, (const char *)&at[i], 1);
	}
	puts_(c, "\"");
}

static void put_int32(cook *c, int32_t value)
{
	if (value == INT32_MIN)
		puts_(c, "(-2147483647 - 1)");
	else
		putf(c, "%" PRId32, value);
}

static void put_entity(cook *c, const uint8_t *at)
{
	voe_ecs_entity target;

	memcpy(&target, at, sizeof(target));
	const voe_scene_identity *identity =
		voe_ecs_entity_alive(c->world, target) ?
			voe_ecs_component_get(c->world, c->identity, target) :
			NULL;
	const voe_authoring_authored *found =
		identity != NULL ? voe_authoring_authored_find(c->authored,
							       c->authored_count,
							       identity->id) :
				   NULL;
	if (found != NULL)
		putf(c, "e[%td]", found - c->authored);
	else
		puts_(c, "{ 0 }");
}

// One element of a field: a scalar, a vector kind, or for CHAR the string that
// is the innermost dimension (`size` bytes).
static bool put_element(cook *c, voe_base_field_kind kind, const uint8_t *at,
			size_t size)
{
	int64_t i64;
	uint64_t u64;
	int32_t i32;
	int16_t i16;
	int8_t i8;
	uint32_t u32;
	uint16_t u16;
	double f64;
	float f32;

	switch (kind) {
	case VOE_BASE_FIELD_INT8:
		memcpy(&i8, at, 1);
		putf(c, "%d", i8);
		return true;
	case VOE_BASE_FIELD_INT16:
		memcpy(&i16, at, 2);
		putf(c, "%d", i16);
		return true;
	case VOE_BASE_FIELD_INT32:
	case VOE_BASE_FIELD_ENUM:
		memcpy(&i32, at, 4);
		put_int32(c, i32);
		return true;
	case VOE_BASE_FIELD_INT64:
		memcpy(&i64, at, 8);
		if (i64 == INT64_MIN)
			puts_(c, "(-9223372036854775807ll - 1)");
		else
			putf(c, "%" PRId64 "ll", i64);
		return true;
	case VOE_BASE_FIELD_UINT8:
		putf(c, "%u", at[0]);
		return true;
	case VOE_BASE_FIELD_UINT16:
		memcpy(&u16, at, 2);
		putf(c, "%u", u16);
		return true;
	case VOE_BASE_FIELD_UINT32:
		memcpy(&u32, at, 4);
		putf(c, "%" PRIu32 "u", u32);
		return true;
	case VOE_BASE_FIELD_UINT64:
		memcpy(&u64, at, 8);
		putf(c, "%" PRIu64 "ull", u64);
		return true;
	case VOE_BASE_FIELD_FLOAT32:
		memcpy(&f32, at, 4);
		return put_float(c, f32, true);
	case VOE_BASE_FIELD_FLOAT64:
		memcpy(&f64, at, 8);
		return put_float(c, f64, false);
	case VOE_BASE_FIELD_BOOL:
		puts_(c, at[0] != 0 ? "true" : "false");
		return true;
	case VOE_BASE_FIELD_FLOAT2:
		return put_floats(c, at, 2);
	case VOE_BASE_FIELD_FLOAT3:
	case VOE_BASE_FIELD_COLOUR:
		return put_floats(c, at, 3);
	case VOE_BASE_FIELD_FLOAT4:
	case VOE_BASE_FIELD_QUAT:
		return put_floats(c, at, 4);
	case VOE_BASE_FIELD_FLOAT4X4:
		puts_(c, "{ { ");
		for (uint32_t row = 0; row < 4; row++) {
			if (row > 0)
				puts_(c, ", ");
			if (!put_floats(c, at + row * 4 * sizeof(float), 4))
				return false;
		}
		puts_(c, " } }");
		return true;
	case VOE_BASE_FIELD_CHAR:
		put_string(c, at, size);
		return true;
	case VOE_BASE_FIELD_ENTITY:
		put_entity(c, at);
		return true;
	}
	VOE_BASE_ASSERT(false, "a field kind this cook does not know");
	return false;
}

// Dimension `level` of a field whose items at this level are `size` bytes in
// all; one brace per dimension. A CHAR's innermost dimension is its string, and
// a rank-0 CHAR is one byte, cooked as its integer.
static bool put_dimension(cook *c, const voe_base_field_description *field,
			  const uint8_t *at, size_t size, uint32_t level)
{
	bool is_char = field->kind == VOE_BASE_FIELD_CHAR;
	uint32_t leaf = is_char && field->rank > 0 ? field->rank - 1 :
						     field->rank;

	if (level == leaf && is_char && field->rank == 0) {
		putf(c, "%u", at[0]);
		return true;
	}
	if (level == leaf)
		return put_element(c, field->kind, at, size);
	size_t stride = size / field->dims[level];
	puts_(c, "{ ");
	for (uint32_t i = 0; i < field->dims[level]; i++) {
		if (i > 0)
			puts_(c, ", ");
		if (!put_dimension(c, field, at + i * stride, stride, level + 1))
			return false;
	}
	puts_(c, " }");
	return true;
}

static bool put_row(cook *c, uint32_t index, const cook_type *type)
{
	const uint8_t *row = voe_ecs_component_get(
		c->world, type->type, c->authored[index].entity);

	if (row == NULL)
		return true;
	c->component = type->key;
	if (type->description == NULL) {
		VOE_BASE_ERROR(MODULE,
			       "entity %" PRIu64 ", component %s: this build "
			       "compiled its description out, so it cannot be "
			       "cooked",
			       c->id, type->key);
		return false;
	}
	puts_(c, "\tif (!voe_ecs_component_add(world, voe_ecs_component_type("
		 "world, &");
	puts_(c, type->key);
	putf(c, "_key), e[%" PRIu32 "], &(", index);
	puts_(c, type->key);
	puts_(c, "){ ");
	for (uint32_t f = 0; f < type->description->field_count; f++) {
		const voe_base_field_description *field =
			&type->description->fields[f];

		c->field = field->name;
		putf(c, "%s.", f > 0 ? ", " : "");
		puts_(c, field->name);
		puts_(c, " = ");
		if (!put_dimension(c, field, row + field->offset, field->size,
				   0))
			return false;
	}
	puts_(c, " }))\n\t\treturn false;\n");
	return true;
}

// Every described type that is not runtime-only, ascending by key name; notes
// the identity type. False when the world has no identity type.
static bool collect_types(cook *c)
{
	uint32_t count = voe_ecs_component_type_count(c->world);
	bool has_identity = false;

	c->types = voe_base_arena_push(c->arena, (count + 1) * sizeof(cook_type));
	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(c->world, i);
		const struct voe_ecs_key *key =
			voe_ecs_component_key(c->world, type);

		if (key == &voe_scene_identity_key) {
			has_identity = true;
			c->identity = type;
		}
		if (voe_ecs_component_runtime_only(c->world, type))
			continue;
		cook_type item = {
			.key = key->name,
			.type = type,
			.description = voe_ecs_component_description(c->world, type),
		};
		uint32_t j = c->type_count++;
		for (; j > 0 && strcmp(c->types[j - 1].key, item.key) > 0; j--)
			c->types[j] = c->types[j - 1];
		c->types[j] = item;
	}
	return has_identity;
}

// The identities, ascending by id; false on two with one id.
static bool collect_authored(cook *c)
{
	uint32_t count = voe_ecs_component_count(c->world, c->identity);
	const voe_scene_identity *rows =
		voe_ecs_component_rows(c->world, c->identity);
	const voe_ecs_entity *owners =
		voe_ecs_component_entities(c->world, c->identity);
	size_t bytes = (count + 1) * sizeof(voe_authoring_authored);
	voe_authoring_authored *scratch = voe_base_arena_push(c->arena, bytes);

	c->authored = voe_base_arena_push(c->arena, bytes);
	for (uint32_t i = 0; i < count; i++)
		c->authored[i] = (voe_authoring_authored){ rows[i].id, owners[i] };
	voe_authoring_authored_sort(c->authored, scratch, count);
	c->authored_count = count;
	for (uint32_t i = 1; i < count; i++) {
		if (c->authored[i - 1].id != c->authored[i].id)
			continue;
		VOE_BASE_ERROR(MODULE,
			       "entity %" PRIu64 ", component %s, field id: two "
			       "entities have this authored id, and the cook "
			       "cannot tell them apart",
			       c->authored[i].id, voe_scene_identity_key.name);
		return false;
	}
	return true;
}

bool voe_authoring_scene_cook(const voe_ecs_world *world, const char *include,
			      const char *function, voe_base_arena *arena,
			      voe_authoring_text *out)
{
	VOE_BASE_ASSERT(world != NULL && include != NULL && function != NULL,
			"no world, include or function");
	VOE_BASE_ASSERT(arena != NULL && out != NULL, "no arena or out");
	cook c = { .world = world, .arena = arena };

	if (collect_types(&c) && !collect_authored(&c))
		return false;
	puts_(&c, "#include <");
	puts_(&c, include);
	puts_(&c, ">\n\nbool ");
	puts_(&c, function);
	puts_(&c, "(voe_ecs_world *world)\n{\n");
	if (c.authored_count == 0) {
		puts_(&c, "\t(void)world;\n");
	} else {
		putf(&c,
		     "\tvoe_ecs_entity e[%" PRIu32 "];\n\n"
		     "\tfor (uint32_t i = 0; i < %" PRIu32 "; i++)\n"
		     "\t\tif (!voe_ecs_entity_create(world, &e[i]))\n"
		     "\t\t\treturn false;\n",
		     c.authored_count, c.authored_count);
	}
	for (uint32_t i = 0; i < c.authored_count; i++) {
		c.id = c.authored[i].id;
		for (uint32_t t = 0; t < c.type_count; t++)
			if (!put_row(&c, i, &c.types[t]))
				return false;
	}
	puts_(&c, "\treturn true;\n}\n");
	*out = (voe_authoring_text){ .text = c.bytes, .size = c.size };
	return true;
}
