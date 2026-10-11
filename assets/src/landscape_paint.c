// The landscape file's `[Paint]` behind landscape_paint.h: rows of whole
// numbers and `v*n` runs read into two RGBA8 planes, printed back with runs of
// three or more folded, and resampled bilinearly per weight.
//
// THE SECTION'S KEYS ARE WALKED ONCE, NOT LOOKED UP BY NAME. Seven layers at
// 2048 cells is 14 343 rows, and a lookup per row is a scan per row: 10⁸
// compares. A key counts only when its name is exactly `paint<k>_<r>` with no
// leading zeros, and the sectioned reader refuses a key twice, so counting the
// rows read is checking that none is missing. A key for a layer or row the
// file does not have is passed over, as other readers here pass over a key
// they do not know.
//
// LAYER 0 IS COMPUTED, NEVER READ: 255 minus layers 1-7, after every row is in,
// so a point past 255 is refused wherever its rows lie.
#include "landscape_paint.h"

#include <base/assert.h>
#include <base/report.h>

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t point_count(uint32_t cells)
{
	return ((size_t)cells + 1) * ((size_t)cells + 1);
}

static size_t weight_index(size_t points, size_t point, uint32_t layer)
{
	return (size_t)(layer / 4) * points * 4 + point * 4 + layer % 4;
}

static bool refuse(voe_base_error *error, const char *key,
		   const char *message)
{
	VOE_BASE_ERROR("assets", "landscape: %s: %s", key, message);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

// Digits only at `at`, no sign, into `value` if no more than `most`. Returns
// the byte after them, or NULL.
static const char *digits(const char *at, unsigned long most,
			  unsigned long *value)
{
	char *end;

	if (*at < '0' || *at > '9')
		return NULL;
	errno = 0;
	*value = strtoul(at, &end, 10);
	return errno == ERANGE || *value > most ? NULL : end;
}

// One token at `at`: `v` or `v*n`, v 0-255 and n at least 1, then a blank or
// the end. Returns the byte after it, or NULL.
static const char *run_token(const char *at, unsigned long *value,
			     unsigned long *count)
{
	*count = 1;
	at = digits(at, 255, value);
	if (at != NULL && *at == '*') {
		at = digits(at + 1, VOE_ASSETS_LANDSCAPE_CELLS_MAX + 1, count);
		if (at != NULL && *count == 0)
			return NULL;
	}
	if (at == NULL || (*at != '\0' && *at != ' ' && *at != '\t'))
		return NULL;
	return at;
}

// Row `text` of `layer` into the paint from point `first`: cells + 1 weights.
// NULL when it is, else what is wrong with it. Each token is at least one
// weight, so at most cells + 1 tokens are read before the row is too long.
static const char *read_row(const char *text, uint32_t cells, uint8_t *paint,
			    size_t points, uint32_t layer, size_t first)
{
	const char *at = text;
	uint32_t c = 0;

	for (uint32_t token = 0; token <= cells + 1; token++) {
		unsigned long value;
		unsigned long count;

		while (*at == ' ' || *at == '\t')
			at++;
		if (*at == '\0')
			return c == cells + 1 ? NULL : "too few numbers";
		at = run_token(at, &value, &count);
		if (at == NULL)
			return "a value that is not a whole number 0 to 255";
		if (count > cells + 1 - c)
			return "too many numbers";
		for (unsigned long i = 0; i < count; i++)
			paint[weight_index(points, first + c + i, layer)] =
				(uint8_t)value;
		c += (uint32_t)count;
	}
	return "too many numbers";
}

// Layer and row of a key named exactly `paint<k>_<r>`; false for any other.
static bool paint_key(const char *name, uint32_t *layer, uint32_t *row)
{
	char canonical[32];
	unsigned long k;
	unsigned long r;
	const char *at;

	if (strncmp(name, "paint", 5) != 0)
		return false;
	at = digits(name + 5, VOE_ASSETS_LANDSCAPE_LAYERS, &k);
	if (at == NULL || *at != '_')
		return false;
	at = digits(at + 1, VOE_ASSETS_LANDSCAPE_CELLS_MAX, &r);
	if (at == NULL || *at != '\0')
		return false;
	snprintf(canonical, sizeof(canonical), "paint%lu_%lu", k, r);
	*layer = (uint32_t)k;
	*row = (uint32_t)r;
	return strcmp(canonical, name) == 0;
}

// Every row of `section` for layers 1 to layer_count - 1 into the paint.
static bool read_rows(const voe_assets_sectioned *doc, uint32_t section,
		      uint32_t cells, uint32_t layer_count, uint8_t *paint,
		      voe_base_error *error)
{
	const voe_assets_sectioned_section *rows = &doc->sections[section];
	const size_t points = point_count(cells);
	size_t read = 0;

	for (uint32_t i = 0; i < rows->key_count; i++) {
		const voe_assets_sectioned_key *key =
			&doc->keys[rows->first_key + i];
		const char *wrong;
		uint32_t layer;
		uint32_t row;

		if (!paint_key(key->name, &layer, &row) || layer == 0 ||
		    layer >= layer_count || row > cells)
			continue;
		wrong = read_row(key->value, cells, paint, points, layer,
				 (size_t)row * (cells + 1));
		if (wrong != NULL)
			return refuse(error, key->name, wrong);
		read++;
	}
	if (read != (size_t)(layer_count - 1) * (cells + 1))
		return refuse(error, "[Paint]", "a row missing");
	return true;
}

uint8_t *voe_assets_landscape_paint_read(const voe_assets_sectioned *doc,
					 uint32_t cells, uint32_t layer_count,
					 voe_base_arena *arena,
					 voe_base_error *error)
{
	const size_t points = point_count(cells);
	const uint32_t section = voe_assets_sectioned_find(doc, "Paint");
	// A push is zeroed, so a layer with no rows weighs nothing.
	uint8_t *paint = voe_base_arena_push(arena, 8 * points);

	VOE_BASE_ASSERT(layer_count >= 1 &&
				layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"reading paint with no layers");
	VOE_BASE_ASSERT(paint != NULL, "no room for a landscape's paint");

	if (section != VOE_ASSETS_SECTIONED_NONE &&
	    !read_rows(doc, section, cells, layer_count, paint, error))
		return NULL;
	for (size_t p = 0; p < points; p++) {
		uint32_t sum = 0;

		for (uint32_t k = 1; k < VOE_ASSETS_LANDSCAPE_LAYERS; k++)
			sum += paint[weight_index(points, p, k)];
		if (sum > 255) {
			refuse(error, "[Paint]", "a point whose layers pass 255");
			return NULL;
		}
		paint[weight_index(points, p, 0)] = (uint8_t)(255 - sum);
	}
	return paint;
}

// One row of `layer` from point `first`, a run of three or more as `v*n`.
static void print_row(struct voe_assets_landscape_printer *printer,
		      const uint8_t *paint, size_t points, uint32_t layer,
		      size_t first, uint32_t cells)
{
	// Each pass prints a run of at least one, so cells + 1 passes at most.
	for (uint32_t c = 0; c <= cells;) {
		const unsigned value =
			paint[weight_index(points, first + c, layer)];
		uint32_t run = 1;

		while (c + run <= cells &&
		       paint[weight_index(points, first + c + run, layer)] ==
			       value)
			run++;
		if (run >= 3)
			voe_assets_landscape_print(printer, "%u*%u", value,
						   run);
		else if (run == 2)
			voe_assets_landscape_print(printer, "%u %u", value,
						   value);
		else
			voe_assets_landscape_print(printer, "%u", value);
		c += run;
		voe_assets_landscape_print(printer, c <= cells ? " " : "\n");
	}
}

void voe_assets_landscape_paint_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape)
{
	const uint32_t cells = landscape->cells;
	const size_t points = point_count(cells);

	VOE_BASE_ASSERT(printer != NULL, "printing paint to no printer");
	VOE_BASE_ASSERT(landscape->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"printing more layers than a landscape holds");

	if (landscape->paint == NULL || landscape->layer_count < 2)
		return;
	voe_assets_landscape_print(printer, "\n[Paint]\n");
	for (uint32_t k = 1; k < landscape->layer_count; k++)
		for (uint32_t row = 0; row <= cells; row++) {
			voe_assets_landscape_print(printer, "paint%u_%u=", k,
						   row);
			print_row(printer, landscape->paint, points, k,
				  (size_t)row * (cells + 1), cells);
		}
}

// Layer `layer`'s weight bilinear between rows starting at points `near` and
// `far`, columns x0 and x1.
static float bilinear(const uint8_t *paint, size_t points, uint32_t layer,
		      size_t near, size_t far, uint32_t x0, uint32_t x1,
		      float fx, float fz)
{
	const float n0 = paint[weight_index(points, near + x0, layer)];
	const float n1 = paint[weight_index(points, near + x1, layer)];
	const float f0 = paint[weight_index(points, far + x0, layer)];
	const float f1 = paint[weight_index(points, far + x1, layer)];
	const float a = n0 + (n1 - n0) * fx;
	const float b = f0 + (f1 - f0) * fx;

	return a + (b - a) * fz;
}

// Rounding seven weights may pass 255 by up to three where their exact sum
// does not, so a weight is capped at what the earlier ones left.
static void blend_point(const uint8_t *from, size_t from_points, size_t near,
			size_t far, uint32_t x0, uint32_t x1, float fx, float fz,
			uint8_t *to, size_t points, size_t point)
{
	uint32_t sum = 0;

	for (uint32_t k = 1; k < VOE_ASSETS_LANDSCAPE_LAYERS; k++) {
		uint32_t weight = (uint32_t)lroundf(bilinear(
			from, from_points, k, near, far, x0, x1, fx, fz));

		if (weight > 255 - sum)
			weight = 255 - sum;
		to[weight_index(points, point, k)] = (uint8_t)weight;
		sum += weight;
	}
	to[weight_index(points, point, 0)] = (uint8_t)(255 - sum);
}

uint8_t *voe_assets_landscape_paint_resample(const voe_assets_landscape *from,
					     uint32_t cells,
					     voe_base_arena *arena)
{
	const uint32_t from_cells = from->cells;
	const size_t from_points = point_count(from_cells);
	const size_t points = point_count(cells);
	uint8_t *paint;

	VOE_BASE_ASSERT(arena != NULL, "resampling paint with no arena");
	if (from->paint == NULL)
		return NULL;
	paint = voe_base_arena_push(arena, 8 * points);
	VOE_BASE_ASSERT(paint != NULL, "no room for a landscape's paint");
	for (uint32_t row = 0; row <= cells; row++) {
		float fz;
		const uint32_t z0 = voe_assets_landscape_source_index(
			row, cells, from_cells, &fz);
		const uint32_t z1 = z0 < from_cells ? z0 + 1 : z0;

		for (uint32_t c = 0; c <= cells; c++) {
			float fx;
			const uint32_t x0 = voe_assets_landscape_source_index(
				c, cells, from_cells, &fx);
			const uint32_t x1 = x0 < from_cells ? x0 + 1 : x0;

			blend_point(from->paint, from_points,
				    (size_t)z0 * (from_cells + 1),
				    (size_t)z1 * (from_cells + 1), x0, x1, fx,
				    fz, paint, points,
				    (size_t)row * (cells + 1) + c);
		}
	}
	return paint;
}
