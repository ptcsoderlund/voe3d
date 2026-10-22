// The landscape file behind assets/landscape.h: the sectioned reader for the
// lines, then each value turned into a number or a path here, and a printer for
// the way back. `[Paint]` is `landscape_paint.c`'s, `[Foliage]` and `[Copies]`
// `landscape_foliage.c`'s.
//
// A ROW IS FOUND BY NAME, one linear scan of its section's keys per row, so a
// 2048-cell file is about two million string compares. That is tens of
// milliseconds; an index from row number to key would lift it.
//
// A LAYER'S OR KIND'S QUOTES ARE CHECKED IN THE TEXT. The sectioned reader
// strips them and does not say, so the key's line is found by counting line
// breaks from the start: 24 scans at most, each ending at `[Layers]` or
// `[Foliage]`, which the writer puts before the heights. A reader that kept
// whether a value was quoted would lift it.
//
// THE WRITER MEASURES, THEN PRINTS. A height is any float, so a millimetre
// count can be forty digits long; one pass with no buffer counts the bytes, one
// push holds exactly them, and the same pass prints into it. Paths are written
// quoted with `"` and `\` escaped, as the material writer does (0149).
#include "landscape_foliage.h"

#include <base/assert.h>
#include <base/report.h>

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool cells_supported(int64_t cells)
{
	return cells >= 4 && cells <= VOE_ASSETS_LANDSCAPE_CELLS_MAX &&
	       cells % 4 == 0;
}

static bool size_supported(double size)
{
	return size >= VOE_ASSETS_LANDSCAPE_SIZE_MIN &&
	       size <= VOE_ASSETS_LANDSCAPE_SIZE_MAX;
}

static size_t height_count(uint32_t cells)
{
	return ((size_t)cells + 1) * ((size_t)cells + 1);
}

static bool refuse(voe_base_error *error, voe_base_error code,
		   const char *message)
{
	VOE_BASE_ERROR("assets", "landscape: %s", message);
	if (error != NULL)
		*error = code;
	return false;
}

static bool refuse_row(voe_base_error *error, uint32_t row,
		       const char *message)
{
	VOE_BASE_ERROR("assets", "landscape: row%u: %s", row, message);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

static bool refuse_layer(voe_base_error *error, uint32_t layer,
			 const char *message)
{
	VOE_BASE_ERROR("assets", "landscape: layer%u: %s", layer, message);
	if (error != NULL)
		*error = VOE_BASE_ERROR_MALFORMED;
	return false;
}

// A whole number at `at`, blanks before it allowed. Returns the byte after it,
// which must be a blank or the end for the number to count as whole, or NULL.
static const char *whole_number(const char *at, int64_t *value)
{
	char *end;
	long long parsed;

	errno = 0;
	parsed = strtoll(at, &end, 10);
	if (end == at || errno == ERANGE ||
	    (*end != '\0' && *end != ' ' && *end != '\t'))
		return NULL;
	*value = parsed;
	return end;
}

// cells + 1 whole millimetres into `heights`, and nothing after them.
static bool read_row(const char *text, uint32_t row, uint32_t cells,
		     float *heights, voe_base_error *error)
{
	const char *at = text;

	for (uint32_t c = 0; c <= cells; c++) {
		int64_t millimetres;

		while (*at == ' ' || *at == '\t')
			at++;
		if (*at == '\0')
			return refuse_row(error, row, "too few numbers");
		at = whole_number(at, &millimetres);
		if (at == NULL)
			return refuse_row(error, row,
					  "a value that is not whole millimetres");
		heights[c] = (float)((double)millimetres / 1000.0);
	}
	while (*at == ' ' || *at == '\t')
		at++;
	if (*at != '\0')
		return refuse_row(error, row, "too many numbers");
	return true;
}

// `[Landscape]`'s two keys, each checked for shape and then for range.
static bool read_shape(const voe_assets_sectioned *doc, double *size,
		       uint32_t *cells, voe_base_error *error)
{
	uint32_t section = voe_assets_sectioned_find(doc, "Landscape");
	const char *size_text;
	const char *cells_text;
	const char *rest;
	char *end;
	int64_t count;

	if (section == VOE_ASSETS_SECTIONED_NONE)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "no [Landscape] section");
	size_text = voe_assets_sectioned_value(doc, section, "size");
	cells_text = voe_assets_sectioned_value(doc, section, "cells");
	if (size_text == NULL || cells_text == NULL)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "[Landscape] without size= or cells=");

	*size = strtod(size_text, &end);
	if (end == size_text || *end != '\0' || !isfinite(*size))
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "a size that is not a number");
	rest = whole_number(cells_text, &count);
	if (rest == NULL || *rest != '\0')
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "cells that is not a whole number");
	if (!size_supported(*size))
		return refuse(error, VOE_BASE_ERROR_UNSUPPORTED,
			      "a size outside 16 to 8192 metres");
	if (!cells_supported(count))
		return refuse(error, VOE_BASE_ERROR_UNSUPPORTED,
			      "cells not a multiple of 4 from 4 to 2048");
	*cells = (uint32_t)count;
	return true;
}

// The first non-blank after the `=` on the key's line is `"`.
bool voe_assets_landscape_written_quoted(const voe_assets_sectioned *doc,
					 uint32_t section, const char *name,
					 const char *text, size_t size)
{
	const voe_assets_sectioned_section *keys = &doc->sections[section];
	const char *const end = text + size;
	const char *at = text;
	uint32_t line = 1;

	for (uint32_t i = 0; i < keys->key_count; i++)
		if (strcmp(doc->keys[keys->first_key + i].name, name) == 0)
			line = doc->keys[keys->first_key + i].line;
	for (uint32_t l = 1; l < line; l++) {
		at = memchr(at, '\n', (size_t)(end - at));
		VOE_BASE_ASSERT(at != NULL, "a key's line is in its text");
		at++;
	}
	while (at < end && *at != '=')
		at++;
	at++;
	while (at < end && (*at == ' ' || *at == '\t'))
		at++;
	return at < end && *at == '"';
}

// `[Layers]`' paths from layer0= to the first missing key, into `into`.
static bool read_layers(const voe_assets_sectioned *doc, const char *text,
			size_t size, voe_assets_landscape *into,
			voe_base_error *error)
{
	const uint32_t section = voe_assets_sectioned_find(doc, "Layers");

	if (section == VOE_ASSETS_SECTIONED_NONE)
		return true;
	for (uint32_t k = 0; k < VOE_ASSETS_LANDSCAPE_LAYERS; k++) {
		char name[16];
		const char *path;
		size_t length;

		snprintf(name, sizeof(name), "layer%u", k);
		path = voe_assets_sectioned_value(doc, section, name);
		if (path == NULL)
			break;
		if (!voe_assets_landscape_written_quoted(doc, section, name,
							 text, size))
			return refuse_layer(error, k, "a path not quoted");
		length = strlen(path);
		if (length >= VOE_ASSETS_MATERIAL_PATH)
			return refuse_layer(error, k,
					    "a path of 128 bytes or more");
		memcpy(into->layers[k], path, length + 1);
		into->layer_count = k + 1;
	}
	return true;
}

voe_assets_landscape voe_assets_landscape_flat(float size, uint32_t cells,
					       voe_base_arena *arena)
{
	VOE_BASE_ASSERT(size_supported((double)size),
			"a landscape size out of range");
	VOE_BASE_ASSERT(cells_supported(cells),
			"landscape cells not a multiple of 4 from 4 to 2048");
	VOE_BASE_ASSERT(arena != NULL, "a landscape with no arena");

	// A push is zeroed, and zero is flat.
	return (voe_assets_landscape){
		.size = size,
		.cells = cells,
		.heights = voe_base_arena_push(
			arena, height_count(cells) * sizeof(float)),
	};
}

voe_assets_landscape
voe_assets_landscape_resample(const voe_assets_landscape *from, uint32_t cells,
			      voe_base_arena *arena)
{
	voe_assets_landscape to;
	uint32_t from_cells;

	VOE_BASE_ASSERT(from != NULL && from->heights != NULL &&
				cells_supported(from->cells),
			"resampling no landscape");
	VOE_BASE_ASSERT(cells_supported(cells),
			"landscape cells not a multiple of 4 from 4 to 2048");
	VOE_BASE_ASSERT(arena != NULL, "resampling a landscape with no arena");

	from_cells = from->cells;
	to = voe_assets_landscape_flat(from->size, cells, arena);
	for (uint32_t row = 0; row <= cells; row++) {
		float fz;
		const uint32_t z0 = voe_assets_landscape_source_index(
			row, cells, from_cells, &fz);
		const uint32_t z1 = z0 < from_cells ? z0 + 1 : z0;
		const float *near = from->heights + (size_t)z0 * (from_cells + 1);
		const float *far = from->heights + (size_t)z1 * (from_cells + 1);

		for (uint32_t c = 0; c <= cells; c++) {
			float fx;
			const uint32_t x0 = voe_assets_landscape_source_index(
				c, cells, from_cells, &fx);
			const uint32_t x1 = x0 < from_cells ? x0 + 1 : x0;
			const float a = near[x0] + (near[x1] - near[x0]) * fx;
			const float b = far[x0] + (far[x1] - far[x0]) * fx;

			to.heights[(size_t)row * (cells + 1) + c] =
				a + (b - a) * fz;
		}
	}
	memcpy(to.layers, from->layers, sizeof(to.layers));
	to.layer_count = from->layer_count;
	to.paint = voe_assets_landscape_paint_resample(from, cells, arena);
	memcpy(to.kinds, from->kinds, sizeof(to.kinds));
	to.kind_count = from->kind_count;
	if (from->copy_count > 0) {
		const size_t bytes = (size_t)from->copy_count * sizeof(*to.copies);

		to.copies = voe_base_arena_push(arena, bytes);
		memcpy(to.copies, from->copies, bytes);
		to.copy_count = from->copy_count;
	}
	return to;
}

bool voe_assets_landscape_read(const char *text, size_t size,
			       voe_base_arena *arena,
			       voe_assets_landscape *out,
			       voe_base_error *error)
{
	voe_assets_landscape result = { 0 };
	voe_assets_sectioned doc;
	uint32_t heights_section;
	double metres;

	VOE_BASE_ASSERT(text != NULL, "reading a landscape from nothing");
	VOE_BASE_ASSERT(arena != NULL, "reading a landscape with no arena");
	VOE_BASE_ASSERT(out != NULL, "reading a landscape into nothing");

	if (!voe_assets_sectioned_parse(text, size, arena, &doc))
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "not sectioned text");
	if (!read_shape(&doc, &metres, &result.cells, error))
		return false;
	heights_section = voe_assets_sectioned_find(&doc, "Heights");
	if (heights_section == VOE_ASSETS_SECTIONED_NONE)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "no [Heights] section");
	if (!read_layers(&doc, text, size, &result, error))
		return false;

	result.size = (float)metres;
	result.heights = voe_base_arena_push(
		arena, height_count(result.cells) * sizeof(float));
	for (uint32_t row = 0; row <= result.cells; row++) {
		char name[16];
		const char *line;

		snprintf(name, sizeof(name), "row%u", row);
		line = voe_assets_sectioned_value(&doc, heights_section, name);
		if (line == NULL)
			return refuse_row(error, row, "missing");
		if (!read_row(line, row, result.cells,
			      result.heights + (size_t)row * (result.cells + 1),
			      error))
			return false;
	}
	if (result.layer_count > 0) {
		result.paint = voe_assets_landscape_paint_read(
			&doc, result.cells, result.layer_count, arena, error);
		if (result.paint == NULL)
			return false;
	}
	if (!voe_assets_landscape_foliage_read(&doc, text, size, arena, &result,
					       error))
		return false;

	*out = result;
	return true;
}

void voe_assets_landscape_print(struct voe_assets_landscape_printer *printer,
				const char *format, ...)
{
	char *at = printer->buffer ? printer->buffer + printer->used : NULL;
	size_t room = printer->buffer ? printer->capacity - printer->used : 0;
	va_list arguments;
	int written;

	va_start(arguments, format);
	written = vsnprintf(at, room, format, arguments);
	va_end(arguments);
	VOE_BASE_ASSERT(written >= 0, "printing a number failed");
	VOE_BASE_ASSERT(printer->buffer == NULL || (size_t)written < room,
			"the landscape text was measured by this same pass");
	printer->used += (size_t)written;
}

void voe_assets_landscape_quote(const char *path, char *quoted)
{
	size_t at = 0;

	VOE_BASE_ASSERT(memchr(path, '\0', VOE_ASSETS_MATERIAL_PATH) != NULL,
			"a layer or kind path not NUL-terminated in its room");
	VOE_BASE_ASSERT(strpbrk(path, "\r\n") == NULL,
			"a layer or kind path with a line break");
	quoted[at++] = '"';
	for (size_t i = 0; path[i] != '\0'; i++) {
		if (path[i] == '"' || path[i] == '\\')
			quoted[at++] = '\\';
		quoted[at++] = path[i];
	}
	quoted[at++] = '"';
	quoted[at] = '\0';
	VOE_BASE_ASSERT(at < VOE_ASSETS_LANDSCAPE_QUOTED_PATH,
			"a quoted path outgrew its room");
}

static void print_landscape(struct voe_assets_landscape_printer *printer,
			    const voe_assets_landscape *landscape)
{
	const uint32_t cells = landscape->cells;

	voe_assets_landscape_print(printer,
				   "[Landscape]\nsize=%.9g\ncells=%u\n\n",
				   (double)landscape->size, cells);
	if (landscape->layer_count > 0)
		voe_assets_landscape_print(printer, "[Layers]\n");
	for (uint32_t k = 0; k < landscape->layer_count; k++) {
		char quoted[VOE_ASSETS_LANDSCAPE_QUOTED_PATH];

		voe_assets_landscape_quote(landscape->layers[k], quoted);
		voe_assets_landscape_print(printer, "layer%u=%s\n", k, quoted);
	}
	if (landscape->layer_count > 0)
		voe_assets_landscape_print(printer, "\n");
	voe_assets_landscape_kinds_print(printer, landscape);
	voe_assets_landscape_print(printer, "[Heights]\n");
	for (uint32_t row = 0; row <= cells; row++) {
		const float *heights =
			landscape->heights + (size_t)row * (cells + 1);

		voe_assets_landscape_print(printer, "row%u=", row);
		for (uint32_t c = 0; c <= cells; c++)
			// Adding 0.0 turns a rounded -0 into 0.
			voe_assets_landscape_print(
				printer, c < cells ? "%.0f " : "%.0f\n",
				round((double)heights[c] * 1000.0) + 0.0);
	}
	voe_assets_landscape_paint_print(printer, landscape);
	voe_assets_landscape_copies_print(printer, landscape);
}

voe_assets_landscape_text
voe_assets_landscape_write(const voe_assets_landscape *landscape,
			   voe_base_arena *arena)
{
	struct voe_assets_landscape_printer printer = { 0 };

	VOE_BASE_ASSERT(landscape != NULL && landscape->heights != NULL,
			"writing no landscape");
	VOE_BASE_ASSERT(size_supported((double)landscape->size) &&
				cells_supported(landscape->cells),
			"writing a landscape the reader would refuse");
	VOE_BASE_ASSERT(landscape->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"writing more layers than a landscape holds");
	VOE_BASE_ASSERT(arena != NULL, "writing a landscape with no arena");

	print_landscape(&printer, landscape);
	printer.capacity = printer.used + 1;
	printer.buffer = voe_base_arena_push(arena, printer.capacity);
	printer.used = 0;
	print_landscape(&printer, landscape);

	return (voe_assets_landscape_text){ .text = printer.buffer,
					    .size = printer.used };
}
