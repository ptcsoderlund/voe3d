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
// whole number, its shares and swaps one number each, and always written back;
// an old `strength` key is read and ignored whatever its value, and never
// written (0437). The old swap word is kept aside until every key is read, so
// it fills only the swaps whose own key was absent whatever the order, and the
// shares' order is checked last.
#include <assets/parts.h>
#include <assets/sectioned.h>

#include <base/assert.h>
#include <base/report.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// One escaped path: every byte doubled at worst, two quotes and a NUL.
#define QUOTED_PATH (2 * VOE_ASSETS_MATERIAL_PATH + 3)

// The old swap words (0432 point 1) and the multiplier each reads as.
static const char *const swap_words[] = { "near", "normal", "far",
					  "farther" };
static const float swap_word_values[] = { 0.5f, 1.0f, 2.0f, 4.0f };

// What a `[Levels]` section said beside its keys' values: the old swap
// word's multiplier, 0 for none, and which `swap<N>` keys were there.
struct levels_seen {
	float word;
	bool swap_given[VOE_ASSETS_LEVELS];
};

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

static bool share_in_range(float share)
{
	return share > 0.0f && share < 100.0f;
}

static bool swap_in_range(float swap)
{
	return swap >= VOE_ASSETS_LEVELS_SWAP_LEAST &&
	       swap <= VOE_ASSETS_LEVELS_SWAP_MOST;
}

// The first level 1 to 5 whose share is not below the one before it (level
// 1's below 100), or 0 when all are in order.
static uint32_t share_out_of_order(const voe_assets_levels *levels)
{
	VOE_BASE_ASSERT(levels != NULL, "ordering no shares");
	for (uint32_t i = 1; i < VOE_ASSETS_LEVELS; i++)
		if (!(levels->shares[i] <
		      (i == 1 ? 100.0f : levels->shares[i - 1])))
			return i;
	return 0;
}

// N of a key `<prefix><N>`, N one digit 1 to VOE_ASSETS_LEVELS - 1.
static bool level_index(const char *name, const char *prefix, uint32_t *level)
{
	const size_t length = strlen(prefix);

	VOE_BASE_ASSERT(name != NULL && level != NULL, "indexing no level key");
	if (strncmp(name, prefix, length) != 0 || name[length] < '1' ||
	    name[length] >= '0' + VOE_ASSETS_LEVELS || name[length + 1] != '\0')
		return false;
	*level = (uint32_t)(name[length] - '0');
	VOE_BASE_ASSERT(*level >= 1 && *level < VOE_ASSETS_LEVELS,
			"a level key past its room");
	return true;
}

// One `share<N>` or `swap<N>` value: a number, the whole value, in its range;
// read with strtof as `number()` in material.c reads.
static bool read_level_number(const voe_assets_sectioned_key *key, bool share,
			      float *value, voe_base_error *error)
{
	char *end;
	float parsed;

	VOE_BASE_ASSERT(key != NULL && value != NULL, "reading no level number");
	parsed = strtof(key->value, &end);
	if (end == key->value || *end != '\0' || !isfinite(parsed))
		return refuse(error, key->name, "not a number");
	if (share && !share_in_range(parsed))
		return refuse(error, key->name, "a share not between 0 and 100");
	if (!share && !swap_in_range(parsed))
		return refuse(error, key->name, "a swap not 0.1 to 10");
	*value = parsed;
	return true;
}

// One `[Levels]` key into `levels`: `count` a single digit 1 to
// VOE_ASSETS_LEVELS, `share<N>` and `swap<N>` a number each, the old `swap`
// word, one of four, into `seen`, and an old `strength` key, whatever its
// value, accepted and ignored (0437 point 3).
static bool read_level_key(const voe_assets_sectioned_key *key,
			   voe_assets_levels *levels, struct levels_seen *seen,
			   voe_base_error *error)
{
	uint32_t level;

	VOE_BASE_ASSERT(key != NULL && levels != NULL && seen != NULL,
			"reading no level key");
	if (level_index(key->name, "share", &level))
		return read_level_number(key, true, &levels->shares[level],
					 error);
	if (level_index(key->name, "swap", &level)) {
		seen->swap_given[level] = true;
		return read_level_number(key, false, &levels->swaps[level],
					 error);
	}
	if (strcmp(key->name, "count") == 0) {
		if (key->value[0] < '1' ||
		    key->value[0] > '0' + VOE_ASSETS_LEVELS ||
		    key->value[1] != '\0')
			return refuse(error, key->name, "not 1 to 6");
		levels->count = (uint32_t)(key->value[0] - '0');
		return true;
	}
	if (strcmp(key->name, "swap") == 0) {
		for (uint32_t i = 0; i < 4; i++) {
			if (strcmp(key->value, swap_words[i]) == 0) {
				seen->word = swap_word_values[i];
				return true;
			}
		}
		return refuse(error, key->name,
			      "not near, normal, far or farther");
	}
	if (strcmp(key->name, "strength") != 0)
		return refuse(error, key->name,
			      "not count, share1 to share5, swap1 to swap5 or "
			      "swap");
	return true;
}

// The optional `[Levels]` over the defaults already in `parts`, and no
// section but `[Parts]` and `[Levels]`; after every key, the old swap word
// into the swaps not given and the shares' order.
static bool read_levels(const voe_assets_sectioned *doc,
			voe_assets_parts *parts, voe_base_error *error)
{
	const uint32_t found = voe_assets_sectioned_find(doc, "Levels");
	const voe_assets_sectioned_section *section;
	struct levels_seen seen = { 0 };
	char name[16];
	uint32_t disorder;

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
				    &parts->levels, &seen, error))
			return false;
	for (uint32_t i = 1; i < VOE_ASSETS_LEVELS; i++)
		if (seen.word > 0.0f && !seen.swap_given[i])
			parts->levels.swaps[i] = seen.word;
	disorder = share_out_of_order(&parts->levels);
	if (disorder != 0) {
		snprintf(name, sizeof(name), "share%u", (unsigned)disorder);
		return refuse(error, name, "not below the share before it");
	}
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
			   "\n[Levels]\ncount=%u\n",
			   (unsigned)parts->levels.count);
	VOE_BASE_ASSERT(written > 0, "printing parts failed");
	at += (size_t)written;
	// share1 to share5, then swap1 to swap5.
	for (uint32_t i = 0; i < 2 * (VOE_ASSETS_LEVELS - 1); i++) {
		const bool share = i < VOE_ASSETS_LEVELS - 1;
		const uint32_t level = 1 + i % (VOE_ASSETS_LEVELS - 1);

		written = snprintf(buffer == NULL ? NULL : buffer + at,
				   buffer == NULL ? 0 : room - at, "%s%u=%g\n",
				   share ? "share" : "swap", (unsigned)level,
				   (double)(share ? parts->levels.shares[level] :
						    parts->levels.swaps[level]));
		VOE_BASE_ASSERT(written > 0, "printing parts failed");
		at += (size_t)written;
	}
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
	for (uint32_t i = 1; i < VOE_ASSETS_LEVELS; i++)
		VOE_BASE_ASSERT(share_in_range(parts->levels.shares[i]) &&
					swap_in_range(parts->levels.swaps[i]),
				"writing a share or swap out of range");
	VOE_BASE_ASSERT(share_out_of_order(&parts->levels) == 0,
			"writing shares out of order");

	size =print_parts(NULL, 0, parts);
	text = voe_base_arena_push(arena, size + 1);
	print_parts(text, size + 1, parts);
	return (voe_assets_material_text){ .text = text, .size = size };
}
