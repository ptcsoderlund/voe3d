// The scene reader. Two passes, and only the second one touches the world.
//
// PASS ONE READS EVERYTHING INTO SCRATCH. Every section's rows are pushed zeroed
// and filled from the text — a field the text does not mention from the type's
// unsaid row (voe_ecs_component_unsaid), else its default row, else zero — every
// entity reference is held as an authored id, and
// every kept section is copied out — so every refusal happens while the world is
// still exactly as it was handed over. Pass two creates the entities, maps the ids
// to them, patches the references and adds the rows, and can only fail on the
// world's capacity.
//
// THE TEXT IS WALKED A SECOND TIME, BESIDE THE SECTIONED READER, for key spans
// only: a kept section's lines as the file spelled them, which is the only way
// they can go back out byte for byte — the parsed value has lost its quotes.
// That walk is key_span.c. The line every refusal names is the parser's own,
// read off the parsed section or key.
//
// A ROW IS PUSHED AT THE SIZE ITS DESCRIPTION IMPLIES, NOT AT ITS REGISTERED SIZE.
// DEVIATION: card 071 scope 2, "scratch bytes of the type's size" is read as the
// end of the description's last field rounded up to 8, because ecs hands a type's
// size back only through _replace and not every type has one, and ecs may not be
// edited on this card. A struct written through VOE_BASE_DESCRIBE_STRUCT describes
// every member, its alignment is at most 8 for every kind, so that is at least its
// sizeof; where _replace does know the size an assert holds the two to it.
//
// ONE FIELD'S VALUE IS field_read.c's, which reads the text without recursing
// over it; this file decides which field of which row a key is.
//
// PASS ONE IS SHARED through scene_scratch.h: prefab_read.c runs the same pass
// and its own second one.
#include <authoring/scene_read.h>

#include "authored.h"
#include "scene_scratch.h"

#include <assets/sectioned.h>
#include <base/assert.h>
#include <base/describe.h>
#include <base/report.h>
#include <ecs/component.h>
#include <scene/identity_component.h>

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#define MODULE "authoring"

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
static bool classify(voe_authoring_scratch *reader)
{
	const voe_assets_sectioned *doc = &reader->doc;
	uint32_t authored = 0;

	for (uint32_t s = 0; s < doc->section_count; s++) {
		const char *name = doc->sections[s].name;
		const char *dot = strchr(name, '.');
		size_t digits = dot != NULL ? (size_t)(dot - name) : strlen(name);
		voe_authoring_section *section = &reader->sections[s];

		if (!authored_id(name, digits, &section->id) ||
		    (dot != NULL && dot[1] == '\0')) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is not a section of a scene, "
				       "which is [N] or [N.<key name>] with N a "
				       "decimal from 1 and no leading zero; nothing "
				       "was loaded",
				       reader->doc.sections[s].line, name);
			return false;
		}
		if (dot != NULL)
			continue;

		if (!reader->has_identity) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is an authored entity, and "
				       "this world registered no %s to give it "
				       "one with; nothing was loaded",
				       reader->doc.sections[s].line, name,
				       voe_scene_identity_key.name);
			return false;
		}
		section->role = VOE_AUTHORING_SECTION_ENTITY;
		section->type = reader->identity;
		section->description =
			voe_ecs_component_description(reader->world,
						      reader->identity);
		reader->refs.authored[authored++] = (voe_authoring_authored){
			.id = section->id,
		};
	}

	voe_authoring_authored *scratch = voe_base_arena_push(
		reader->arena, (authored + 1) * sizeof(*scratch));

	reader->refs.authored_count = authored;
	voe_authoring_authored_sort(reader->refs.authored, scratch, authored);

	for (uint32_t s = 0; s < doc->section_count; s++) {
		voe_authoring_section *section = &reader->sections[s];
		const char *name = doc->sections[s].name;
		const char *dot = strchr(name, '.');

		if (dot == NULL)
			continue;
		if (voe_authoring_authored_find(reader->refs.authored,
						reader->refs.authored_count,
						section->id) == NULL) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] belongs to entity %" PRIu64
				       ", and the file has no [%" PRIu64 "]; "
				       "nothing was loaded",
				       reader->doc.sections[s].line, name, section->id,
				       section->id);
			return false;
		}

		const char *key = dot + 1;

		if (!type_by_name(reader->world, key, &section->type)) {
			section->role = VOE_AUTHORING_SECTION_KEPT;
			continue;
		}
		if (reader->has_identity &&
		    section->type.value == reader->identity.value) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is an identity, and an "
				       "identity is written as [%" PRIu64 "] "
				       "itself; nothing was loaded",
				       reader->doc.sections[s].line, name, section->id);
			return false;
		}
		if (voe_ecs_component_runtime_only(reader->world,
						   section->type)) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: [%s] is %s, which is runtime-only "
				       "and never authored, so a file cannot hold "
				       "one; nothing was loaded",
				       reader->doc.sections[s].line, name, key);
			return false;
		}
		section->role = VOE_AUTHORING_SECTION_COMPONENT;
		section->description =
			voe_ecs_component_description(reader->world,
						      section->type);
	}
	return true;
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
static bool read_section(voe_authoring_scratch *reader, uint32_t s)
{
	const voe_assets_sectioned_section *parsed = &reader->doc.sections[s];
	voe_authoring_section *section = &reader->sections[s];
	const voe_base_struct_description *description = section->description;
	bool entity = section->role == VOE_AUTHORING_SECTION_ENTITY;
	voe_authoring_site site = {
		.line = reader->doc.sections[s].line,
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

		site.line = reader->doc.keys[index].line;
		if (field == NULL) {
			VOE_BASE_WARNING(MODULE,
					 "line %u: [%s] has no field %s; the "
					 "line is ignored",
					 site.line, parsed->name, key->name);
			continue;
		}
		site.field = field->name;
		if (!voe_authoring_field_value(&reader->refs, &site, field,
					       key->value, section->row))
			return false;
	}

	// The unsaid row first (0324): what an old file meant may differ from
	// what a new row is.
	const uint8_t *fallback =
		voe_ecs_component_unsaid(reader->world, section->type);
	const char *fallback_name = "its unsaid value";

	if (fallback == NULL) {
		fallback = voe_ecs_component_default(reader->world,
						     section->type);
		fallback_name = fallback != NULL ? "its default" : "zero";
	}

	for (uint32_t f = 0; f < description->field_count; f++) {
		const voe_base_field_description *field = &description->fields[f];

		if (entity && field->offset == offsetof(voe_scene_identity, id))
			continue;
		if (voe_assets_sectioned_value(&reader->doc, s, field->name) !=
		    NULL)
			continue;
		if (fallback != NULL)
			memcpy((uint8_t *)section->row + field->offset,
			       fallback + field->offset, field->size);
		VOE_BASE_WARNING(MODULE,
				 "line %u: [%s] does not say %s; loaded as %s",
				 reader->doc.sections[s].line, parsed->name,
				 field->name, fallback_name);
	}

	if (entity) {
		voe_scene_identity identity;

		memcpy(&identity, section->row, sizeof(identity));
		identity.id = section->id;
		memcpy(section->row, &identity, sizeof(identity));
	}
	return true;
}

static void keep_section(voe_authoring_scratch *reader, uint32_t s)
{
	const voe_assets_sectioned_section *parsed = &reader->doc.sections[s];
	size_t size = 0;
	char *lines;

	for (uint32_t k = 0; k < parsed->key_count; k++)
		size += reader->key_span[parsed->first_key + k].size + 1;

	lines = voe_base_arena_push(reader->arena, size + 1);
	size = 0;
	for (uint32_t k = 0; k < parsed->key_count; k++) {
		const voe_authoring_span *span =
			&reader->key_span[parsed->first_key + k];

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
static uint32_t patch_capacity(const voe_authoring_scratch *reader)
{
	uint32_t count = 0;

	for (uint32_t s = 0; s < reader->doc.section_count; s++) {
		const voe_authoring_section *section = &reader->sections[s];

		if (section->role == VOE_AUTHORING_SECTION_KEPT || section->description == NULL)
			continue;
		for (uint32_t f = 0; f < section->description->field_count; f++)
			if (section->description->fields[f].kind ==
			    VOE_BASE_FIELD_ENTITY)
				count += section->description->fields[f].count;
	}
	return count;
}

static void find_identity(voe_authoring_scratch *reader)
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
static bool create(voe_authoring_scratch *reader)
{
	const voe_assets_sectioned *doc = &reader->doc;

	for (uint32_t s = 0; s < doc->section_count; s++) {
		voe_authoring_section *section = &reader->sections[s];

		if (section->role != VOE_AUTHORING_SECTION_ENTITY)
			continue;

		voe_authoring_authored *authored = voe_authoring_authored_find(
			reader->refs.authored, reader->refs.authored_count, section->id);

		VOE_BASE_ASSERT(authored != NULL, "an [N] missing from its map");
		if (!voe_ecs_entity_create(reader->world, &authored->entity)) {
			VOE_BASE_ERROR(MODULE,
				       "line %u: the world is full at [%" PRIu64
				       "]; it holds part of the scene and must "
				       "be discarded",
				       reader->doc.sections[s].line, section->id);
			return false;
		}
	}

	for (uint32_t p = 0; p < reader->refs.patch_count; p++) {
		const voe_authoring_patch *patch = &reader->refs.patches[p];
		voe_authoring_authored *authored = voe_authoring_authored_find(
			reader->refs.authored, reader->refs.authored_count, patch->id);

		VOE_BASE_ASSERT(authored != NULL, "a held id missing from the map");
		memcpy(patch->bytes, &authored->entity, sizeof(authored->entity));
	}

	// The identities first, then every other row, each in file order.
	for (uint32_t pass = 0; pass < 2; pass++) {
		for (uint32_t s = 0; s < doc->section_count; s++) {
			const voe_authoring_section *section = &reader->sections[s];

			if (section->role == VOE_AUTHORING_SECTION_KEPT ||
			    (section->role == VOE_AUTHORING_SECTION_ENTITY) != (pass == 0))
				continue;

			voe_authoring_authored *authored =
				voe_authoring_authored_find(
					reader->refs.authored,
					reader->refs.authored_count, section->id);

			VOE_BASE_ASSERT(authored != NULL,
					"a section's [N] missing from the map");
			if (voe_ecs_component_add(reader->world, section->type,
						  authored->entity, section->row))
				continue;
			VOE_BASE_ERROR(MODULE,
				       "line %u: the world has no room for [%s]; "
				       "it holds part of the scene and must be "
				       "discarded",
				       reader->doc.sections[s].line,
				       doc->sections[s].name);
			return false;
		}
	}
	return true;
}

bool voe_authoring_scratch_read(voe_authoring_scratch *reader,
				const char *text, size_t size)
{
	VOE_BASE_ASSERT(text != NULL || size == 0, "reading NULL text");
	VOE_BASE_ASSERT(reader != NULL && reader->world != NULL &&
				reader->arena != NULL,
			"reading with no world or arena");

	voe_base_arena *arena = reader->arena;

	find_identity(reader);
	if (!voe_assets_sectioned_parse(text, size, arena, &reader->doc))
		return false;

	uint32_t sections = reader->doc.section_count;
	uint32_t keys = reader->doc.key_count;

	// One more than needed of each, so that an empty file pushes something.
	reader->key_span =
		voe_base_arena_push(arena, (keys + 1) * sizeof(*reader->key_span));
	reader->sections = voe_base_arena_push(
		arena, (sections + 1) * sizeof(*reader->sections));
	memset(reader->sections, 0, (sections + 1) * sizeof(*reader->sections));
	reader->refs.authored = voe_base_arena_push(
		arena, (sections + 1) * sizeof(*reader->refs.authored));
	reader->kept =
		voe_base_arena_push(arena, (sections + 1) * sizeof(*reader->kept));

	voe_authoring_key_spans(text, size, &reader->doc, reader->key_span);
	if (!classify(reader))
		return false;

	reader->refs.patches = voe_base_arena_push(
		arena, (patch_capacity(reader) + 1) * sizeof(*reader->refs.patches));

	for (uint32_t s = 0; s < sections; s++) {
		if (reader->sections[s].role == VOE_AUTHORING_SECTION_KEPT)
			keep_section(reader, s);
		else if (!read_section(reader, s))
			return false;
	}
	VOE_BASE_ASSERT(reader->kept_count <= sections, "more kept than sections");
	return true;
}

bool voe_authoring_scene_read(const char *text, size_t size,
			      voe_ecs_world *world, voe_base_arena *arena,
			      voe_authoring_kept *out_kept)
{
	voe_authoring_scratch reader = { .world = world, .arena = arena };

	VOE_BASE_ASSERT(world != NULL, "reading into a NULL world");
	VOE_BASE_ASSERT(arena != NULL, "reading with a NULL arena");
	VOE_BASE_ASSERT(out_kept != NULL, "nowhere to put the kept sections");

	find_identity(&reader);
	VOE_BASE_ASSERT(!reader.has_identity ||
				voe_ecs_component_count(world, reader.identity) == 0,
			"loading a scene into a world that already holds an "
			"authored entity");

	if (!voe_authoring_scratch_read(&reader, text, size) ||
	    !create(&reader))
		return false;

	*out_kept = (voe_authoring_kept){
		.sections = reader.kept_count > 0 ? reader.kept : NULL,
		.count = reader.kept_count,
	};
	return true;
}
