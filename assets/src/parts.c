// The parts file behind assets/parts.h: the sectioned reader for the lines,
// each key turned into an index here, and one measured print for the way back.
//
// THE READER MAKES ITS OWN SCRATCH ARENA, as material.c does and for the same
// reason: the result is a plain struct, a file is a few lines read on open,
// and an arena parameter would lift the one malloc per read if a caller ever
// read in a loop.
//
// PATHS ARE ALWAYS WRITTEN QUOTED, with `"` and `\` escaped as the sectioned
// reader takes them back (0149), so any path returns as it went.
//
// `[LEVELS]` IS READ THE SAME WAY, after the defaults are filled, its count a
// whole number and its strength and swap quoted words, and always written
// back.
#include <assets/parts.h>
#include <assets/sectioned.h>

#include <base/assert.h>
#include <base/report.h>

#include <stdio.h>
#include <string.h>

// One escaped path: every byte doubled at worst, two quotes and a NUL.
#define QUOTED_PATH (2 * VOE_ASSETS_MATERIAL_PATH + 3)

// Each strength's word, in the enum's order.
static const char *const strength_words[] = { "gentle", "medium", "strong" };

// Each swap's word, in the enum's order.
static const char *const swap_words[] = { "near", "normal", "far",
					  "farther" };

static bool refuse(voe_base_error *error, const char *key, const char *message)
{
	VOE_BASE_ERROR("assets", "parts: %s: %s", key, message);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

// N of a key `material<N>`, N in decimal with no leading zero and below
// VOE_ASSETS_PARTS, so one index has one spelling and no two keys collide.
static bool key_index(const char *name, uint32_t *index)
{
	const char *digits = name + strlen("material");
	uint32_t value = 0;

	if (strncmp(name, "material", strlen("material")) != 0 ||
	    digits[0] == '\0' || (digits[0] == '0' && digits[1] != '\0'))
		return false;
	for (size_t i = 0; digits[i] != '\0'; i++) {
		if (i >= 2 || digits[i] < '0' || digits[i] > '9')
			return false;
		value = value * 10 + (uint32_t)(digits[i] - '0');
	}
	if (value >= VOE_ASSETS_PARTS)
		return false;
	*index = value;
	return true;
}

static bool read_section(const voe_assets_sectioned *doc,
			 voe_assets_parts *parts, voe_base_error *error)
{
	const uint32_t found = voe_assets_sectioned_find(doc, "Parts");
	const voe_assets_sectioned_section *section;

	if (found == VOE_ASSETS_SECTIONED_NONE)
		return refuse(error, "[Parts]", "no such section");
	section = &doc->sections[found];
	for (uint32_t i = 0; i < section->key_count; i++) {
		const voe_assets_sectioned_key *key =
			&doc->keys[section->first_key + i];
		const size_t length = strlen(key->value);
		uint32_t index;

		if (!key_index(key->name, &index))
			return refuse(error, key->name,
				      "not material0 to material31");
		if (length >= VOE_ASSETS_MATERIAL_PATH)
			return refuse(error, key->name,
				      "a path of 128 bytes or more");
		memcpy(parts->materials[index], key->value, length + 1);
		if (index >= parts->count)
			parts->count = index + 1;
	}
	VOE_BASE_ASSERT(parts->count <= VOE_ASSETS_PARTS,
			"a parts count past its room");
	return true;
}

// One `[Levels]` key into `levels`: `count` a single digit 1 to
// VOE_ASSETS_LEVELS, `strength` one of the three words, `swap` one of the
// four.
static bool read_level_key(const voe_assets_sectioned_key *key,
			   voe_assets_levels *levels, voe_base_error *error)
{
	VOE_BASE_ASSERT(key != NULL && levels != NULL, "reading no level key");
	if (strcmp(key->name, "count") == 0) {
		if (key->value[0] < '1' ||
		    key->value[0] > '0' + VOE_ASSETS_LEVELS ||
		    key->value[1] != '\0')
			return refuse(error, key->name, "not 1 to 4");
		levels->count = (uint32_t)(key->value[0] - '0');
		return true;
	}
	if (strcmp(key->name, "swap") == 0) {
		for (uint32_t i = 0; i < 4; i++) {
			if (strcmp(key->value, swap_words[i]) == 0) {
				levels->swap = (voe_assets_levels_swap)i;
				return true;
			}
		}
		return refuse(error, key->name,
			      "not near, normal, far or farther");
	}
	if (strcmp(key->name, "strength") != 0)
		return refuse(error, key->name, "not count, strength or swap");
	for (uint32_t i = 0; i < 3; i++) {
		if (strcmp(key->value, strength_words[i]) == 0) {
			levels->strength = (voe_assets_levels_strength)i;
			return true;
		}
	}
	return refuse(error, key->name, "not gentle, medium or strong");
}

// The optional `[Levels]` over the defaults already in `parts`, and no
// section but `[Parts]` and `[Levels]`.
static bool read_levels(const voe_assets_sectioned *doc,
			voe_assets_parts *parts, voe_base_error *error)
{
	const uint32_t found = voe_assets_sectioned_find(doc, "Levels");
	const voe_assets_sectioned_section *section;

	for (uint32_t i = 0; i < doc->section_count; i++)
		if (strcmp(doc->sections[i].name, "Parts") != 0 &&
		    strcmp(doc->sections[i].name, "Levels") != 0)
			return refuse(error, doc->sections[i].name,
				      "not [Parts] or [Levels]");
	if (found == VOE_ASSETS_SECTIONED_NONE)
		return true;
	section = &doc->sections[found];
	for (uint32_t i = 0; i < section->key_count; i++)
		if (!read_level_key(&doc->keys[section->first_key + i],
				    &parts->levels, error))
			return false;
	VOE_BASE_ASSERT(parts->levels.count >= 1 &&
				parts->levels.count <= VOE_ASSETS_LEVELS,
			"a levels count out of range");
	return true;
}

bool voe_assets_parts_read(const char *text, size_t size,
			   voe_assets_parts *out, voe_base_error *error)
{
	voe_assets_parts parts = { .levels = VOE_ASSETS_LEVELS_DEFAULT };
	voe_base_arena *scratch;
	voe_assets_sectioned doc;
	bool read;

	VOE_BASE_ASSERT(text != NULL, "reading parts from nothing");
	VOE_BASE_ASSERT(out != NULL, "reading parts into nothing");

	scratch = voe_base_arena_new(4096);
	read = voe_assets_sectioned_parse(text, size, scratch, &doc);
	if (!read)
		refuse(error, "text", "not sectioned text");
	else
		read = read_section(&doc, &parts, error) &&
		       read_levels(&doc, &parts, error);
	voe_base_arena_destroy(scratch);
	if (read)
		*out = parts;
	return read;
}

// `path` between quotes with `"` and `\` escaped, into `quoted`.
static void quote(const char *path, char *quoted)
{
	size_t at = 0;

	VOE_BASE_ASSERT(memchr(path, '\0', VOE_ASSETS_MATERIAL_PATH) != NULL,
			"a parts path not terminated in its room");
	quoted[at++] = '"';
	for (size_t i = 0; path[i] != '\0'; i++) {
		VOE_BASE_ASSERT(path[i] != '\n' && path[i] != '\r',
				"a parts path with a line break");
		if (path[i] == '"' || path[i] == '\\')
			quoted[at++] = '\\';
		quoted[at++] = path[i];
	}
	quoted[at++] = '"';
	quoted[at] = '\0';
	VOE_BASE_ASSERT(at < QUOTED_PATH, "a quoted path outgrew its room");
}

// The whole text into `buffer`, or only its length when `buffer` is NULL.
static size_t print_parts(char *buffer, size_t room,
			  const voe_assets_parts *parts)
{
	char quoted[QUOTED_PATH];
	size_t at;
	int written = snprintf(buffer, room, "[Parts]\n");

	VOE_BASE_ASSERT(written > 0, "printing parts failed");
	at = (size_t)written;
	for (uint32_t i = 0; i < parts->count; i++) {
		quote(parts->materials[i], quoted);
		written = snprintf(buffer == NULL ? NULL : buffer + at,
				   buffer == NULL ? 0 : room - at,
				   "material%u=%s\n", (unsigned)i, quoted);
		VOE_BASE_ASSERT(written > 0, "printing parts failed");
		at += (size_t)written;
	}
	written = snprintf(buffer == NULL ? NULL : buffer + at,
			   buffer == NULL ? 0 : room - at,
			   "\n[Levels]\ncount=%u\nstrength=\"%s\"\n"
			   "swap=\"%s\"\n",
			   (unsigned)parts->levels.count,
			   strength_words[parts->levels.strength],
			   swap_words[parts->levels.swap]);
	VOE_BASE_ASSERT(written > 0, "printing parts failed");
	at += (size_t)written;
	VOE_BASE_ASSERT(buffer == NULL || at < room,
			"the parts text was measured by this same print");
	return at;
}

voe_assets_material_text voe_assets_parts_write(const voe_assets_parts *parts,
						voe_base_arena *arena)
{
	size_t size;
	char *text;

	VOE_BASE_ASSERT(parts != NULL, "writing no parts");
	VOE_BASE_ASSERT(arena != NULL, "writing parts with no arena");
	VOE_BASE_ASSERT(parts->count <= VOE_ASSETS_PARTS,
			"writing more parts than there is room for");
	VOE_BASE_ASSERT(parts->levels.count >= 1 &&
				parts->levels.count <= VOE_ASSETS_LEVELS,
			"writing a levels count out of range");
	VOE_BASE_ASSERT(parts->levels.strength <= VOE_ASSETS_LEVELS_STRONG,
			"writing a strength that is not one of three");
	VOE_BASE_ASSERT(parts->levels.swap <= VOE_ASSETS_LEVELS_SWAP_FARTHER,
			"writing a swap that is not one of four");

	size =print_parts(NULL, 0, parts);
	text = voe_base_arena_push(arena, size + 1);
	print_parts(text, size + 1, parts);
	return (voe_assets_material_text){ .text = text, .size = size };
}
