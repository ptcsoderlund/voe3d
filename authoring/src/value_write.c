// One field's value spelled into a scene's text, by its kind and its shape —
// the scene writer's leaves; scene_write.c owns the walk that reaches them.
//
// THE SPLIT FALLS AT ONE FIELD. The walk decides which entities and sections are
// written and in what order, and refuses what is wrong with an entity or an id;
// everything here is one value and what is wrong with it: an ENUM, a NaN or an
// infinity, a control byte, a description compiled out. Nothing here knows the
// order of anything or what a kept section is.
//
// A FLOAT IS THE FEWEST DIGITS THAT READ BACK TO THE SAME BITS, found by raising
// %g's precision until they do (put_shortest).
//
// A FIELD'S SHAPE IS WALKED BY RECURSION, NOT AN EXPLICIT STACK (ADR-0154). The
// nesting here is over a field's own rank and dims — the program's own
// description, compiled in, never read from a file — so rule 14 (no recursion
// over data read from a file) does not bind; put_shape() recurses at most
// VOE_BASE_FIELD_RANK_MAX deep, a fixed bound the type checks at compile time.
#include "value_write.h"

#include <base/assert.h>
#include <base/report.h>
#include <scene/identity_component.h>
#include <scene/prefab_component.h>

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE "authoring"

// The most significant digits %g needs for any float, and for any double, to
// read back to the same bits.
#define FLOAT32_DIGITS_MAX 9
#define FLOAT64_DIGITS_MAX 17

bool voe_authoring_measuring(const voe_authoring_output *text)
{
	return text->bytes == NULL;
}

void voe_authoring_put(voe_authoring_output *text, const char *bytes,
		       size_t size)
{
	if (!voe_authoring_measuring(text))
		memcpy(text->bytes + text->size, bytes, size);
	text->size += size;
}

void voe_authoring_put_string(voe_authoring_output *text, const char *string)
{
	voe_authoring_put(text, string, strlen(string));
}

static void put_signed(voe_authoring_output *text, int64_t value)
{
	char digits[24];

	(void)snprintf(digits, sizeof(digits), "%" PRId64, value);
	voe_authoring_put_string(text, digits);
}

void voe_authoring_put_unsigned(voe_authoring_output *text, uint64_t value)
{
	char digits[24];

	(void)snprintf(digits, sizeof(digits), "%" PRIu64, value);
	voe_authoring_put_string(text, digits);
}

static bool reads_back(const char *digits, double value, bool single)
{
	if (single) {
		float wanted = (float)value;
		float back = strtof(digits, NULL);

		return memcmp(&back, &wanted, sizeof(wanted)) == 0;
	}

	double back = strtod(digits, NULL);

	return memcmp(&back, &value, sizeof(value)) == 0;
}

// The fewest significant digits that read back to the same bits, found by
// raising %g's precision until they do, and then the shorter of the two ways to
// spell those digits. -0.0 is "-0" at every precision, so it needs no case.
//
// DEVIATION: card 070 scope 3, %.*g's own spelling is not always kept, because
// %g goes to an exponent once the exponent reaches the precision, so 10 at one
// digit is `1e+01` and the card's rule is the shortest decimal. The same digits
// written without an exponent are tried and the shorter spelling wins, the plain
// one on a tie: 10 is `10`, 1e-05 stays `1e-05`, 3.4028235e+38 stays as it is.
static void put_shortest(voe_authoring_output *text, double value, bool single)
{
	int most = single ? FLOAT32_DIGITS_MAX : FLOAT64_DIGITS_MAX;
	char digits[32];
	char plain[400];
	int precision;

	for (precision = 1; precision < most; precision++) {
		(void)snprintf(digits, sizeof(digits), "%.*g", precision, value);
		if (reads_back(digits, value, single))
			break;
	}
	(void)snprintf(digits, sizeof(digits), "%.*g", precision, value);

	const char *e = strchr(digits, 'e');

	if (e == NULL) {
		voe_authoring_put_string(text, digits);
		return;
	}

	// Enough decimals to keep `precision` significant digits, rounded at the
	// same place %g rounded them, so both spellings carry the same digits.
	long exponent = strtol(e + 1, NULL, 10);
	long decimals = precision - 1 - exponent;

	(void)snprintf(plain, sizeof(plain), "%.*f",
		       (int)(decimals > 0 ? decimals : 0), value);
	voe_authoring_put_string(text,
				 strlen(plain) <= strlen(digits) ? plain : digits);
}

static bool finite_or_refuse(const voe_authoring_output *text,
			     const voe_authoring_value_site *site, double value)
{
	if (isfinite(value))
		return true;

	if (voe_authoring_measuring(text))
		VOE_BASE_ERROR(MODULE,
			       "entity %" PRIu64 ", %s.%s: %g is not finite, and "
			       "a scene file has no decimal for it; nothing was "
			       "written",
			       site->id, site->component, site->field, value);
	return false;
}

// `count` floats, bracketed when there is more than one.
static bool put_floats(voe_authoring_output *text,
		       const voe_authoring_value_site *site,
		       const uint8_t *bytes, uint32_t count)
{
	if (count > 1)
		voe_authoring_put_string(text, "[");
	for (uint32_t i = 0; i < count; i++) {
		float value;

		memcpy(&value, bytes + i * sizeof(value), sizeof(value));
		if (!finite_or_refuse(text, site, value))
			return false;
		if (i > 0)
			voe_authoring_put_string(text, ", ");
		put_shortest(text, value, true);
	}
	if (count > 1)
		voe_authoring_put_string(text, "]");
	return true;
}

// Three doubles in one bracket, each as FLOAT64 is written (ADR-0250).
static bool put_double3(voe_authoring_output *text,
			const voe_authoring_value_site *site,
			const uint8_t *bytes)
{
	double value[3];

	VOE_BASE_ASSERT(text != NULL && site != NULL, "a text and a site");
	VOE_BASE_ASSERT(bytes != NULL, "the field's bytes");
	memcpy(value, bytes, sizeof(value));
	voe_authoring_put_string(text, "[");
	for (uint32_t i = 0; i < 3; i++) {
		if (!finite_or_refuse(text, site, value[i]))
			return false;
		if (i > 0)
			voe_authoring_put_string(text, ", ");
		put_shortest(text, value[i], false);
	}
	voe_authoring_put_string(text, "]");
	return true;
}

bool voe_authoring_prefab_part_skipped(const voe_ecs_world *world,
				       voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(world != NULL, "asking about a part in no world");

	const voe_scene_prefab_part *part =
		voe_scene_prefab_part_get(world, entity);
	bool skipped = part != NULL &&
		       (part->instance.index != entity.index ||
			part->instance.generation != entity.generation);

	VOE_BASE_DEBUG_ASSERT(!skipped || part != NULL,
			      "a skipped part without a part row");
	return skipped;
}

bool voe_authoring_tree_holds(const voe_authoring_tree *tree,
			      voe_ecs_entity entity)
{
	VOE_BASE_ASSERT(tree != NULL, "asking about no tree");
	VOE_BASE_ASSERT(tree->entities != NULL || tree->count == 0,
			"a tree with entities and nowhere they are");

	for (uint32_t i = 0; i < tree->count; i++)
		if (tree->entities[i].index == entity.index &&
		    tree->entities[i].generation == entity.generation)
			return true;
	return false;
}

static void put_entity(voe_authoring_output *text,
		       const voe_authoring_value_site *site,
		       const uint8_t *bytes)
{
	voe_ecs_entity target;
	const voe_scene_identity *identity;

	memcpy(&target, bytes, sizeof(target));
	// Outside a prefab's tree is not in its file, and that is what a prefab
	// is, so it is no warning (0283 point 1).
	if (!voe_ecs_entity_alive(site->world, target) ||
	    (site->tree != NULL &&
	     !voe_authoring_tree_holds(site->tree, target))) {
		voe_authoring_put_string(text, "0");
		return;
	}

	// An entity with a field being written is authored, so the identity type
	// is registered.
	identity = voe_ecs_component_get(site->world, site->identity, target);
	if (identity != NULL &&
	    !voe_authoring_prefab_part_skipped(site->world, target)) {
		voe_authoring_put_unsigned(text, identity->id);
		return;
	}

	if (voe_authoring_measuring(text))
		VOE_BASE_WARNING(MODULE,
				 "entity %" PRIu64 ", %s.%s names entity %uv%u, "
				 "which has no identity and will not be in the "
				 "file; written as 0",
				 site->id, site->component, site->field,
				 target.index, target.generation);
	voe_authoring_put_string(text, "0");
}

// One element of a field that is not CHAR.
static bool put_element(voe_authoring_output *text,
			const voe_authoring_value_site *site,
			voe_base_field_kind kind, const uint8_t *bytes)
{
	switch (kind) {
	case VOE_BASE_FIELD_INT8: {
		int8_t value;

		memcpy(&value, bytes, sizeof(value));
		put_signed(text, value);
		return true;
	}
	case VOE_BASE_FIELD_INT16: {
		int16_t value;

		memcpy(&value, bytes, sizeof(value));
		put_signed(text, value);
		return true;
	}
	case VOE_BASE_FIELD_INT32: {
		int32_t value;

		memcpy(&value, bytes, sizeof(value));
		put_signed(text, value);
		return true;
	}
	case VOE_BASE_FIELD_INT64: {
		int64_t value;

		memcpy(&value, bytes, sizeof(value));
		put_signed(text, value);
		return true;
	}
	case VOE_BASE_FIELD_UINT8:
		voe_authoring_put_unsigned(text, bytes[0]);
		return true;
	case VOE_BASE_FIELD_UINT16: {
		uint16_t value;

		memcpy(&value, bytes, sizeof(value));
		voe_authoring_put_unsigned(text, value);
		return true;
	}
	case VOE_BASE_FIELD_UINT32: {
		uint32_t value;

		memcpy(&value, bytes, sizeof(value));
		voe_authoring_put_unsigned(text, value);
		return true;
	}
	case VOE_BASE_FIELD_UINT64: {
		uint64_t value;

		memcpy(&value, bytes, sizeof(value));
		voe_authoring_put_unsigned(text, value);
		return true;
	}
	case VOE_BASE_FIELD_FLOAT32:
		return put_floats(text, site, bytes, 1);
	case VOE_BASE_FIELD_FLOAT64: {
		double value;

		memcpy(&value, bytes, sizeof(value));
		if (!finite_or_refuse(text, site, value))
			return false;
		put_shortest(text, value, false);
		return true;
	}
	case VOE_BASE_FIELD_BOOL:
		voe_authoring_put_string(text,
					 bytes[0] != 0 ? "true" : "false");
		return true;
	case VOE_BASE_FIELD_FLOAT2:
		return put_floats(text, site, bytes, 2);
	case VOE_BASE_FIELD_FLOAT3:
	case VOE_BASE_FIELD_COLOUR:
		return put_floats(text, site, bytes, 3);
	case VOE_BASE_FIELD_DOUBLE3:
		return put_double3(text, site, bytes);
	case VOE_BASE_FIELD_FLOAT4:
	case VOE_BASE_FIELD_QUAT:
		return put_floats(text, site, bytes, 4);
	case VOE_BASE_FIELD_FLOAT4X4:
		return put_floats(text, site, bytes, 16);
	case VOE_BASE_FIELD_ENTITY:
		put_entity(text, site, bytes);
		return true;
	case VOE_BASE_FIELD_ENUM:
		if (voe_authoring_measuring(text))
			VOE_BASE_ERROR(MODULE,
				       "entity %" PRIu64 ", %s.%s is an enum, and "
				       "no enum value has a name to be written "
				       "with yet; nothing was written",
				       site->id, site->component, site->field);
		return false;
	case VOE_BASE_FIELD_CHAR:
		break;
	}

	VOE_BASE_ASSERT(false, "a field kind this writer does not know");
	return false;
}

// The bytes up to the first NUL, quoted, with `"` and `\` escaped.
static bool put_chars(voe_authoring_output *text,
		      const voe_authoring_value_site *site,
		      const uint8_t *bytes, uint32_t count)
{
	voe_authoring_put_string(text, "\"");
	for (uint32_t i = 0; i < count && bytes[i] != 0; i++) {
		if (bytes[i] < 0x20) {
			if (voe_authoring_measuring(text))
				VOE_BASE_ERROR(MODULE,
					       "entity %" PRIu64 ", %s.%s holds "
					       "byte 0x%02x at %u, and a scene "
					       "file cannot carry a control "
					       "character; nothing was written",
					       site->id, site->component,
					       site->field, bytes[i], i);
			return false;
		}
		if (bytes[i] == '"' || bytes[i] == '\\')
			voe_authoring_put_string(text, "\\");
		voe_authoring_put(text, (const char *)bytes + i, 1);
	}
	voe_authoring_put_string(text, "\"");
	return true;
}

// Levels 0 to `bracket_rank` (exclusive) of a field's own brackets — every
// dimension but, for CHAR, the innermost, which is the string's own bytes and
// not a level (ADR-0154 point 8). `strides[level]` is the byte span of one
// whole item at `level`; `strides[bracket_rank]` is one leaf: one kind element,
// or for CHAR the string's own byte count.
static bool put_shape(voe_authoring_output *text,
		      voe_authoring_value_site *site,
		      const voe_base_field_description *field,
		      uint32_t bracket_rank, const size_t *strides,
		      uint32_t level, const uint8_t *bytes)
{
	if (level == bracket_rank) {
		if (field->kind == VOE_BASE_FIELD_CHAR)
			return put_chars(text, site, bytes,
					 field->dims[field->rank - 1]);
		return put_element(text, site, field->kind, bytes);
	}

	char base[sizeof(site->field)];
	uint32_t count = field->dims[level];

	memcpy(base, site->field, sizeof(base));
	voe_authoring_put_string(text, "[");
	for (uint32_t i = 0; i < count; i++) {
		if (i > 0)
			voe_authoring_put_string(text, ", ");
		(void)snprintf(site->field, sizeof(site->field), "%s[%u]",
			       base, i);
		if (!put_shape(text, site, field, bracket_rank, strides,
			       level + 1, bytes + i * strides[level + 1]))
			return false;
	}
	voe_authoring_put_string(text, "]");
	return true;
}

bool voe_authoring_value_put_field(voe_authoring_output *text,
				   voe_authoring_value_site *site,
				   const voe_base_field_description *field,
				   const uint8_t *row)
{
	const uint8_t *bytes = row + field->offset;
	// Every dimension is a bracket level, except for CHAR, whose innermost
	// dimension is the string's own bytes rather than an array of them
	// (ADR-0154 point 8).
	uint32_t bracket_rank = field->kind == VOE_BASE_FIELD_CHAR
					 ? field->rank - 1
					 : field->rank;
	size_t strides[VOE_BASE_FIELD_RANK_MAX + 1];

	VOE_BASE_ASSERT(field->count > 0, "a field of no elements");
	VOE_BASE_ASSERT(field->kind != VOE_BASE_FIELD_CHAR || field->rank >= 1,
			"a CHAR field with no length dimension");
	(void)snprintf(site->field, sizeof(site->field), "%s", field->name);
	voe_authoring_put_string(text, field->name);
	voe_authoring_put_string(text, " = ");

	strides[bracket_rank] = field->kind == VOE_BASE_FIELD_CHAR
					 ? field->dims[field->rank - 1]
					 : field->size / field->count;
	for (uint32_t l = bracket_rank; l > 0; l--)
		strides[l - 1] = strides[l] * field->dims[l - 1];

	if (!put_shape(text, site, field, bracket_rank, strides, 0, bytes))
		return false;

	voe_authoring_put_string(text, "\n");
	return true;
}

// DEVIATION: card 070 scope 3, a described type with its description compiled
// out is refused as well as the four listed, because ecs/component.h names
// refusing loudly as what a writer does rather than saving nothing.
bool voe_authoring_value_described_or_refuse(
	const voe_authoring_output *text, const voe_authoring_value_site *site,
	const voe_base_struct_description *description)
{
	if (description != NULL)
		return true;

	if (voe_authoring_measuring(text))
		VOE_BASE_ERROR(MODULE,
			       "entity %" PRIu64 " has %s, whose description "
			       "this build compiled out, and saving without it "
			       "would lose the component; nothing was written",
			       site->id, site->component);
	return false;
}
