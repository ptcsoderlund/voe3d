// A landscape file (`<name>.landscape`, ADR 0379 point 1, 0402 points 1-2): a
// square grid of heights and up to eight painted material layers in sectioned
// text, read into memory and written back.
//
//     [Landscape]
//     size=256
//     cells=4
//
//     [Layers]
//     layer0="Assets/grass.material"
//     layer1="Assets/rock.material"
//
//     [Heights]
//     row0=0 0 0 0 0
//     ...
//     row4=0 0 0 0 0
//
//     [Paint]
//     paint1_0=0*5
//     paint1_1=0 120 255 120 0
//     ...
//
// `size` is metres a side and `cells` the cells per side, so there are
// cells + 1 rows of cells + 1 heights. Row r lies at z = -size/2 + r*size/cells
// and value c at x = -size/2 + c*size/cells, about the thing's origin; a height
// is metres up from it. Every grid is row-major, row r starting at r*(cells + 1).
//
// ONE FILE AND NO PICTURE BESIDE IT (0379 point 1): text stays one file the
// editor's file operations follow, and diffable. 512 cells is 2-3 MB. AT MOST
// 2048 CELLS A SIDE (0396 point 1): 2049² texels fit Vulkan's 4096 a side.
//
// LAYERS ARE QUOTED .material PATHS from layer0=, read to the first missing
// key, quoted so the follow (0378) rewrites them. Layer 0 is the ground.
// PAINT IS ONE WEIGHT 0-255 PER LAYER AT EVERY HEIGHT POINT, summing to 255.
// `paint<k>_<r>` is layer k's row r for k from 1. LAYER 0 IS NEVER WRITTEN: it
// is 255 minus the rest, so a sum can be written too high but never short. A
// token `v*n` is n copies of v, so an unpainted row is `0*513`. No [Paint]
// reads as all layer 0. NULL PAINT IS ALL LAYER 0 too, so a struct built by
// hand (the game, 3d's tests) stays valid and no layers holds no paint.
//
// RESAMPLING IS BILINEAR. Each new height and layer weight is the old grid's at
// the same point, so corners and edges keep theirs and a plane stays a plane;
// layers 1-7 round and layer 0 takes the rest.
// HEIGHTS ARE WHOLE MILLIMETRES, so the text is exact and short and a value
// read back is the value written; writing rounds to the nearest millimetre.
//
// THE GAME NEVER READS THIS (0236): the editor does, and the cook turns it into
// a table. Nothing here opens a file; `platform` owns files. The result and the
// working memory go on the arena handed in, freed by rewinding or destroying it.
#pragma once

#include <assets/material.h>
#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// What Create writes.
#define VOE_ASSETS_LANDSCAPE_CELLS 512
// The most a file may hold.
#define VOE_ASSETS_LANDSCAPE_CELLS_MAX 2048
#define VOE_ASSETS_LANDSCAPE_SIZE_MIN 16.0f
#define VOE_ASSETS_LANDSCAPE_SIZE_MAX 8192.0f
#define VOE_ASSETS_LANDSCAPE_SIZE_DEFAULT 256.0f
// The most material layers a landscape names, two RGBA8 planes' worth.
#define VOE_ASSETS_LANDSCAPE_LAYERS 8

typedef struct {
	// Metres a side.
	float size;
	// Cells per side, a multiple of 4 from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX.
	uint32_t cells;
	// (cells + 1)² metres, row-major.
	float *heights;
	// Project-relative `.material` paths, NUL-terminated; the first
	// `layer_count` are the landscape's.
	char layers[VOE_ASSETS_LANDSCAPE_LAYERS][VOE_ASSETS_MATERIAL_PATH];
	uint32_t layer_count;
	// Two RGBA8 planes of (cells + 1)² texels each, row-major as the heights:
	// plane 0 holds layers 0-3 in r g b a, plane 1 layers 4-7. A point's eight
	// bytes sum to 255. NULL means every point all layer 0.
	uint8_t *paint;
} voe_assets_landscape;

typedef struct {
	const char *text;
	size_t size;
} voe_assets_landscape_text;

// Every height 0, no layers, paint NULL. Asserts size in range and cells a
// multiple of 4 from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX.
voe_assets_landscape voe_assets_landscape_flat(float size, uint32_t cells,
					       voe_base_arena *arena);

// `from` at the same size and `cells` cells a side, each height and each
// layer's weight bilinear from `from`'s at the same point of the square, layers
// 1-7 rounded and layer 0 given the rest; the same count is an exact copy. The
// layers are carried, and paint NULL stays NULL. Asserts cells a multiple of 4
// from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX.
voe_assets_landscape
voe_assets_landscape_resample(const voe_assets_landscape *from, uint32_t cells,
			      voe_base_arena *arena);

// Reads `size` bytes of text, which need not be NUL-terminated. MALFORMED for
// text the sectioned reader refuses, a missing key or row, a row with too few
// or too many numbers or one not a whole number, a layer path not quoted or of
// VOE_ASSETS_MATERIAL_PATH bytes or more, a weight past 255, or a point whose
// layers 1-7 pass 255; UNSUPPORTED for a size out of range or cells not a
// multiple of 4 from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX. Paint is allocated
// when there are layers and NULL with none. A line on stderr names what. `out`
// untouched on failure; `error` may be NULL.
[[nodiscard]] bool voe_assets_landscape_read(const char *text, size_t size,
					     voe_base_arena *arena,
					     voe_assets_landscape *out,
					     voe_base_error *error);

// The text voe_assets_landscape_read takes, NUL-terminated, `size` not
// counting the NUL. [Layers] only when there are any; [Paint] only when paint
// is not NULL and there are two layers or more, runs of 3 or more as `v*n`.
// Cannot fail; asserts each layer path is NUL-terminated with no line break.
voe_assets_landscape_text
voe_assets_landscape_write(const voe_assets_landscape *landscape,
			   voe_base_arena *arena);
