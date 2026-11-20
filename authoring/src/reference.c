// Reference text written from the readers' own tables — authoring/reference.h
// says what each text holds; this file says how it is built.
//
// BOTH TEXTS ARE WRITTEN TWICE, AS THE SCENE WRITER IS: a measuring pass counts
// the bytes through voe_authoring_output (value_write.h) and reports any refusal,
// then a pass of exactly that size writes them. Nothing grows and nothing is
// reported twice.
//
// WHAT A MISSING KEY READS AS IS THE READER'S FALLBACK, SPELLED BY THE WRITER.
// The row is the type's unsaid row, else its default row, else zeros, as
// scene_read.c fills a field a section does not say (ecs/component.h, 0324);
// voe_authoring_value_put_field spells that field's bytes, and the `name = `
// before them and the newline after are dropped. An ENUM field has no spelling
// — the reader refuses one — so its cell says so instead.
//
// A TABLE CELL ESCAPES `|`, the one byte that would split a Markdown table row;
// every string that comes from a description, a key table or a value goes
// through put_cell.
//
// THE TYPES ARE INSERTION-SORTED BY KEY NAME, because a world registers a handful
// and the order a program registered them in is its startup's, not the text's.
#include <authoring/reference.h>

#include "value_write.h"

#include <base/assert.h>
#include <base/report.h>
#include <ecs/component.h>
#include <scene/identity_component.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define MODULE "authoring"

struct reference {
	const voe_ecs_world *world;
	bool has_identity;
	voe_ecs_type identity;
	// Every registered type that is not runtime-only, sorted by key name.
	voe_ecs_type *types;
	uint32_t count;
	voe_base_arena *arena;
};

static const char *key_of(const struct reference *ref, voe_ecs_type type)
{
	return voe_ecs_component_key(ref->world, type)->name;
}

// `string` as a table cell: every `|` escaped.
static void put_cell(voe_authoring_output *text, const char *string, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (string[i] == '|')
			voe_authoring_put_string(text, "\\");
		voe_authoring_put(text, string + i, 1);
	}
}

static void put_cell_string(voe_authoring_output *text, const char *string)
{
	put_cell(text, string, strlen(string));
}

static void put_number(voe_authoring_output *text, double value)
{
	char digits[32];

	(void)snprintf(digits, sizeof(digits), "%g", value);
	voe_authoring_put_string(text, digits);
}

// `least to greatest unit`.
static void put_range(voe_authoring_output *text, double least, double greatest,
		      const char *unit)
{
	put_number(text, least);
	voe_authoring_put_string(text, " to ");
	put_number(text, greatest);
	if (unit[0] != '\0') {
		voe_authoring_put_string(text, " ");
		put_cell_string(text, unit);
	}
}

static const char *element_phrase(voe_base_field_kind kind)
{
	switch (kind) {
	case VOE_BASE_FIELD_INT8:
	case VOE_BASE_FIELD_INT16:
	case VOE_BASE_FIELD_INT32:
	case VOE_BASE_FIELD_INT64:
		return "whole number";
	case VOE_BASE_FIELD_UINT8:
	case VOE_BASE_FIELD_UINT16:
	case VOE_BASE_FIELD_UINT32:
	case VOE_BASE_FIELD_UINT64:
		return "whole number, 0 or more";
	case VOE_BASE_FIELD_FLOAT32:
		return "number";
	case VOE_BASE_FIELD_FLOAT64:
		return "number, double precision";
	case VOE_BASE_FIELD_BOOL:
		return "true or false";
	case VOE_BASE_FIELD_FLOAT2:
		return "`[x, y]`";
	case VOE_BASE_FIELD_FLOAT3:
		return "`[x, y, z]`";
	case VOE_BASE_FIELD_COLOUR:
		return "linear colour `[r, g, b]`, each 0 to 1";
	case VOE_BASE_FIELD_DOUBLE3:
		return "world position `[x, y, z]`, double precision";
	case VOE_BASE_FIELD_FLOAT4:
		return "`[x, y, z, w]`";
	case VOE_BASE_FIELD_QUAT:
		return "rotation quaternion `[x, y, z, w]`";
	case VOE_BASE_FIELD_FLOAT4X4:
		return "16 numbers in one bracket, row by row";
	case VOE_BASE_FIELD_ENUM:
		return "enum, which the reader refuses: no value has a name yet";
	case VOE_BASE_FIELD_CHAR:
		return "quoted text";
	case VOE_BASE_FIELD_ENTITY:
		return "an entity's authored id, 0 for none";
	}
	VOE_BASE_ASSERT(false, "a field kind this reference does not know");
	return "";
}

// The kind phrase: the array around it, the element, a string's room, and what
// each named value is called.
static void put_kind(voe_authoring_output *text,
		     const voe_base_struct_description *description,
		     const voe_base_field_description *field)
{
	bool chars = field->kind == VOE_BASE_FIELD_CHAR;
	uint32_t levels = chars ? field->rank - 1 : field->rank;

	if (levels > 0) {
		voe_authoring_put_string(text, "array of ");
		for (uint32_t l = 0; l < levels; l++) {
			if (l > 0)
				voe_authoring_put_string(text, " × ");
			voe_authoring_put_unsigned(text, field->dims[l]);
		}
		voe_authoring_put_string(text, " nested brackets, each ");
	}
	voe_authoring_put_string(text, element_phrase(field->kind));
	if (chars) {
		voe_authoring_put_string(text, ", at most ");
		voe_authoring_put_unsigned(text, field->dims[field->rank - 1] - 1);
		voe_authoring_put_string(text, " bytes");
	}

	const voe_base_field_names *names =
		voe_base_names_find(description, field->name);
	bool first = true;

	for (uint32_t v = 0; names != NULL && v < names->value_count; v++) {
		if (names->values[v] == NULL)
			continue;
		voe_authoring_put_string(text, first ? "; " : ", ");
		voe_authoring_put_unsigned(text, v);
		voe_authoring_put_string(text, " is ");
		put_cell_string(text, names->values[v]);
		first = false;
	}
}

// What a section that leaves `field` out reads it as, in backticks.
static bool put_missing(voe_authoring_output *text, const struct reference *ref,
			voe_ecs_type type,
			const voe_base_field_description *field,
			const uint8_t *row)
{
	if (field->kind == VOE_BASE_FIELD_ENUM) {
		voe_authoring_put_string(text, "—");
		return true;
	}

	voe_authoring_value_site site = {
		.world = ref->world,
		.identity = ref->identity,
		.component = key_of(ref, type),
	};
	voe_authoring_output measured = { 0 };

	if (!voe_authoring_value_put_field(&measured, &site, field, row))
		return false;

	voe_authoring_output spelled = {
		.bytes = voe_base_arena_push(ref->arena, measured.size),
	};
	bool accepted =
		voe_authoring_value_put_field(&spelled, &site, field, row);
	size_t skip = strlen(field->name) + strlen(" = ");

	VOE_BASE_ASSERT(accepted && spelled.size == measured.size &&
				spelled.size > skip,
			"the spelling pass disagreed with the measuring pass");
	voe_authoring_put_string(text, "`");
	put_cell(text, spelled.bytes + skip, spelled.size - skip - 1);
	voe_authoring_put_string(text, "`");
	return true;
}

// The bytes a field the section does not say starts from: the unsaid row, else
// the default row, else zeros as long as the description reaches.
static const uint8_t *missing_row(const struct reference *ref,
				  voe_ecs_type type,
				  const voe_base_struct_description *description)
{
	const uint8_t *row = voe_ecs_component_unsaid(ref->world, type);

	if (row == NULL)
		row = voe_ecs_component_default(ref->world, type);
	if (row != NULL)
		return row;

	size_t end = 1;

	for (uint32_t f = 0; f < description->field_count; f++)
		if (description->fields[f].offset + description->fields[f].size >
		    end)
			end = description->fields[f].offset +
			      description->fields[f].size;
	return voe_base_arena_push(ref->arena, end);
}

// The menu, needs and former names, each a bullet where the type has one.
static void put_notes(voe_authoring_output *text, const struct reference *ref,
		      voe_ecs_type type)
{
	const char *menu = voe_ecs_component_menu(ref->world, type);
	voe_ecs_type needed;
	uint32_t former_count;
	const voe_ecs_former_name *former =
		voe_ecs_component_formerly(ref->world, type, &former_count);
	bool any = false;

	if (menu != NULL) {
		voe_authoring_put_string(text, "- Menu: ");
		voe_authoring_put_string(text, menu);
		voe_authoring_put_string(text, ".\n");
		any = true;
	}
	if (voe_ecs_component_needs(ref->world, type, &needed)) {
		voe_authoring_put_string(text, "- Needs `");
		voe_authoring_put_string(text, key_of(ref, needed));
		voe_authoring_put_string(text, "` on the same entity.\n");
		any = true;
	}
	for (uint32_t i = 0; i < former_count; i++) {
		voe_authoring_put_string(text, "- Former name: `");
		voe_authoring_put_string(text, former[i].former);
		voe_authoring_put_string(text, "` is read as `");
		voe_authoring_put_string(text, former[i].field);
		voe_authoring_put_string(text, "`.\n");
		any = true;
	}
	if (any)
		voe_authoring_put_string(text, "\n");
}

// `## <type key>`, its notes and its field table.
static bool put_type(voe_authoring_output *text, const struct reference *ref,
		     voe_ecs_type type)
{
	const voe_base_struct_description *description =
		voe_ecs_component_description(ref->world, type);
	bool entity = ref->has_identity && type.value == ref->identity.value;
	const uint8_t *row = missing_row(ref, type, description);

	voe_authoring_put_string(text, "## ");
	voe_authoring_put_string(text, key_of(ref, type));
	voe_authoring_put_string(text, "\n\n");
	if (entity)
		voe_authoring_put_string(text,
					 "Its keys are written under `[N]`, not "
					 "in a section of their own, and `id` is "
					 "the N; `[N.voe_scene_identity]` is "
					 "refused.\n\n");
	put_notes(text, ref, type);
	voe_authoring_put_string(text,
				 "| Key | Kind | Range | Missing reads as |\n"
				 "|---|---|---|---|\n");

	for (uint32_t f = 0; f < description->field_count; f++) {
		const voe_base_field_description *field = &description->fields[f];
		const voe_base_field_range *range =
			voe_base_ranges_find(description, field->name);

		if (entity && field->offset == offsetof(voe_scene_identity, id))
			continue;
		voe_authoring_put_string(text, "| `");
		voe_authoring_put_string(text, field->name);
		voe_authoring_put_string(text, "`");
		if (field->read_only)
			voe_authoring_put_string(text,
						 ", the editor's: shown, not "
						 "edited");
		voe_authoring_put_string(text, " | ");
		put_kind(text, description, field);
		voe_authoring_put_string(text, " | ");
		if (range != NULL)
			put_range(text, range->least, range->greatest,
				  range->unit);
		else
			voe_authoring_put_string(text, "—");
		voe_authoring_put_string(text, " | ");
		if (!put_missing(text, ref, type, field, row))
			return false;
		voe_authoring_put_string(text, " |\n");
	}
	voe_authoring_put_string(text, "\n");
	return true;
}

// `[N]`'s keys: the identity's fields other than `id`.
static void put_entity_keys(voe_authoring_output *text,
			    const struct reference *ref)
{
	if (!ref->has_identity)
		return;

	const voe_base_struct_description *description =
		voe_ecs_component_description(ref->world, ref->identity);
	bool first = true;

	voe_authoring_put_string(text, " Its keys are");
	for (uint32_t f = 0; f < description->field_count; f++) {
		const voe_base_field_description *field = &description->fields[f];

		if (field->offset == offsetof(voe_scene_identity, id))
			continue;
		voe_authoring_put_string(text, first ? " `" : ", `");
		voe_authoring_put_string(text, field->name);
		voe_authoring_put_string(text, "`");
		first = false;
	}
	voe_authoring_put_string(text, ", as `voe_scene_identity` below lists.");
}

static bool put_scene(voe_authoring_output *text, const struct reference *ref)
{
	voe_authoring_put_string(
		text,
		"## The text form\n\n"
		"A `.scene` or `.prefab` file is sections of `key = value` "
		"lines. This text is written from the component descriptions "
		"the reader reads by, so it is what the strict read "
		"enforces.\n\n"
		"- `[N]` is an entity with authored id N, a decimal from 1 "
		"with no leading zero.");
	put_entity_keys(text, ref);
	voe_authoring_put_string(
		text,
		"\n"
		"- `[N.<type key>]` is a component of entity N, and the file "
		"must also hold `[N]`. Its keys are the fields under that "
		"type's heading below.\n"
		"- A key that is not listed, as a field or a former name, is "
		"refused, with its line; so is a value outside its range.\n"
		"- A section of a type nothing registered passes unread.\n"
		"- A key a section leaves out reads as its row's last column "
		"says.\n\n");

	for (uint32_t i = 0; i < ref->count; i++)
		if (!put_type(text, ref, ref->types[i]))
			return false;
	return true;
}

// Every type that is not runtime-only, sorted by key name; false, reported,
// for one whose description this build compiled out.
static bool gather_or_refuse(struct reference *ref)
{
	uint32_t type_count = voe_ecs_component_type_count(ref->world);

	ref->types = voe_base_arena_push(ref->arena,
					 (type_count + 1) * sizeof(*ref->types));
	for (uint32_t i = 0; i < type_count; i++) {
		voe_ecs_type type = voe_ecs_component_type_at(ref->world, i);

		if (voe_ecs_component_runtime_only(ref->world, type))
			continue;
		if (voe_ecs_component_description(ref->world, type) == NULL) {
			VOE_BASE_ERROR(MODULE,
				       "%s's description is compiled out of "
				       "this build, so no reference can say "
				       "its fields; nothing was written",
				       key_of(ref, type));
			return false;
		}
		if (voe_ecs_component_key(ref->world, type) ==
		    &voe_scene_identity_key) {
			ref->has_identity = true;
			ref->identity = type;
		}

		uint32_t j = ref->count++;

		while (j > 0 && strcmp(key_of(ref, ref->types[j - 1]),
				       key_of(ref, type)) > 0) {
			ref->types[j] = ref->types[j - 1];
			j--;
		}
		ref->types[j] = type;
	}
	return true;
}

// The written pass's bytes handed back as `*out`, NUL-terminated.
static void hand_back(voe_authoring_output *written, size_t measured,
		      voe_authoring_text *out)
{
	VOE_BASE_ASSERT(written->size == measured,
			"the writing pass disagreed with the measuring pass");
	written->bytes[written->size] = '\0';
	*out = (voe_authoring_text){
		.text = written->bytes,
		.size = written->size,
	};
}

bool voe_authoring_reference_scene(const voe_ecs_world *world,
				   voe_base_arena *arena,
				   voe_authoring_text *out)
{
	struct reference ref = { .world = world, .arena = arena };
	voe_authoring_output measured = { 0 };

	VOE_BASE_ASSERT(world != NULL, "a reference for a NULL world");
	VOE_BASE_ASSERT(arena != NULL && out != NULL,
			"a reference with nowhere to go");

	if (!gather_or_refuse(&ref) || !put_scene(&measured, &ref))
		return false;

	voe_authoring_output written = {
		.bytes = voe_base_arena_push(arena, measured.size + 1),
	};
	bool accepted = put_scene(&written, &ref);

	VOE_BASE_ASSERT(accepted, "the writing pass refused what was measured");
	hand_back(&written, measured.size, out);
	return true;
}

static void put_keys(voe_authoring_output *text, const char *title,
		     const voe_assets_documented_key *keys, uint32_t count)
{
	voe_authoring_put_string(text, "## ");
	voe_authoring_put_string(text, title);
	voe_authoring_put_string(text,
				 "\n\n| Section | Key | Kind | Default | Range | "
				 "Says |\n|---|---|---|---|---|---|\n");
	for (uint32_t i = 0; i < count; i++) {
		const voe_assets_documented_key *key = &keys[i];

		voe_authoring_put_string(text, "| `[");
		put_cell_string(text, key->section);
		voe_authoring_put_string(text, "]` | `");
		put_cell_string(text, key->key);
		voe_authoring_put_string(text, "` | ");
		put_cell_string(text, key->kind);
		voe_authoring_put_string(text, " | ");
		if (key->default_text[0] == '\0') {
			voe_authoring_put_string(text, "required");
		} else {
			voe_authoring_put_string(text, "`");
			put_cell_string(text, key->default_text);
			voe_authoring_put_string(text, "`");
		}
		voe_authoring_put_string(text, " | ");
		if (key->least == 0.0 && key->greatest == 0.0) {
			voe_authoring_put_string(text, "—");
		} else {
			put_range(text, key->least, key->greatest, key->unit);
			if (key->refused)
				voe_authoring_put_string(text, ", refused");
		}
		voe_authoring_put_string(text, " | ");
		put_cell_string(text, key->says);
		voe_authoring_put_string(text, " |\n");
	}
	voe_authoring_put_string(
		text,
		"\nA range marked refused is enforced: the strict read refuses "
		"a value outside it rather than clamping it. Any other range "
		"is not refused; the reader clamps the value or takes it as "
		"written.\n");
}

bool voe_authoring_reference_keys(const char *title,
				  const voe_assets_documented_key *keys,
				  uint32_t count, voe_base_arena *arena,
				  voe_authoring_text *out)
{
	voe_authoring_output measured = { 0 };

	VOE_BASE_ASSERT(title != NULL, "a key table with no title");
	VOE_BASE_ASSERT(keys != NULL || count == 0, "rows with nowhere they are");
	VOE_BASE_ASSERT(arena != NULL && out != NULL,
			"a reference with nowhere to go");

	put_keys(&measured, title, keys, count);

	voe_authoring_output written = {
		.bytes = voe_base_arena_push(arena, measured.size + 1),
	};

	put_keys(&written, title, keys, count);
	hand_back(&written, measured.size, out);
	return true;
}
