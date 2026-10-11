// The landscape file's paint, internal to this folder: `landscape.c` reads the
// shape, layers and heights and hands `[Paint]` to `landscape_paint.c`, which
// prints with `landscape.c`'s printer and resamples with its source index.
//
// A PLANE'S TEXEL FOR LAYER k AT POINT p is byte (k / 4)·points·4 + p·4 + k % 4
// of the paint, points being (cells + 1)²; the paint is 8·points bytes.
#pragma once

#include <assets/landscape.h>
#include <assets/sectioned.h>

#include <stddef.h>
#include <stdint.h>

// Where the printer is. With no buffer it only counts.
struct voe_assets_landscape_printer {
	char *buffer;
	size_t capacity;
	size_t used;
};

// Prints into the printer, or counts with no buffer. In `landscape.c`.
[[gnu::format(printf, 2, 3)]] void
voe_assets_landscape_print(struct voe_assets_landscape_printer *printer,
			   const char *format, ...);

// Where new index `i` of `cells` falls on a grid of `from_cells`: the index
// below and how far toward the next. Integer division keeps it exact, so the
// same count lands on every old point with no fraction and an edge on an edge.
static inline uint32_t voe_assets_landscape_source_index(uint32_t i,
							 uint32_t cells,
							 uint32_t from_cells,
							 float *fraction)
{
	const uint64_t scaled = (uint64_t)i * from_cells;

	*fraction = (float)(scaled % cells) / (float)cells;
	return (uint32_t)(scaled / cells);
}

// The paint of a file with `layer_count` layers, at least one: `[Paint]`'s rows
// of layers 1 up, or all layer 0 with no `[Paint]`. NULL and a line on stderr
// for a row missing, of the wrong count or past 255, or a point past 255.
uint8_t *voe_assets_landscape_paint_read(const voe_assets_sectioned *doc,
					 uint32_t cells, uint32_t layer_count,
					 voe_base_arena *arena,
					 voe_base_error *error);

// `[Paint]` with a blank line before it, when paint is not NULL and there are
// two layers or more; nothing otherwise.
void voe_assets_landscape_paint_print(
	struct voe_assets_landscape_printer *printer,
	const voe_assets_landscape *landscape);

// `from`'s paint at `cells` a side, bilinear per weight, layers 1-7 rounded and
// layer 0 given the rest; NULL when `from`'s is NULL.
uint8_t *voe_assets_landscape_paint_resample(const voe_assets_landscape *from,
					     uint32_t cells,
					     voe_base_arena *arena);
