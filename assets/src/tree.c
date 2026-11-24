// The tree species file behind assets/tree.h: the sectioned reader for the
// lines, each value turned into a number or a path here, and one measured
// print for the way back.
//
// THE READER MAKES ITS OWN SCRATCH ARENA, as material.c does and for the same
// reason: the result is a plain struct, and a species file is a few hundred
// bytes read on open and after an edit. An arena parameter would lift the one
// malloc a read if a caller ever read in a loop.
//
// ONE TABLE OF RANGED KEYS READS EVERY FLOAT, for both readers: the lenient one
// clamps a value outside the row's ends, the strict one refuses it. The strict
// reader then walks every section and key against the key table, so a key the
// table lacks is refused by the strict reader alone.
//
// PATHS ARE ALWAYS WRITTEN QUOTED with the landscape's quote helper, so a path
// with blanks at its ends, a quote or a backslash returns as it went.
#include <assets/sectioned.h>
#include <assets/tree.h>

#include "landscape_foliage.h"

#include <base/assert.h>
#include <base/report.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool refuse(voe_base_error *error, const char *key, const char *message)
{
	VOE_BASE_ERROR("assets", "tree: %s: %s", key, message);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

static bool refuse_line(voe_base_error *error, uint32_t line, const char *name,
			const char *message)
{
	VOE_BASE_ERROR("assets", "tree: line %u: %s: %s", line, name, message);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

static bool refuse_range(voe_base_error *error, const char *key, double least,
			 double greatest)
{
	VOE_BASE_ERROR("assets", "tree: %s: outside %g to %g", key, least,
		       greatest);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

// One float key: where it sits in the struct and its ends.
struct ranged_key {
	const char *key;
	size_t at;
	float least;
	float greatest;
};

#define RANGED(name, upper)                                                  \
	{ #name, offsetof(voe_assets_tree, name), VOE_ASSETS_TREE_##upper##_MIN, \
	  VOE_ASSETS_TREE_##upper##_MAX }

static const struct ranged_key ranged_keys[] = {
	RANGED(height, HEIGHT),		  RANGED(trunk, TRUNK),
	RANGED(crown, CROWN),		  RANGED(branching, BRANCHING),
	RANGED(reach, REACH),		  RANGED(angle, ANGLE),
	RANGED(droop, DROOP),		  RANGED(crooked, CROOKED),
	RANGED(leaf_size, LEAF_SIZE),	  RANGED(leaf_density, LEAF_DENSITY),
	RANGED(leaf_roll, LEAF_ROLL),	  RANGED(leaf_width, LEAF_WIDTH),
};

static bool read_path(const voe_assets_sectioned *doc, uint32_t section,
		      const char *key, char *path, voe_base_error *error)
{
	const char *value = voe_assets_sectioned_value(doc, section, key);
	size_t length;

	if (value == NULL)
		return true;
	length = strlen(value);
	if (length >= VOE_ASSETS_TREE_PATH)
		return refuse(error, key, "a path of 128 bytes or more");
	memcpy(path, value, length + 1);
	return true;
}

// The key's float into its field of `tree`, clamped to the row's ends, or
// refused outside them when `strict`; a missing key leaves the field.
static bool read_ranged(const voe_assets_sectioned *doc, uint32_t section,
			const struct ranged_key *row, voe_assets_tree *tree,
			bool strict, voe_base_error *error)
{
	const char *value = voe_assets_sectioned_value(doc, section, row->key);
	char *end;
	float parsed;

	if (value == NULL)
		return true;
	parsed = strtof(value, &end);
	if (end == value || *end != '\0' || !isfinite(parsed))
		return refuse(error, row->key, "not a number");
	if (parsed < row->least || parsed > row->greatest) {
		if (strict)
			return refuse_range(error, row->key,
					    (double)row->least,
					    (double)row->greatest);
		parsed = fminf(fmaxf(parsed, row->least), row->greatest);
	}
	memcpy((char *)tree + row->at, &parsed, sizeof parsed);
	return true;
}

// `variations` as a whole number, clamped or refused as read_ranged does.
static bool read_variations(const voe_assets_sectioned *doc, uint32_t section,
			    uint32_t *variations, bool strict,
			    voe_base_error *error)
{
	const char *value =
		voe_assets_sectioned_value(doc, section, "variations");
	char *end;
	long long parsed;

	if (value == NULL)
		return true;
	parsed = strtoll(value, &end, 10);
	if (end == value || *end != '\0')
		return refuse(error, "variations", "not a whole number");
	if (parsed < VOE_ASSETS_TREE_VARIATIONS_MIN ||
	    parsed > VOE_ASSETS_TREE_VARIATIONS_MAX) {
		if (strict)
			return refuse_range(error, "variations",
					    VOE_ASSETS_TREE_VARIATIONS_MIN,
					    VOE_ASSETS_TREE_VARIATIONS_MAX);
		parsed = parsed < VOE_ASSETS_TREE_VARIATIONS_MIN ?
				 VOE_ASSETS_TREE_VARIATIONS_MIN :
				 VOE_ASSETS_TREE_VARIATIONS_MAX;
	}
	*variations = (uint32_t)parsed;
	return true;
}

static bool read_section(const voe_assets_sectioned *doc, voe_assets_tree *tree,
			 bool strict, voe_base_error *error)
{
	const uint32_t section = voe_assets_sectioned_find(doc, "Tree");

	if (section == VOE_ASSETS_SECTIONED_NONE)
		return refuse(error, "[Tree]", "no such section");
	if (!read_path(doc, section, "bark", tree->bark, error) ||
	    !read_path(doc, section, "leaf", tree->leaf, error))
		return false;
	for (size_t i = 0; i < sizeof(ranged_keys) / sizeof(ranged_keys[0]); i++)
		if (!read_ranged(doc, section, &ranged_keys[i], tree, strict,
				 error))
			return false;
	return read_variations(doc, section, &tree->variations, strict, error);
}

static bool key_taken(const char *name)
{
	uint32_t count;
	const voe_assets_documented_key *keys = voe_assets_tree_keys(&count);

	for (uint32_t i = 0; i < count; i++)
		if (strcmp(name, keys[i].key) == 0)
			return true;
	return false;
}

// Every section `[Tree]` and each of its keys a row of the key table.
static bool check_strict(const voe_assets_sectioned *doc, voe_base_error *error)
{
	for (uint32_t s = 0; s < doc->section_count; s++) {
		const voe_assets_sectioned_section *section = &doc->sections[s];

		if (strcmp(section->name, "Tree") != 0)
			return refuse_line(error, section->line, section->name,
					   "a section other than [Tree]");
		for (uint32_t i = 0; i < section->key_count; i++) {
			const voe_assets_sectioned_key *key =
				&doc->keys[section->first_key + i];

			if (!key_taken(key->name))
				return refuse_line(error, key->line, key->name,
						   "a key the reader does not take");
		}
	}
	return true;
}

voe_assets_tree voe_assets_tree_default(void)
{
	return (voe_assets_tree){
		.height = VOE_ASSETS_TREE_HEIGHT_DEFAULT,
		.trunk = VOE_ASSETS_TREE_TRUNK_DEFAULT,
		.crown = VOE_ASSETS_TREE_CROWN_DEFAULT,
		.branching = VOE_ASSETS_TREE_BRANCHING_DEFAULT,
		.reach = VOE_ASSETS_TREE_REACH_DEFAULT,
		.angle = VOE_ASSETS_TREE_ANGLE_DEFAULT,
		.droop = VOE_ASSETS_TREE_DROOP_DEFAULT,
		.crooked = VOE_ASSETS_TREE_CROOKED_DEFAULT,
		.leaf_size = VOE_ASSETS_TREE_LEAF_SIZE_DEFAULT,
		.leaf_density = VOE_ASSETS_TREE_LEAF_DENSITY_DEFAULT,
		.leaf_roll = VOE_ASSETS_TREE_LEAF_ROLL_DEFAULT,
		.leaf_width = VOE_ASSETS_TREE_LEAF_WIDTH_DEFAULT,
		.variations = VOE_ASSETS_TREE_VARIATIONS_DEFAULT,
	};
}

static bool read_tree(const char *text, size_t size, voe_assets_tree *out,
		      voe_base_error *error, bool strict)
{
	voe_assets_tree tree = voe_assets_tree_default();
	voe_base_arena *scratch;
	voe_assets_sectioned doc;
	bool read;

	VOE_BASE_ASSERT(text != NULL, "reading a tree from nothing");
	VOE_BASE_ASSERT(out != NULL, "reading a tree into nothing");

	scratch = voe_base_arena_new(4096);
	read = voe_assets_sectioned_parse(text, size, scratch, &doc);
	if (!read)
		refuse(error, "text", "not sectioned text");
	else
		read = read_section(&doc, &tree, strict, error) &&
		       (!strict || check_strict(&doc, error));
	voe_base_arena_destroy(scratch);
	if (read)
		*out = tree;
	return read;
}

bool voe_assets_tree_read(const char *text, size_t size, voe_assets_tree *out,
			  voe_base_error *error)
{
	return read_tree(text, size, out, error, false);
}

bool voe_assets_tree_read_strict(const char *text, size_t size,
				 voe_assets_tree *out, voe_base_error *error)
{
	return read_tree(text, size, out, error, true);
}

// The whole text into `buffer`, or only its length when `buffer` is NULL.
static size_t print_tree(char *buffer, size_t room, const voe_assets_tree *tree,
			 const char *bark, const char *leaf)
{
	const int written = snprintf(
		buffer, room,
		"[Tree]\nbark=%s\nleaf=%s\nheight=%.9g\ntrunk=%.9g\n"
		"crown=%.9g\nbranching=%.9g\nreach=%.9g\nangle=%.9g\n"
		"droop=%.9g\ncrooked=%.9g\nleaf_size=%.9g\nleaf_density=%.9g\n"
		"leaf_roll=%.9g\nleaf_width=%.9g\nvariations=%u\n",
		bark, leaf, (double)tree->height, (double)tree->trunk,
		(double)tree->crown, (double)tree->branching,
		(double)tree->reach, (double)tree->angle, (double)tree->droop,
		(double)tree->crooked, (double)tree->leaf_size,
		(double)tree->leaf_density, (double)tree->leaf_roll,
		(double)tree->leaf_width, tree->variations);

	VOE_BASE_ASSERT(written > 0, "printing a tree failed");
	VOE_BASE_ASSERT(buffer == NULL || (size_t)written < room,
			"the tree text was measured by this same print");
	return (size_t)written;
}

voe_assets_tree_text voe_assets_tree_write(const voe_assets_tree *tree,
					   voe_base_arena *arena)
{
	char bark[VOE_ASSETS_LANDSCAPE_QUOTED_PATH];
	char leaf[VOE_ASSETS_LANDSCAPE_QUOTED_PATH];
	size_t size;
	char *text;

	VOE_BASE_ASSERT(tree != NULL, "writing no tree");
	VOE_BASE_ASSERT(arena != NULL, "writing a tree with no arena");

	voe_assets_landscape_quote(tree->bark, bark);
	voe_assets_landscape_quote(tree->leaf, leaf);
	size = print_tree(NULL, 0, tree, bark, leaf);
	text = voe_base_arena_push(arena, size + 1);
	print_tree(text, size + 1, tree, bark, leaf);
	return (voe_assets_tree_text){ .text = text, .size = size };
}

// Whether the `length` bytes of `path` end `.tree` in any case.
static bool ends_tree(const char *path, size_t length)
{
	static const char suffix[] = ".tree";
	const size_t suffix_length = sizeof(suffix) - 1;

	if (length < suffix_length)
		return false;
	for (size_t i = 0; i < suffix_length; i++)
		if (tolower((unsigned char)path[length - suffix_length + i]) !=
		    suffix[i])
			return false;
	return true;
}

bool voe_assets_tree_path_is(const char *path)
{
	const char *hash;

	VOE_BASE_ASSERT(path != NULL, "asking whether nothing is a tree");
	if (ends_tree(path, strlen(path)))
		return true;
	hash = strrchr(path, '#');
	if (hash == NULL || hash[1] == '\0')
		return false;
	for (const char *at = hash + 1; *at != '\0'; at++)
		if (!isdigit((unsigned char)*at))
			return false;
	return ends_tree(path, (size_t)(hash - path));
}

bool voe_assets_tree_variation_path(char *out, size_t room, const char *path,
				    uint32_t v)
{
	int needed;

	VOE_BASE_ASSERT(out != NULL && path != NULL,
			"a variation path from or into nothing");
	needed = snprintf(NULL, 0, "%s#%u", path, v);
	VOE_BASE_ASSERT(needed > 0, "measuring a variation path failed");
	if ((size_t)needed >= room)
		return false;
	needed = snprintf(out, room, "%s#%u", path, v);
	VOE_BASE_ASSERT((size_t)needed < room,
			"the variation path was measured by this same print");
	return true;
}

// One row per key in tree.h's struct order, ranges from its constants.
// Every ranged row is refused out of range by the strict reader.
static const voe_assets_documented_key documented_keys[] = {
	{ "Tree", "bark", "quoted path", "\"\"", 0, 0, "", false,
	  "The bark .material, project-relative, \"\" for none." },
	{ "Tree", "leaf", "quoted path", "\"\"", 0, 0, "", false,
	  "The leaf .material, project-relative, \"\" for none." },
	{ "Tree", "height", "number", "12", VOE_ASSETS_TREE_HEIGHT_MIN,
	  VOE_ASSETS_TREE_HEIGHT_MAX, "m", true, "The tree's height." },
	{ "Tree", "trunk", "number", "0.4", VOE_ASSETS_TREE_TRUNK_MIN,
	  VOE_ASSETS_TREE_TRUNK_MAX, "m", true,
	  "The trunk's diameter at its base." },
	{ "Tree", "crown", "number", "0.3", VOE_ASSETS_TREE_CROWN_MIN,
	  VOE_ASSETS_TREE_CROWN_MAX, "", true,
	  "The share of the height where branches start." },
	{ "Tree", "branching", "number", "4", VOE_ASSETS_TREE_BRANCHING_MIN,
	  VOE_ASSETS_TREE_BRANCHING_MAX, "/m", true,
	  "Branches per metre of their parent." },
	{ "Tree", "reach", "number", "0.35", VOE_ASSETS_TREE_REACH_MIN,
	  VOE_ASSETS_TREE_REACH_MAX, "", true,
	  "A child's length as a share of its parent's." },
	{ "Tree", "angle", "number", "60", VOE_ASSETS_TREE_ANGLE_MIN,
	  VOE_ASSETS_TREE_ANGLE_MAX, "°", true,
	  "Degrees a branch leaves its parent at." },
	{ "Tree", "droop", "number", "0.2", VOE_ASSETS_TREE_DROOP_MIN,
	  VOE_ASSETS_TREE_DROOP_MAX, "", true,
	  "How far branches bend down; a negative value bends them up." },
	{ "Tree", "crooked", "number", "0.2", VOE_ASSETS_TREE_CROOKED_MIN,
	  VOE_ASSETS_TREE_CROOKED_MAX, "", true,
	  "How far branches wander from straight." },
	{ "Tree", "leaf_size", "number", "0.3", VOE_ASSETS_TREE_LEAF_SIZE_MIN,
	  VOE_ASSETS_TREE_LEAF_SIZE_MAX, "m", true, "A leaf card's size." },
	{ "Tree", "leaf_density", "number", "12",
	  VOE_ASSETS_TREE_LEAF_DENSITY_MIN, VOE_ASSETS_TREE_LEAF_DENSITY_MAX, "",
	  true, "Leaf cards per twig." },
	{ "Tree", "leaf_roll", "number", "0", VOE_ASSETS_TREE_LEAF_ROLL_MIN,
	  VOE_ASSETS_TREE_LEAF_ROLL_MAX, "", true,
	  "How far a card is turned toward its twig's level plane; 1 lies flat." },
	{ "Tree", "leaf_width", "number", "1", VOE_ASSETS_TREE_LEAF_WIDTH_MIN,
	  VOE_ASSETS_TREE_LEAF_WIDTH_MAX, "", true,
	  "A card's width out from its twig, as a share of leaf_size." },
	{ "Tree", "variations", "whole number", "4",
	  VOE_ASSETS_TREE_VARIATIONS_MIN, VOE_ASSETS_TREE_VARIATIONS_MAX, "",
	  true, "How many different trees the species grows." },
};

const voe_assets_documented_key *voe_assets_tree_keys(uint32_t *count)
{
	VOE_BASE_ASSERT(count != NULL, "counting tree keys into nothing");
	*count = (uint32_t)(sizeof(documented_keys) / sizeof(documented_keys[0]));
	return documented_keys;
}
