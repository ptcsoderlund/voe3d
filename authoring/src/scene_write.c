// The scene writer. One walk over the authored entities, twice.
//
// TWO PASSES OVER THE SAME WALK, AND THAT IS HOW THE TEXT IS ONE PUSH OF EXACTLY
// ITS SIZE. Two pushes are not guaranteed to be adjacent (base/arena.h), so text
// that grew a push at a time could not be handed back as one span. The first pass
// measures — every put() adds to a count and writes nothing — and it is also the
// pass that refuses and reports; the second pass writes into a buffer pushed at
// that size. Both passes go through put_scene(), so they cannot disagree about a
// byte, and the second cannot refuse anything the first accepted.
//
// ORDER IS SORTED OUT BEFORE ANYTHING IS WRITTEN. The authored entities are
// copied out of the identity table and merge-sorted by id (authored.h), the
// component types are sorted by key name, and a copy of the kept sections by id
// and then key name. None uses qsort, whose comparator is a function pointer, and
// function pointers in this engine are render's loader table alone. The ids are
// merge-sorted because a scene may hold thousands; the types are insertion-sorted
// because a world holds a handful; the kept sections are insertion-sorted because
// they arrive from a file this writer wrote, already in that order, and insertion
// sort is one comparison per item on sorted input.
//
// A KEPT SECTION IS MERGED INTO ITS ENTITY'S COMPONENT SECTIONS AS THEY ARE
// WRITTEN, two sorted runs walked side by side, so it lands where its key name
// sorts without the described types and the kept ones ever sharing an array.
//
// THE IDENTITY TYPE IS FOUND BY WALKING THE TYPES, not with
// voe_ecs_component_type, which asserts on a world that registered none — and a
// world with no identities is an ordinary world with nothing authored in it.
//
// A FIELD'S SHAPE IS WALKED BY RECURSION, NOT AN EXPLICIT STACK (ADR-0154). The
// nesting here is over a field's own rank and dims — the program's own
// description, compiled in, never read from a file — so rule 14 (no recursion
// over data read from a file) does not bind; put_shape() recurses at most
// VOE_BASE_FIELD_RANK_MAX deep, a fixed bound the type checks at compile time.
#include <authoring/scene_write.h>

#include "authored.h"

#include <base/assert.h>
#include <base/describe.h>
#include <base/report.h>
#include <ecs/component.h>
#include <scene/identity_component.h>

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODULE "authoring"

// The most significant digits %g needs for any float, and for any double, to
// read back to the same bits.
#define FLOAT32_DIGITS_MAX 9
#define FLOAT64_DIGITS_MAX 17

struct text {
	// NULL on the measuring pass, which counts and writes nothing.
	char *bytes;
	size_t size;
};

struct described {
	voe_ecs_type type;
	const char *key;
	// NULL when this build compiled the description out.
	const voe_base_struct_description *description;
};

struct scene {
	const voe_ecs_world *world;

	bool has_identity;
	voe_ecs_type identity;

	// Ascending by id.
	voe_authoring_authored *authored;
	uint32_t authored_count;

	// Every described type but the identity, ascending by key name.
	struct described *described;
	uint32_t described_count;

	// A copy of the kept sections, ascending by id and then by key name.
	voe_authoring_kept_section *kept;
	uint32_t kept_count;
};

// Where a value sits, for a report: which entity, which component, which field.
// `field` is a buffer and not a pointer because an element inside an array
// names its index too — `tags[1]` — built up as put_shape() descends and torn
// down as it returns, one field at a time, never two at once.
struct site {
	const struct scene *scene;
	uint64_t id;
	const char *component;
	char field[128];
};

static bool measuring(const struct text *text)
{
	return text->bytes == NULL;
}

static void put(struct text *text, const char *bytes, size_t size)
{
	if (!measuring(text))
		memcpy(text->bytes + text->size, bytes, size);
	text->size += size;
}

static void put_string(struct text *text, const char *string)
{
	put(text, string, strlen(string));
}

static void put_signed(struct text *text, int64_t value)
{
	char digits[24];

	(void)snprintf(digits, sizeof(digits), "%" PRId64, value);
	put_string(text, digits);
}

static void put_unsigned(struct text *text, uint64_t value)
{
	char digits[24];

	(void)snprintf(digits, sizeof(digits), "%" PRIu64, value);
	put_string(text, digits);
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
static void put_shortest(struct text *text, double value, bool single)
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
		put_string(text, digits);
		return;
	}

	// Enough decimals to keep `precision` significant digits, rounded at the
	// same place %g rounded them, so both spellings carry the same digits.
	long exponent = strtol(e + 1, NULL, 10);
	long decimals = precision - 1 - exponent;

	(void)snprintf(plain, sizeof(plain), "%.*f",
		       (int)(decimals > 0 ? decimals : 0), value);
	put_string(text, strlen(plain) <= strlen(digits) ? plain : digits);
}

static bool finite_or_refuse(const struct text *text, const struct site *site,
			     double value)
{
	if (isfinite(value))
		return true;

	if (measuring(text))
		VOE_BASE_ERROR(MODULE,
			       "entity %" PRIu64 ", %s.%s: %g is not finite, and "
			       "a scene file has no decimal for it; nothing was "
			       "written",
			       site->id, site->component, site->field, value);
	return false;
}

// `count` floats, bracketed when there is more than one.
static bool put_floats(struct text *text, const struct site *site,
		       const uint8_t *bytes, uint32_t count)
{
	if (count > 1)
		put_string(text, "[");
	for (uint32_t i = 0; i < count; i++) {
		float value;

		memcpy(&value, bytes + i * sizeof(value), sizeof(value));
		if (!finite_or_refuse(text, site, value))
			return false;
		if (i > 0)
			put_string(text, ", ");
		put_shortest(text, value, true);
	}
	if (count > 1)
		put_string(text, "]");
	return true;
}

static void put_entity(struct text *text, const struct site *site,
		       const uint8_t *bytes)
{
	const struct scene *scene = site->scene;
	voe_ecs_entity target;
	const voe_scene_identity *identity;

	memcpy(&target, bytes, sizeof(target));
	if (!voe_ecs_entity_alive(scene->world, target)) {
		put_string(text, "0");
		return;
	}

	// An entity with a field being written is authored, so the identity type
	// is registered.
	identity = voe_ecs_component_get(scene->world, scene->identity, target);
	if (identity != NULL) {
		put_unsigned(text, identity->id);
		return;
	}

	if (measuring(text))
		VOE_BASE_WARNING(MODULE,
				 "entity %" PRIu64 ", %s.%s names entity %uv%u, "
				 "which has no identity and will not be in the "
				 "file; written as 0",
				 site->id, site->component, site->field,
				 target.index, target.generation);
	put_string(text, "0");
}

// One element of a field that is not CHAR.
static bool put_element(struct text *text, const struct site *site,
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
		put_unsigned(text, bytes[0]);
		return true;
	case VOE_BASE_FIELD_UINT16: {
		uint16_t value;

		memcpy(&value, bytes, sizeof(value));
		put_unsigned(text, value);
		return true;
	}
	case VOE_BASE_FIELD_UINT32: {
		uint32_t value;

		memcpy(&value, bytes, sizeof(value));
		put_unsigned(text, value);
		return true;
	}
	case VOE_BASE_FIELD_UINT64: {
		uint64_t value;

		memcpy(&value, bytes, sizeof(value));
		put_unsigned(text, value);
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
		put_string(text, bytes[0] != 0 ? "true" : "false");
		return true;
	case VOE_BASE_FIELD_FLOAT2:
		return put_floats(text, site, bytes, 2);
	case VOE_BASE_FIELD_FLOAT3:
		return put_floats(text, site, bytes, 3);
	case VOE_BASE_FIELD_FLOAT4:
	case VOE_BASE_FIELD_QUAT:
		return put_floats(text, site, bytes, 4);
	case VOE_BASE_FIELD_FLOAT4X4:
		return put_floats(text, site, bytes, 16);
	case VOE_BASE_FIELD_ENTITY:
		put_entity(text, site, bytes);
		return true;
	case VOE_BASE_FIELD_ENUM:
		if (measuring(text))
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
static bool put_chars(struct text *text, const struct site *site,
		      const uint8_t *bytes, uint32_t count)
{
	put_string(text, "\"");
	for (uint32_t i = 0; i < count && bytes[i] != 0; i++) {
		if (bytes[i] < 0x20) {
			if (measuring(text))
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
			put_string(text, "\\");
		put(text, (const char *)bytes + i, 1);
	}
	put_string(text, "\"");
	return true;
}

// Levels 0 to `bracket_rank` (exclusive) of a field's own brackets — every
// dimension but, for CHAR, the innermost, which is the string's own bytes and
// not a level (ADR-0154 point 8). `strides[level]` is the byte span of one
// whole item at `level`; `strides[bracket_rank]` is one leaf: one kind element,
// or for CHAR the string's own byte count.
static bool put_shape(struct text *text, struct site *site,
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
	put_string(text, "[");
	for (uint32_t i = 0; i < count; i++) {
		if (i > 0)
			put_string(text, ", ");
		(void)snprintf(site->field, sizeof(site->field), "%s[%u]",
			       base, i);
		if (!put_shape(text, site, field, bracket_rank, strides,
			       level + 1, bytes + i * strides[level + 1]))
			return false;
	}
	put_string(text, "]");
	return true;
}

static bool put_field(struct text *text, struct site *site,
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
	put_string(text, field->name);
	put_string(text, " = ");

	strides[bracket_rank] = field->kind == VOE_BASE_FIELD_CHAR
					 ? field->dims[field->rank - 1]
					 : field->size / field->count;
	for (uint32_t l = bracket_rank; l > 0; l--)
		strides[l - 1] = strides[l] * field->dims[l - 1];

	if (!put_shape(text, site, field, bracket_rank, strides, 0, bytes))
		return false;

	put_string(text, "\n");
	return true;
}

// DEVIATION: card 070 scope 3, a described type with its description compiled
// out is refused as well as the four listed, because ecs/component.h names
// refusing loudly as what a writer does rather than saving nothing.
static bool described_or_refuse(const struct text *text, const struct site *site,
				const voe_base_struct_description *description)
{
	if (description != NULL)
		return true;

	if (measuring(text))
		VOE_BASE_ERROR(MODULE,
			       "entity %" PRIu64 " has %s, whose description "
			       "this build compiled out, and saving without it "
			       "would lose the component; nothing was written",
			       site->id, site->component);
	return false;
}

static bool registered(const voe_ecs_world *world, const char *name)
{
	uint32_t count = voe_ecs_component_type_count(world);

	for (uint32_t i = 0; i < count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);

		if (strcmp(voe_ecs_component_key(world, type)->name, name) == 0)
			return true;
	}
	return false;
}

// DEVIATION: card 071 scope 3, a kept section whose key name some registered type
// now has is refused, which the card does not list. It was kept because nothing
// had registered that name; written back beside that type's own section it is the
// same section twice, and beside a runtime-only one it is a section the reader
// refuses — either way a file that cannot be read back as what was saved, which is
// what this writer refuses rather than writes.
static bool put_kept(struct text *text, const struct scene *scene,
		     const voe_authoring_kept_section *kept)
{
	if (registered(scene->world, kept->key)) {
		if (measuring(text))
			VOE_BASE_ERROR(MODULE,
				       "entity %" PRIu64 " has a kept section %s, "
				       "and a type has been registered under that "
				       "name since; writing it would give a file "
				       "that cannot be read back; nothing was "
				       "written",
				       kept->id, kept->key);
		return false;
	}

	put_string(text, "[");
	put_unsigned(text, kept->id);
	put_string(text, ".");
	put_string(text, kept->key);
	put_string(text, "]\n");
	put(text, kept->lines, kept->size);
	return true;
}

// `kept` is this entity's kept sections, `kept_count` of them, ascending by key
// name.
static bool put_entity_block(struct text *text, const struct scene *scene,
			     const voe_authoring_authored *authored,
			     const voe_authoring_kept_section *kept,
			     uint32_t kept_count)
{
	const voe_ecs_world *world = scene->world;
	const voe_base_struct_description *identity_description =
		voe_ecs_component_description(world, scene->identity);
	struct site site = {
		.scene = scene,
		.id = authored->id,
		.component = voe_ecs_component_key(world, scene->identity)->name,
	};
	const uint8_t *row;

	if (!described_or_refuse(text, &site, identity_description))
		return false;

	put_string(text, "[");
	put_unsigned(text, authored->id);
	put_string(text, "]\n");

	row = voe_ecs_component_get(world, scene->identity, authored->entity);
	for (uint32_t f = 0; f < identity_description->field_count; f++) {
		const voe_base_field_description *field =
			&identity_description->fields[f];

		if (field->offset == offsetof(voe_scene_identity, id))
			continue;
		if (!put_field(text, &site, field, row))
			return false;
	}

	uint32_t k = 0;

	for (uint32_t t = 0; t < scene->described_count; t++) {
		const struct described *described = &scene->described[t];

		row = voe_ecs_component_get(world, described->type,
					    authored->entity);
		if (row == NULL)
			continue;

		for (; k < kept_count && strcmp(kept[k].key, described->key) < 0;
		     k++)
			if (!put_kept(text, scene, &kept[k]))
				return false;

		site.component = described->key;
		site.field[0] = '\0';
		if (!described_or_refuse(text, &site, described->description))
			return false;

		put_string(text, "[");
		put_unsigned(text, authored->id);
		put_string(text, ".");
		put_string(text, described->key);
		put_string(text, "]\n");
		for (uint32_t f = 0; f < described->description->field_count;
		     f++)
			if (!put_field(text, &site,
				       &described->description->fields[f], row))
				return false;
	}
	for (; k < kept_count; k++)
		if (!put_kept(text, scene, &kept[k]))
			return false;
	return true;
}

static void drop_kept(const struct text *text,
		      const voe_authoring_kept_section *kept)
{
	if (measuring(text))
		VOE_BASE_WARNING(MODULE,
				 "the kept section [%" PRIu64 ".%s] belongs to "
				 "authored id %" PRIu64 ", which no entity in the "
				 "world has now; it is dropped",
				 kept->id, kept->key, kept->id);
}

static bool put_scene(struct text *text, const struct scene *scene)
{
	uint32_t k = 0;

	for (uint32_t i = 0; i < scene->authored_count; i++) {
		const voe_authoring_authored *authored = &scene->authored[i];
		uint32_t end;

		for (; k < scene->kept_count && scene->kept[k].id < authored->id;
		     k++)
			drop_kept(text, &scene->kept[k]);
		end = k;
		while (end < scene->kept_count &&
		       scene->kept[end].id == authored->id)
			end++;

		if (i > 0)
			put_string(text, "\n");
		if (!put_entity_block(text, scene, authored, scene->kept + k,
				      end - k))
			return false;
		k = end;
	}
	for (; k < scene->kept_count; k++)
		drop_kept(text, &scene->kept[k]);
	return true;
}

static void sort_by_key(struct described *items, uint32_t count)
{
	for (uint32_t i = 1; i < count; i++) {
		struct described item = items[i];
		uint32_t j = i;

		while (j > 0 && strcmp(items[j - 1].key, item.key) > 0) {
			items[j] = items[j - 1];
			j--;
		}
		items[j] = item;
	}
}

static bool kept_before(const voe_authoring_kept_section *a,
			const voe_authoring_kept_section *b)
{
	return a->id < b->id || (a->id == b->id && strcmp(a->key, b->key) < 0);
}

static void sort_kept(voe_authoring_kept_section *items, uint32_t count)
{
	for (uint32_t i = 1; i < count; i++) {
		voe_authoring_kept_section item = items[i];
		uint32_t j = i;

		while (j > 0 && kept_before(&item, &items[j - 1])) {
			items[j] = items[j - 1];
			j--;
		}
		items[j] = item;
	}
}

static void gather_kept(struct scene *scene, const voe_authoring_kept *kept,
			voe_base_arena *arena)
{
	if (kept == NULL || kept->count == 0)
		return;

	scene->kept = voe_base_arena_push(arena,
					  kept->count * sizeof(*scene->kept));
	memcpy(scene->kept, kept->sections, kept->count * sizeof(*scene->kept));
	scene->kept_count = kept->count;
	sort_kept(scene->kept, scene->kept_count);
}

static void gather(struct scene *scene, const voe_ecs_world *world,
		   const voe_authoring_kept *kept, voe_base_arena *arena)
{
	uint32_t type_count = voe_ecs_component_type_count(world);

	*scene = (struct scene){ .world = world };
	gather_kept(scene, kept, arena);
	if (type_count > 0)
		scene->described = voe_base_arena_push(
			arena, type_count * sizeof(*scene->described));

	for (uint32_t i = 0; i < type_count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(world, i);
		const struct voe_ecs_key *key = voe_ecs_component_key(world, type);

		if (key == &voe_scene_identity_key) {
			scene->has_identity = true;
			scene->identity = type;
			continue;
		}
		if (voe_ecs_component_runtime_only(world, type))
			continue;

		scene->described[scene->described_count++] = (struct described){
			.type = type,
			.key = key->name,
			.description = voe_ecs_component_description(world, type),
		};
	}
	sort_by_key(scene->described, scene->described_count);
	for (uint32_t i = 1; i < scene->described_count; i++)
		VOE_BASE_ASSERT(strcmp(scene->described[i - 1].key,
				       scene->described[i].key) != 0,
				"two component types registered under one key name");

	if (!scene->has_identity)
		return;

	uint32_t count = voe_ecs_component_count(world, scene->identity);

	if (count == 0)
		return;

	const voe_scene_identity *rows =
		voe_ecs_component_rows(world, scene->identity);
	const voe_ecs_entity *entities =
		voe_ecs_component_entities(world, scene->identity);
	voe_authoring_authored *scratch =
		voe_base_arena_push(arena, count * sizeof(*scratch));

	scene->authored = voe_base_arena_push(arena,
					      count * sizeof(*scene->authored));
	scene->authored_count = count;
	for (uint32_t i = 0; i < count; i++)
		scene->authored[i] = (voe_authoring_authored){
			.id = rows[i].id,
			.entity = entities[i],
		};
	voe_authoring_authored_sort(scene->authored, scratch, count);
}

static bool ids_unique_or_refuse(const struct scene *scene)
{
	for (uint32_t i = 1; i < scene->authored_count; i++) {
		const voe_authoring_authored *a = &scene->authored[i - 1];
		const voe_authoring_authored *b = &scene->authored[i];

		if (a->id != b->id)
			continue;
		VOE_BASE_ERROR(MODULE,
			       "entities %uv%u and %uv%u both have authored id "
			       "%" PRIu64 ", and a scene file cannot tell them "
			       "apart; nothing was written",
			       a->entity.index, a->entity.generation,
			       b->entity.index, b->entity.generation, a->id);
		return false;
	}
	return true;
}

bool voe_authoring_scene_write(const voe_ecs_world *world,
			       const voe_authoring_kept *kept,
			       voe_base_arena *arena, voe_authoring_text *out)
{
	struct scene scene;
	struct text measured = { 0 };
	struct text written;

	VOE_BASE_ASSERT(world != NULL, "writing a NULL world");
	VOE_BASE_ASSERT(arena != NULL, "writing into a NULL arena");
	VOE_BASE_ASSERT(out != NULL, "nowhere to put the text");

	gather(&scene, world, kept, arena);
	if (!ids_unique_or_refuse(&scene))
		return false;
	if (!put_scene(&measured, &scene))
		return false;

	written = (struct text){
		.bytes = voe_base_arena_push(arena, measured.size + 1),
	};
	bool accepted = put_scene(&written, &scene);

	VOE_BASE_ASSERT(accepted && written.size == measured.size,
			"the writing pass disagreed with the measuring pass");
	written.bytes[written.size] = '\0';

	*out = (voe_authoring_text){
		.text = written.bytes,
		.size = written.size,
	};
	return true;
}
