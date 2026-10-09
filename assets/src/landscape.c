// The landscape file behind assets/landscape.h: the sectioned reader for the
// lines, then each value turned into a number here, and a printer for the way
// back.
//
// A ROW IS FOUND BY NAME, one linear scan of its section's keys per row, so a
// 2048-cell file is about two million string compares. That is tens of
// milliseconds; an index from row number to key would lift it.
//
// THE WRITER MEASURES, THEN PRINTS. A height is any float, so a millimetre
// count can be forty digits long; one pass with no buffer counts the bytes, one
// push holds exactly them, and the same pass prints into it.
#include <assets/landscape.h>
#include <assets/sectioned.h>

#include <base/assert.h>
#include <base/report.h>

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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

// Where new index `i` of `cells` falls on a grid of `from_cells`: the index
// below and how far toward the next. Integer division keeps it exact, so the
// same count lands on every old point with no fraction and an edge on an edge.
static uint32_t source_index(uint32_t i, uint32_t cells, uint32_t from_cells,
			     float *fraction)
{
	const uint64_t scaled = (uint64_t)i * from_cells;

	*fraction = (float)(scaled % cells) / (float)cells;
	return (uint32_t)(scaled / cells);
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
		const uint32_t z0 = source_index(row, cells, from_cells, &fz);
		const uint32_t z1 = z0 < from_cells ? z0 + 1 : z0;
		const float *near = from->heights + (size_t)z0 * (from_cells + 1);
		const float *far = from->heights + (size_t)z1 * (from_cells + 1);

		for (uint32_t c = 0; c <= cells; c++) {
			float fx;
			const uint32_t x0 = source_index(c, cells, from_cells, &fx);
			const uint32_t x1 = x0 < from_cells ? x0 + 1 : x0;
			const float a = near[x0] + (near[x1] - near[x0]) * fx;
			const float b = far[x0] + (far[x1] - far[x0]) * fx;

			to.heights[(size_t)row * (cells + 1) + c] =
				a + (b - a) * fz;
		}
	}
	return to;
}

bool voe_assets_landscape_read(const char *text, size_t size,
			       voe_base_arena *arena,
			       voe_assets_landscape *out,
			       voe_base_error *error)
{
	voe_assets_sectioned doc;
	uint32_t heights_section;
	double metres;
	uint32_t cells;
	float *heights;

	VOE_BASE_ASSERT(text != NULL, "reading a landscape from nothing");
	VOE_BASE_ASSERT(arena != NULL, "reading a landscape with no arena");
	VOE_BASE_ASSERT(out != NULL, "reading a landscape into nothing");

	if (!voe_assets_sectioned_parse(text, size, arena, &doc))
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "not sectioned text");
	if (!read_shape(&doc, &metres, &cells, error))
		return false;
	heights_section = voe_assets_sectioned_find(&doc, "Heights");
	if (heights_section == VOE_ASSETS_SECTIONED_NONE)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "no [Heights] section");

	heights = voe_base_arena_push(arena,
				      height_count(cells) * sizeof(float));
	for (uint32_t row = 0; row <= cells; row++) {
		char name[16];
		const char *line;

		snprintf(name, sizeof(name), "row%u", row);
		line = voe_assets_sectioned_value(&doc, heights_section, name);
		if (line == NULL)
			return refuse_row(error, row, "missing");
		if (!read_row(line, row, cells,
			      heights + (size_t)row * (cells + 1), error))
			return false;
	}

	*out = (voe_assets_landscape){
		.size = (float)metres,
		.cells = cells,
		.heights = heights,
	};
	return true;
}

// Where the printer is. With no buffer it only counts.
struct printer {
	char *buffer;
	size_t capacity;
	size_t used;
};

[[gnu::format(printf, 2, 3)]] static void print(struct printer *printer,
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

static void print_landscape(struct printer *printer,
			    const voe_assets_landscape *landscape)
{
	const uint32_t cells = landscape->cells;

	print(printer, "[Landscape]\nsize=%.9g\ncells=%u\n\n[Heights]\n",
	      (double)landscape->size, cells);
	for (uint32_t row = 0; row <= cells; row++) {
		const float *heights =
			landscape->heights + (size_t)row * (cells + 1);

		print(printer, "row%u=", row);
		for (uint32_t c = 0; c <= cells; c++)
			// Adding 0.0 turns a rounded -0 into 0.
			print(printer, c < cells ? "%.0f " : "%.0f\n",
			      round((double)heights[c] * 1000.0) + 0.0);
	}
}

voe_assets_landscape_text
voe_assets_landscape_write(const voe_assets_landscape *landscape,
			   voe_base_arena *arena)
{
	struct printer printer = { 0 };

	VOE_BASE_ASSERT(landscape != NULL && landscape->heights != NULL,
			"writing no landscape");
	VOE_BASE_ASSERT(size_supported((double)landscape->size) &&
				cells_supported(landscape->cells),
			"writing a landscape the reader would refuse");
	VOE_BASE_ASSERT(arena != NULL, "writing a landscape with no arena");

	print_landscape(&printer, landscape);
	printer.capacity = printer.used + 1;
	printer.buffer = voe_base_arena_push(arena, printer.capacity);
	printer.used = 0;
	print_landscape(&printer, landscape);

	return (voe_assets_landscape_text){ .text = printer.buffer,
					    .size = printer.used };
}
