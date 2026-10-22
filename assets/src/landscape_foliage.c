// The landscape file's `[Foliage]` and `[Copies]` behind landscape_foliage.h:
// quoted kind paths read to the first missing key, and one line of six whole
// numbers a copy, both read into the landscape and printed back.
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
// A KIND'S SIZE `size<k>` FOLLOWS `kind<k>` (0460 point 1), whole thousandths
// from 100 to 10000, printed only when neither 0 nor 1000: a 1× kind's lines
// are as they were before sizes, and 0 in memory means 1× (point 2).
#include "landscape_foliage.h"

#include <base/assert.h>
#include <base/report.h>

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

// Kind k's optional `size<k>` into `into`, left 0 when missing.
static bool read_kind_size(const voe_assets_sectioned *doc, uint32_t section,
			   uint32_t k, voe_assets_landscape *into,
			   voe_base_error *error)
{
	char name[16];
	const char *value;
	char *end;
	long long parsed;

	snprintf(name, sizeof(name), "size%u", k);
	value = voe_assets_sectioned_value(doc, section, name);
	if (value == NULL)
		return true;
	errno = 0;
	parsed = strtoll(value, &end, 10);
	if (end == value || *end != '\0' || errno == ERANGE ||
	    parsed < VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN ||
	    parsed > VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX)
		return refuse(error, VOE_BASE_ERROR_MALFORMED, name,
			      "a size not a whole number from 100 to 10000");
	into->kind_sizes[k] = (uint16_t)parsed;
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
		if (!read_kind_size(doc, section, k, into, error))
			return false;
		into->kind_count = k + 1;
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

float voe_assets_landscape_kind_size(const voe_assets_landscape *landscape,
				     uint32_t kind)
{
	VOE_BASE_ASSERT(landscape != NULL, "the kind size of no landscape");
	VOE_BASE_ASSERT(kind < VOE_ASSETS_LANDSCAPE_KINDS,
			"a kind past the most a landscape holds");

	const uint16_t thousandths = landscape->kind_sizes[kind];

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
	       read_copies(doc, arena, into, error);
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
		const uint16_t size = landscape->kind_sizes[k];

		VOE_BASE_ASSERT(size == 0 ||
					(size >= VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN &&
					 size <= VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX),
				"writing a kind size the reader would refuse");
		voe_assets_landscape_quote(landscape->kinds[k], quoted);
		voe_assets_landscape_print(printer, "kind%u=%s\n", k, quoted);
		if (size != 0 && size != 1000)
			voe_assets_landscape_print(printer, "size%u=%u\n", k,
						   (unsigned)size);
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
