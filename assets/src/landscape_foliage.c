// The landscape file's `[Foliage]`, `[Layers]`' grow keys and `[Copies]` behind
// landscape_foliage.h: groups of quoted glb paths with their looks and seeds,
// each layer's density of each group, and one line of six whole numbers a
// copy, all read into the landscape and printed back.
//
// THE COPIES' KEYS ARE WALKED ONCE, NOT LOOKED UP BY NAME, as
// `landscape_paint.c` walks its rows: 65536 lookups would each scan the
// section. Key i must be named exactly `c<i>`, so a key out of order, missing
// or misnamed is refused where it stands, and the count of keys is the count
// of copies, checked against VOE_ASSETS_LANDSCAPE_COPIES before anything is
// pushed.
//
// A COPY IS `group x z turn scale lean` in the text (0442 point 1) and
// {x, z, turn, scale, group, lean} in memory, the order a cooked initializer
// fills; each number is range-checked against its field before it is narrowed.
//
// A GROUP IS `glb<g>_<j>` PATHS (0521 point 1), each glb's look after it:
// `size`, `vary`, `turn`, `tilt`, `shadow`, `blend`, `blendheight` and `share`
// with `<g>_<j>`, each a whole number in its range, one table for the read and
// the print. A key is printed only off the default, and 1000 is a size's
// default too: 0 in memory means 1× (0460 point 2). So is 50 a blend height's,
// 0 in memory meaning 50. `seed<g>`'s default is g itself (0517 point 4): read
// as g it is 0 in memory, any other s is s + 1, and only the lenient read
// clamps one out of range. A share has no such stand-in: 0 is a share.
//
// THE OLD SPELLING, `kind<k>` with `<key><k>` look keys and no share, is told
// apart by one walk of [Foliage] before anything is read: a key `kind<digit>`
// is the old, `glb<digit>` the new, and both is malformed. Each kind reads as
// group k holding one glb; the printer knows only groups.
//
// [Layers] IS WALKED ONCE FOR `grow<l>_<g>` (0476 point 2): any key starting
// `grow`, so one naming a layer or group not in the file is refused, not
// skipped. It is read after the groups, since its g is checked against them.
//
// A STRICT READ WALKS [Foliage] ONCE MORE, after the groups: each key a glb's
// path or look key or a group's seed, in the file's spelling and numbered
// within its counts, or refused.
#include "landscape_foliage.h"

#include <base/assert.h>
#include <base/report.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert(VOE_ASSETS_LANDSCAPE_GLB_PATH == VOE_ASSETS_MATERIAL_PATH,
	      "a glb path is quoted in a material path's room");
static_assert(VOE_ASSETS_LANDSCAPE_GROUPS <= UINT8_MAX + 1,
	      "a copy's group is a byte");

// The six numbers of a copy's line, in the text's order.
enum { GROUP, X, Z, TURN, SCALE, LEAN, NUMBERS };

// Room for a foliage key's name, `blendheight15_7` and more.
enum { KEY_ROOM = 32 };

static bool refuse(voe_base_error *error, voe_base_error code, const char *key,
		   const char *message)
{
	VOE_BASE_ERROR("assets", "landscape: %s: %s", key, message);
	if (error != NULL)
		*error = code;
	return false;
}

// A look's keys, in the order a glb's lines print them; the old spelling has
// those before the share.
enum {
	LOOK_SIZE,
	LOOK_VARY,
	LOOK_TURN,
	LOOK_TILT,
	LOOK_SHADOW,
	LOOK_BLEND,
	LOOK_BLEND_HEIGHT,
	LOOK_SHARE,
	LOOK_KEYS
};

static const struct {
	const char *key;
	long long lowest, highest;
	const char *wrong;
} look_keys[LOOK_KEYS] = {
	{ "size", VOE_ASSETS_LANDSCAPE_GLB_SIZE_MIN,
	  VOE_ASSETS_LANDSCAPE_GLB_SIZE_MAX,
	  "a size not a whole number from 100 to 10000" },
	{ "vary", 0, VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX,
	  "a size spread not a whole number from 0 to 100" },
	{ "turn", 0, VOE_ASSETS_LANDSCAPE_TURN_VARY_MAX,
	  "a turn spread not a whole number from 0 to 180" },
	{ "tilt", 0, VOE_ASSETS_LANDSCAPE_TILT_MAX,
	  "a tilt not a whole number from 0 to 100" },
	{ "shadow", 0, 1, "a shadow not 0 or 1" },
	{ "blend", 0, 1, "a blend not 0 or 1" },
	{ "blendheight", VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MIN,
	  VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_MAX,
	  "a blend height not a whole number from 5 to 1000" },
	{ "share", 0, VOE_ASSETS_LANDSCAPE_SHARE_MAX,
	  "a share not a whole number from 0 to 10" },
};

// What every group's read takes from the file.
struct foliage_source {
	const voe_assets_sectioned *doc;
	const char *text;
	size_t size;
	uint32_t section;
	// Spelled with `kind<k>` keys, each a group of one glb.
	bool old;
	bool strict;
};

// `value` as a whole number from `lowest` to `highest` with nothing after it.
static bool whole_in_range(const char *value, long long lowest,
			   long long highest, long long *out)
{
	char *end;
	long long parsed;

	errno = 0;
	parsed = strtoll(value, &end, 10);
	if (end == value || *end != '\0' || errno == ERANGE || parsed < lowest ||
	    parsed > highest)
		return false;
	*out = parsed;
	return true;
}

// `look` as its keys' values, LOOK_KEYS of them, shadow 1 for casting and
// blend 1 for blending.
static void look_values(voe_assets_landscape_look look, long long *values)
{
	values[LOOK_SIZE] = look.size;
	values[LOOK_VARY] = look.size_vary;
	values[LOOK_TURN] = look.turn_vary;
	values[LOOK_TILT] = look.tilt;
	values[LOOK_SHADOW] = look.shadowless ? 0 : 1;
	values[LOOK_BLEND] = look.blends ? 1 : 0;
	values[LOOK_BLEND_HEIGHT] = look.blend_height;
	values[LOOK_SHARE] = look.share;
}

// Key `stem` of glb j of group g into `name`, KEY_ROOM bytes: `<stem><g>_<j>`,
// or `<stem><g>` in the old spelling, whose one glb is j 0.
static void glb_key(char *name, const char *stem, uint32_t g, uint32_t j,
		    bool old)
{
	VOE_BASE_ASSERT(!old || j == 0, "a kind holds one glb");

	const int written = old ? snprintf(name, KEY_ROOM, "%s%u", stem, g) :
				  snprintf(name, KEY_ROOM, "%s%u_%u", stem, g, j);

	VOE_BASE_ASSERT(written > 0 && written < KEY_ROOM,
			"a foliage key outgrew its room");
}

// Glb j of group g's look keys into `group`, each missing one the default's.
static bool read_look(const struct foliage_source *source, uint32_t g,
		      uint32_t j, voe_assets_landscape_group *group,
		      voe_base_error *error)
{
	const uint32_t keys = source->old ? LOOK_SHARE : LOOK_KEYS;
	long long values[LOOK_KEYS];

	look_values(voe_assets_landscape_look_default(), values);
	for (uint32_t i = 0; i < keys; i++) {
		char name[KEY_ROOM];
		const char *value;

		glb_key(name, look_keys[i].key, g, j, source->old);
		value = voe_assets_sectioned_value(source->doc, source->section,
						   name);
		if (value != NULL &&
		    !whole_in_range(value, look_keys[i].lowest,
				    look_keys[i].highest, &values[i]))
			return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
				      look_keys[i].wrong);
	}
	group->looks[j] = (voe_assets_landscape_look){
		.size = (uint16_t)values[LOOK_SIZE],
		.size_vary = (uint8_t)values[LOOK_VARY],
		.turn_vary = (uint8_t)values[LOOK_TURN],
		.tilt = (uint8_t)values[LOOK_TILT],
		.shadowless = values[LOOK_SHADOW] == 0,
		.blends = values[LOOK_BLEND] == 1,
		.blend_height = (uint16_t)values[LOOK_BLEND_HEIGHT],
		.share = (uint8_t)values[LOOK_SHARE],
	};
	return true;
}

// Group g's `seed<g>` into `group`, g when missing; one out of 0-15 clamped
// unless the read is strict.
static bool read_seed(const struct foliage_source *source, uint32_t g,
		      voe_assets_landscape_group *group, voe_base_error *error)
{
	const long long highest = VOE_ASSETS_LANDSCAPE_GROUPS - 1;
	long long seed = g;
	char name[KEY_ROOM];
	const char *value;

	snprintf(name, sizeof(name), "seed%u", g);
	value = voe_assets_sectioned_value(source->doc, source->section, name);
	if (value != NULL && source->strict &&
	    !whole_in_range(value, 0, highest, &seed))
		return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
			      "a seed not a whole number from 0 to 15");
	if (value != NULL && !source->strict) {
		if (!whole_in_range(value, LLONG_MIN, LLONG_MAX, &seed))
			return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
				      "a seed not a whole number");
		seed = seed < 0 ? 0 : seed > highest ? highest : seed;
	}
	group->seed = seed == g ? 0 : (uint8_t)(seed + 1);
	return true;
}

// Group g's glbs from j 0 to the first missing path, each with its look, then
// its seed, into `group`, which is left with no glbs when g has no first one.
static bool read_group(const struct foliage_source *source, uint32_t g,
		       voe_assets_landscape_group *group, voe_base_error *error)
{
	const uint32_t most = source->old ? 1 : VOE_ASSETS_LANDSCAPE_GROUP_GLBS;

	for (uint32_t j = 0; j < most; j++) {
		char name[KEY_ROOM];
		const char *path;
		size_t length;

		glb_key(name, source->old ? "kind" : "glb", g, j, source->old);
		path = voe_assets_sectioned_value(source->doc, source->section,
						  name);
		if (path == NULL)
			break;
		if (!voe_assets_landscape_written_quoted(source->doc,
							 source->section, name,
							 source->text,
							 source->size))
			return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
				      "a path not quoted");
		length = strlen(path);
		if (length >= VOE_ASSETS_LANDSCAPE_GLB_PATH)
			return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
				      "a path of 128 bytes or more");
		for (uint32_t earlier = 0; earlier < j; earlier++)
			if (strcmp(group->glbs[earlier], path) == 0)
				return refuse(error, VOE_BASE_ERROR_MALFORMED,
					      name, "a path twice in one group");
		memcpy(group->glbs[j], path, length + 1);
		if (!read_look(source, g, j, group, error))
			return false;
		group->glb_count = j + 1;
	}
	return group->glb_count == 0 || read_seed(source, g, group, error);
}

// `<stem><a>_<b>`'s a and b, the stem `at` bytes long; false when `name` is
// not shaped so.
static bool pair_key(const char *name, size_t at, unsigned long *first,
		     unsigned long *second)
{
	char *end;

	if (!isdigit((unsigned char)name[at]))
		return false;
	*first = strtoul(name + at, &end, 10);
	if (*end != '_' || !isdigit((unsigned char)end[1]))
		return false;
	*second = strtoul(end + 1, &end, 10);
	return *end == '\0';
}

// Whether `name` is exactly `<stem><g>_<j>` with no leading zero, g below
// `read`'s group count and j below that group's glb count.
static bool glb_numbered(const char *name, const char *stem,
			 const voe_assets_landscape *read)
{
	const size_t length = strlen(stem);
	char canonical[KEY_ROOM];
	unsigned long g;
	unsigned long j;

	if (strncmp(name, stem, length) != 0 ||
	    !pair_key(name, length, &g, &j) || g >= read->group_count ||
	    j >= read->groups[g].glb_count)
		return false;
	glb_key(canonical, stem, (uint32_t)g, (uint32_t)j, false);
	return strcmp(canonical, name) == 0;
}

// Every key of `[Foliage]` a glb's path or look key or a group's seed, in the
// source's spelling, within `read`'s counts.
static bool check_foliage_keys(const struct foliage_source *source,
			       const voe_assets_landscape *read,
			       voe_base_error *error)
{
	const voe_assets_sectioned_section *keys =
		&source->doc->sections[source->section];

	for (uint32_t i = 0; i < keys->key_count; i++) {
		const voe_assets_sectioned_key *key =
			&source->doc->keys[keys->first_key + i];
		bool taken = voe_assets_landscape_numbered(key->name, "seed",
							   read->group_count);

		if (source->old) {
			taken = taken ||
				voe_assets_landscape_numbered(key->name, "kind",
							      read->group_count);
			for (uint32_t l = 0; l < LOOK_SHARE && !taken; l++)
				taken = voe_assets_landscape_numbered(
					key->name, look_keys[l].key,
					read->group_count);
		} else {
			taken = taken || glb_numbered(key->name, "glb", read);
			for (uint32_t l = 0; l < LOOK_KEYS && !taken; l++)
				taken = glb_numbered(key->name, look_keys[l].key,
						     read);
		}
		if (!taken)
			return voe_assets_landscape_refuse_key(key, error);
	}
	return true;
}

// Whether `name` starts `stem` and a digit.
static bool spelled(const char *name, const char *stem)
{
	const size_t length = strlen(stem);

	return strncmp(name, stem, length) == 0 &&
	       isdigit((unsigned char)name[length]);
}

// `source`'s spelling from one walk of its keys: old when one is
// `kind<digit>…`; MALFORMED when one is that and another `glb<digit>…`.
static bool read_spelling(struct foliage_source *source,
			  voe_base_error *error)
{
	const voe_assets_sectioned_section *keys =
		&source->doc->sections[source->section];
	const char *kind = NULL;
	const char *glb = NULL;

	for (uint32_t i = 0; i < keys->key_count; i++) {
		const char *name = source->doc->keys[keys->first_key + i].name;

		if (kind == NULL && spelled(name, "kind"))
			kind = name;
		if (glb == NULL && spelled(name, "glb"))
			glb = name;
	}
	if (kind != NULL && glb != NULL)
		return refuse(error, VOE_BASE_ERROR_MALFORMED, glb,
			      "a group's glb in a file spelled with kinds");
	source->old = kind != NULL;
	return true;
}

// `[Foliage]`'s groups from group 0 to the first with no first glb, into
// `into`; when `strict`, then its keys checked.
static bool read_groups(const voe_assets_sectioned *doc, const char *text,
			size_t size, bool strict, voe_assets_landscape *into,
			voe_base_error *error)
{
	struct foliage_source source = {
		.doc = doc,
		.text = text,
		.size = size,
		.section = voe_assets_sectioned_find(doc, "Foliage"),
		.strict = strict,
	};

	if (source.section == VOE_ASSETS_SECTIONED_NONE)
		return true;
	if (!read_spelling(&source, error))
		return false;
	for (uint32_t g = 0; g < VOE_ASSETS_LANDSCAPE_GROUPS; g++) {
		if (!read_group(&source, g, &into->groups[g], error))
			return false;
		if (into->groups[g].glb_count == 0)
			break;
		into->group_count = g + 1;
	}
	return !strict || check_foliage_keys(&source, into, error);
}

// `[Layers]`' keys walked once, each starting `grow` a layer's density of a
// group, into `into`, whose layer and group counts are read already.
static bool read_grows(const voe_assets_sectioned *doc,
		       voe_assets_landscape *into, voe_base_error *error)
{
	const uint32_t section = voe_assets_sectioned_find(doc, "Layers");
	const voe_assets_sectioned_section *keys;

	if (section == VOE_ASSETS_SECTIONED_NONE)
		return true;
	keys = &doc->sections[section];
	for (uint32_t i = 0; i < keys->key_count; i++) {
		const voe_assets_sectioned_key *key =
			&doc->keys[keys->first_key + i];
		unsigned long layer;
		unsigned long group;
		long long density;

		if (strncmp(key->name, "grow", 4) != 0)
			continue;
		if (!pair_key(key->name, 4, &layer, &group) ||
		    layer >= into->layer_count || group >= into->group_count)
			return refuse(error, VOE_BASE_ERROR_MALFORMED, key->name,
				      "a grow key whose layer or group is not "
				      "in the file");
		if (!whole_in_range(key->value, 1, VOE_ASSETS_LANDSCAPE_GROW_MAX,
				    &density))
			return refuse(error, VOE_BASE_ERROR_MALFORMED, key->name,
				      "a density not a whole number from 1 to "
				      "500");
		into->grows[layer][group] = (uint16_t)density;
	}
	return true;
}

// One copy's six whole numbers into `copy`. NULL when they are, else what is
// wrong with them.
static const char *read_copy(const char *text, uint32_t group_count,
			     voe_assets_landscape_copy *copy)
{
	const int64_t lowest[NUMBERS] = { 0, INT32_MIN, INT32_MIN, 0, 1, 0 };
	const int64_t highest[NUMBERS] = { (int64_t)group_count - 1,
					   INT32_MAX,
					   INT32_MAX,
					   UINT16_MAX,
					   UINT16_MAX,
					   UINT8_MAX };
	static const char *const out_of_range[NUMBERS] = {
		"a group not below the group count",
		"an x past a 32-bit count of millimetres",
		"a z past a 32-bit count of millimetres",
		"a turn not 0 to 65535",
		"a scale not 1 to 65535",
		"a lean not 0 to 255",
	};
	int64_t value[NUMBERS];
	const char *at = text;

	for (uint32_t i = 0; i < NUMBERS; i++) {
		char *end;
		long long parsed;

		errno = 0;
		parsed = strtoll(at, &end, 10);
		if (end == at || (*end != '\0' && *end != ' ' && *end != '\t'))
			return "not six whole numbers";
		if (errno == ERANGE || parsed < lowest[i] ||
		    parsed > highest[i])
			return out_of_range[i];
		value[i] = parsed;
		at = end;
	}
	while (*at == ' ' || *at == '\t')
		at++;
	if (*at != '\0')
		return "not six whole numbers";
	*copy = (voe_assets_landscape_copy){
		.x = (int32_t)value[X],
		.z = (int32_t)value[Z],
		.turn = (uint16_t)value[TURN],
		.scale = (uint16_t)value[SCALE],
		.group = (uint8_t)value[GROUP],
		.lean = (uint8_t)value[LEAN],
	};
	return NULL;
}

// `[Copies]`' keys in file order, the i-th named `c<i>`, onto `arena`.
static bool read_copies(const voe_assets_sectioned *doc, voe_base_arena *arena,
			voe_assets_landscape *into, voe_base_error *error)
{
	const uint32_t section = voe_assets_sectioned_find(doc, "Copies");
	const voe_assets_sectioned_section *keys;

	if (section == VOE_ASSETS_SECTIONED_NONE)
		return true;
	keys = &doc->sections[section];
	if (keys->key_count > VOE_ASSETS_LANDSCAPE_COPIES)
		return refuse(error, VOE_BASE_ERROR_UNSUPPORTED, "[Copies]",
			      "more than 65536 copies");
	if (keys->key_count == 0)
		return true;
	into->copies = voe_base_arena_push(
		arena, (size_t)keys->key_count * sizeof(*into->copies));
	for (uint32_t i = 0; i < keys->key_count; i++) {
		const voe_assets_sectioned_key *key =
			&doc->keys[keys->first_key + i];
		char name[16];
		const char *wrong;

		snprintf(name, sizeof(name), "c%u", i);
		if (strcmp(key->name, name) != 0)
			return refuse(error, VOE_BASE_ERROR_MALFORMED, key->name,
				      "a copy key out of order or misnamed");
		wrong = read_copy(key->value, into->group_count,
				  &into->copies[i]);
		if (wrong != NULL)
			return refuse(error, VOE_BASE_ERROR_MALFORMED, key->name,
				      wrong);
	}
	into->copy_count = keys->key_count;
	return true;
}

voe_assets_landscape_look voe_assets_landscape_look_default(void)
{
	return (voe_assets_landscape_look){
		.size_vary = VOE_ASSETS_LANDSCAPE_SIZE_VARY_DEFAULT,
		.turn_vary = VOE_ASSETS_LANDSCAPE_TURN_VARY_DEFAULT,
		.tilt = VOE_ASSETS_LANDSCAPE_TILT_DEFAULT,
		.share = VOE_ASSETS_LANDSCAPE_SHARE_DEFAULT,
	};
}

bool voe_assets_landscape_foliage_read(const voe_assets_sectioned *doc,
				       const char *text, size_t size,
				       bool strict, voe_base_arena *arena,
				       voe_assets_landscape *into,
				       voe_base_error *error)
{
	VOE_BASE_ASSERT(doc != NULL && text != NULL && into != NULL,
			"reading foliage from or into nothing");
	VOE_BASE_ASSERT(arena != NULL, "reading foliage with no arena");

	return read_groups(doc, text, size, strict, into, error) &&
	       read_grows(doc, into, error) &&
	       read_copies(doc, arena, into, error);
}

void voe_assets_landscape_grows_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape)
{
	VOE_BASE_ASSERT(printer != NULL, "printing grows to no printer");
	VOE_BASE_ASSERT(landscape->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS &&
				landscape->group_count <=
					VOE_ASSETS_LANDSCAPE_GROUPS,
			"printing grows past what a landscape holds");

	for (uint32_t l = 0; l < landscape->layer_count; l++)
		for (uint32_t g = 0; g < landscape->group_count; g++) {
			const uint16_t density = landscape->grows[l][g];

			VOE_BASE_ASSERT(density <= VOE_ASSETS_LANDSCAPE_GROW_MAX,
					"writing a density the reader would refuse");
			if (density > 0)
				voe_assets_landscape_print(printer,
							   "grow%u_%u=%u\n", l, g,
							   (unsigned)density);
		}
}

// Glb j of group g's look keys, each only off the default.
static void print_look(struct voe_assets_landscape_printer *printer,
		       uint32_t g, uint32_t j, voe_assets_landscape_look look)
{
	long long values[LOOK_KEYS];
	long long defaults[LOOK_KEYS];

	look_values(look, values);
	look_values(voe_assets_landscape_look_default(), defaults);
	for (uint32_t i = 0; i < LOOK_KEYS; i++) {
		const bool is_default =
			values[i] == defaults[i] ||
			(i == LOOK_SIZE && values[i] == 1000) ||
			(i == LOOK_BLEND_HEIGHT &&
			 values[i] == VOE_ASSETS_LANDSCAPE_BLEND_HEIGHT_DEFAULT);
		char name[KEY_ROOM];

		VOE_BASE_ASSERT(is_default || (values[i] >= look_keys[i].lowest &&
					       values[i] <= look_keys[i].highest),
				"writing a look the reader would refuse");
		if (is_default)
			continue;
		glb_key(name, look_keys[i].key, g, j, false);
		voe_assets_landscape_print(printer, "%s=%lld\n", name,
					   values[i]);
	}
}

// Group g's glbs, each path then its look, and its seed when not g.
static void print_group(struct voe_assets_landscape_printer *printer,
			const voe_assets_landscape *landscape, uint32_t g)
{
	const voe_assets_landscape_group *group = &landscape->groups[g];
	const uint32_t seed = voe_assets_landscape_group_seed(landscape, g);

	VOE_BASE_ASSERT(group->glb_count >= 1 &&
				group->glb_count <= VOE_ASSETS_LANDSCAPE_GROUP_GLBS,
			"writing a group the reader would refuse");

	for (uint32_t j = 0; j < group->glb_count; j++) {
		char quoted[VOE_ASSETS_LANDSCAPE_QUOTED_PATH];

		for (uint32_t earlier = 0; earlier < j; earlier++)
			VOE_BASE_ASSERT(strcmp(group->glbs[earlier],
					       group->glbs[j]) != 0,
					"writing a path twice in one group");
		voe_assets_landscape_quote(group->glbs[j], quoted);
		voe_assets_landscape_print(printer, "glb%u_%u=%s\n", g, j,
					   quoted);
		print_look(printer, g, j, group->looks[j]);
	}
	if (seed != g)
		voe_assets_landscape_print(printer, "seed%u=%u\n", g, seed);
}

void voe_assets_landscape_groups_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape)
{
	VOE_BASE_ASSERT(printer != NULL, "printing groups to no printer");
	VOE_BASE_ASSERT(landscape->group_count <= VOE_ASSETS_LANDSCAPE_GROUPS,
			"printing more groups than a landscape holds");

	if (landscape->group_count == 0)
		return;
	voe_assets_landscape_print(printer, "[Foliage]\n");
	for (uint32_t g = 0; g < landscape->group_count; g++)
		print_group(printer, landscape, g);
	voe_assets_landscape_print(printer, "\n");
}

void voe_assets_landscape_copies_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape)
{
	VOE_BASE_ASSERT(printer != NULL, "printing copies to no printer");
	VOE_BASE_ASSERT(landscape->copy_count <= VOE_ASSETS_LANDSCAPE_COPIES &&
				(landscape->copy_count == 0 ||
				 landscape->copies != NULL),
			"printing copies a landscape does not hold");

	if (landscape->copy_count == 0)
		return;
	voe_assets_landscape_print(printer, "\n[Copies]\n");
	for (uint32_t i = 0; i < landscape->copy_count; i++) {
		const voe_assets_landscape_copy *copy = &landscape->copies[i];

		VOE_BASE_ASSERT(copy->group < landscape->group_count &&
					copy->scale > 0,
				"writing a copy the reader would refuse");
		voe_assets_landscape_print(printer, "c%u=%u %d %d %u %u %u\n", i,
					   (unsigned)copy->group, (int)copy->x,
					   (int)copy->z, (unsigned)copy->turn,
					   (unsigned)copy->scale,
					   (unsigned)copy->lean);
	}
}
