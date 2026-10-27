// The landscape file's `[Foliage]`, `[Layers]`' grow keys and `[Copies]` behind
// landscape_foliage.h: quoted kind paths read to the first missing key with
// their looks, each layer's density of each kind, and one line of six whole
// numbers a copy, all read into the landscape and printed back.
//
// THE COPIES' KEYS ARE WALKED ONCE, NOT LOOKED UP BY NAME, as
// `landscape_paint.c` walks its rows: 65536 lookups would each scan the
// section. Key i must be named exactly `c<i>`, so a key out of order, missing
// or misnamed is refused where it stands, and the count of keys is the count
// of copies, checked against VOE_ASSETS_LANDSCAPE_COPIES before anything is
// pushed.
//
// A COPY IS `kind x z turn scale lean` in the text (0442 point 1) and
// {x, z, turn, scale, kind, lean} in memory, the order a cooked initializer
// fills; each number is range-checked against its field before it is narrowed.
//
// A KIND'S LOOK FOLLOWS `kind<k>` (0460 point 1, 0476 point 1, 0481 point 4):
// `size<k>`, `vary<k>`, `turn<k>`, `tilt<k>`, `shadow<k>`, `blend<k>` and
// `blendheight<k>`, each a whole number in its range, one table for the read
// and the print. A key is printed only off the default, and 1000 is a size's
// default too: a 1× kind's lines are as they were before sizes, and 0 in memory
// means 1× (0460 point 2). So is 50 a blend height's, 0 in memory meaning 50.
//
// [Layers] IS WALKED ONCE FOR `grow<l>_<k>` (0476 point 2): any key starting
// `grow`, so one naming a layer or kind not in the file is refused, not
// skipped. It is read after the kinds, since its k is checked against them.
#include "landscape_foliage.h"

#include <base/assert.h>
#include <base/report.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert(VOE_ASSETS_LANDSCAPE_KIND_PATH == VOE_ASSETS_MATERIAL_PATH,
	      "a kind path is quoted in a material path's room");
static_assert(VOE_ASSETS_LANDSCAPE_KINDS <= UINT8_MAX + 1,
	      "a copy's kind is a byte");

// The six numbers of a copy's line, in the text's order.
enum { KIND, X, Z, TURN, SCALE, LEAN, NUMBERS };

static bool refuse(voe_base_error *error, voe_base_error code, const char *key,
		   const char *message)
{
	VOE_BASE_ERROR("assets", "landscape: %s: %s", key, message);
	if (error != NULL)
		*error = code;
	return false;
}

// A look's keys, in the order a kind's lines print them.
enum {
	LOOK_SIZE,
	LOOK_VARY,
	LOOK_TURN,
	LOOK_TILT,
	LOOK_SHADOW,
	LOOK_BLEND,
	LOOK_BLEND_HEIGHT,
	LOOK_KEYS
};

static const struct {
	const char *key;
	long long lowest, highest;
	const char *wrong;
} look_keys[LOOK_KEYS] = {
	{ "size", VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN,
	  VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX,
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
}

// Kind k's look keys into `into`, each missing one the default's.
static bool read_look(const voe_assets_sectioned *doc, uint32_t section,
		      uint32_t k, voe_assets_landscape *into,
		      voe_base_error *error)
{
	long long values[LOOK_KEYS];

	look_values(voe_assets_landscape_look_default(), values);
	for (uint32_t i = 0; i < LOOK_KEYS; i++) {
		char name[16];
		const char *value;

		snprintf(name, sizeof(name), "%s%u", look_keys[i].key, k);
		value = voe_assets_sectioned_value(doc, section, name);
		if (value != NULL &&
		    !whole_in_range(value, look_keys[i].lowest,
				    look_keys[i].highest, &values[i]))
			return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
				      look_keys[i].wrong);
	}
	into->looks[k] = (voe_assets_landscape_look){
		.size = (uint16_t)values[LOOK_SIZE],
		.size_vary = (uint8_t)values[LOOK_VARY],
		.turn_vary = (uint8_t)values[LOOK_TURN],
		.tilt = (uint8_t)values[LOOK_TILT],
		.shadowless = values[LOOK_SHADOW] == 0,
		.blends = values[LOOK_BLEND] == 1,
		.blend_height = (uint16_t)values[LOOK_BLEND_HEIGHT],
	};
	return true;
}

// `[Foliage]`'s paths from kind0= to the first missing key, into `into`.
static bool read_kinds(const voe_assets_sectioned *doc, const char *text,
		       size_t size, voe_assets_landscape *into,
		       voe_base_error *error)
{
	const uint32_t section = voe_assets_sectioned_find(doc, "Foliage");

	if (section == VOE_ASSETS_SECTIONED_NONE)
		return true;
	for (uint32_t k = 0; k < VOE_ASSETS_LANDSCAPE_KINDS; k++) {
		char name[16];
		const char *path;
		size_t length;

		snprintf(name, sizeof(name), "kind%u", k);
		path = voe_assets_sectioned_value(doc, section, name);
		if (path == NULL)
			break;
		if (!voe_assets_landscape_written_quoted(doc, section, name,
							 text, size))
			return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
				      "a path not quoted");
		length = strlen(path);
		if (length >= VOE_ASSETS_LANDSCAPE_KIND_PATH)
			return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
				      "a path of 128 bytes or more");
		memcpy(into->kinds[k], path, length + 1);
		if (!read_look(doc, section, k, into, error))
			return false;
		into->kind_count = k + 1;
	}
	return true;
}

// `grow<l>_<k>`'s l and k; false when `name` is not shaped so.
static bool grow_key(const char *name, unsigned long *layer,
		     unsigned long *kind)
{
	char *end;

	if (!isdigit((unsigned char)name[4]))
		return false;
	*layer = strtoul(name + 4, &end, 10);
	if (*end != '_' || !isdigit((unsigned char)end[1]))
		return false;
	*kind = strtoul(end + 1, &end, 10);
	return *end == '\0';
}

// `[Layers]`' keys walked once, each starting `grow` a layer's density of a
// kind, into `into`, whose layer and kind counts are read already.
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
		unsigned long kind;
		long long density;

		if (strncmp(key->name, "grow", 4) != 0)
			continue;
		if (!grow_key(key->name, &layer, &kind) ||
		    layer >= into->layer_count || kind >= into->kind_count)
			return refuse(error, VOE_BASE_ERROR_MALFORMED, key->name,
				      "a grow key whose layer or kind is not in "
				      "the file");
		if (!whole_in_range(key->value, 1, VOE_ASSETS_LANDSCAPE_GROW_MAX,
				    &density))
			return refuse(error, VOE_BASE_ERROR_MALFORMED, key->name,
				      "a density not a whole number from 1 to "
				      "500");
		into->grows[layer][kind] = (uint16_t)density;
	}
	return true;
}

// One copy's six whole numbers into `copy`. NULL when they are, else what is
// wrong with them.
static const char *read_copy(const char *text, uint32_t kind_count,
			     voe_assets_landscape_copy *copy)
{
	const int64_t lowest[NUMBERS] = { 0, INT32_MIN, INT32_MIN, 0, 1, 0 };
	const int64_t highest[NUMBERS] = { (int64_t)kind_count - 1,
					   INT32_MAX,
					   INT32_MAX,
					   UINT16_MAX,
					   UINT16_MAX,
					   UINT8_MAX };
	static const char *const out_of_range[NUMBERS] = {
		"a kind not below the kind count",
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
		.kind = (uint8_t)value[KIND],
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
		wrong = read_copy(key->value, into->kind_count,
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
	};
}

float voe_assets_landscape_kind_size(const voe_assets_landscape *landscape,
				     uint32_t kind)
{
	VOE_BASE_ASSERT(landscape != NULL, "the kind size of no landscape");
	VOE_BASE_ASSERT(kind < VOE_ASSETS_LANDSCAPE_KINDS,
			"a kind past the most a landscape holds");

	const uint16_t thousandths = landscape->looks[kind].size;

	return thousandths == 0 ? 1.0f : (float)thousandths / 1000.0f;
}

bool voe_assets_landscape_foliage_read(const voe_assets_sectioned *doc,
				       const char *text, size_t size,
				       voe_base_arena *arena,
				       voe_assets_landscape *into,
				       voe_base_error *error)
{
	VOE_BASE_ASSERT(doc != NULL && text != NULL && into != NULL,
			"reading foliage from or into nothing");
	VOE_BASE_ASSERT(arena != NULL, "reading foliage with no arena");

	return read_kinds(doc, text, size, into, error) &&
	       read_grows(doc, into, error) &&
	       read_copies(doc, arena, into, error);
}

void voe_assets_landscape_grows_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape)
{
	VOE_BASE_ASSERT(printer != NULL, "printing grows to no printer");
	VOE_BASE_ASSERT(landscape->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS &&
				landscape->kind_count <=
					VOE_ASSETS_LANDSCAPE_KINDS,
			"printing grows past what a landscape holds");

	for (uint32_t l = 0; l < landscape->layer_count; l++)
		for (uint32_t k = 0; k < landscape->kind_count; k++) {
			const uint16_t density = landscape->grows[l][k];

			VOE_BASE_ASSERT(density <= VOE_ASSETS_LANDSCAPE_GROW_MAX,
					"writing a density the reader would refuse");
			if (density > 0)
				voe_assets_landscape_print(printer,
							   "grow%u_%u=%u\n", l, k,
							   (unsigned)density);
		}
}

// Kind k's look keys, each only off the default.
static void print_look(struct voe_assets_landscape_printer *printer,
		       uint32_t k, voe_assets_landscape_look look)
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

		VOE_BASE_ASSERT(is_default || (values[i] >= look_keys[i].lowest &&
					       values[i] <= look_keys[i].highest),
				"writing a look the reader would refuse");
		if (!is_default)
			voe_assets_landscape_print(printer, "%s%u=%lld\n",
						   look_keys[i].key, k,
						   values[i]);
	}
}

void voe_assets_landscape_kinds_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape)
{
	VOE_BASE_ASSERT(printer != NULL, "printing kinds to no printer");
	VOE_BASE_ASSERT(landscape->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"printing more kinds than a landscape holds");

	if (landscape->kind_count == 0)
		return;
	voe_assets_landscape_print(printer, "[Foliage]\n");
	for (uint32_t k = 0; k < landscape->kind_count; k++) {
		char quoted[VOE_ASSETS_LANDSCAPE_QUOTED_PATH];

		voe_assets_landscape_quote(landscape->kinds[k], quoted);
		voe_assets_landscape_print(printer, "kind%u=%s\n", k, quoted);
		print_look(printer, k, landscape->looks[k]);
	}
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

		VOE_BASE_ASSERT(copy->kind < landscape->kind_count &&
					copy->scale > 0,
				"writing a copy the reader would refuse");
		voe_assets_landscape_print(printer, "c%u=%u %d %d %u %u %u\n", i,
					   (unsigned)copy->kind, (int)copy->x,
					   (int)copy->z, (unsigned)copy->turn,
					   (unsigned)copy->scale,
					   (unsigned)copy->lean);
	}
}
