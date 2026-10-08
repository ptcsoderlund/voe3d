// A landscape file (`<name>.landscape`, ADR 0379 point 1): a square grid of
// heights in sectioned text, read into floats and written back.
//
//     [Landscape]
//     size=256
//     cells=4
//
//     [Heights]
//     row0=0 0 0 0 0
//     row1=0 120 250 120 0
//     ...
//     row4=0 0 0 0 0
//
// `size` is metres a side and `cells` the cells per side, so there are
// cells + 1 rows of cells + 1 heights. Row r lies at z = -size/2 + r*size/cells
// and value c at x = -size/2 + c*size/cells, about the thing's origin; a height
// is metres up from it. The grid is row-major, row r starting at r*(cells + 1).
//
// ONE FILE AND NO PICTURE BESIDE IT (0379 point 1). A 16-bit height picture
// would need a codec and a sibling that a rename or a move could leave behind;
// text keeps the asset one file the editor's file operations already follow,
// and diffable. At 512 cells it is 2-3 MB, read in milliseconds.
//
// HEIGHTS ARE WHOLE MILLIMETRES, so the text is exact and short: no float is
// printed, a value read back is the value written, and a flat row is mostly
// "0 ". Writing rounds each height to the nearest millimetre.
//
// THE GAME NEVER READS THIS (0236): the editor does, and the cook turns it into
// a table the game loads. Nothing here opens a file; `platform` owns files.
//
// THE ARENA HOLDS THE RESULT AND THE WORKING MEMORY. The heights, the text and
// the sectioned reader's copy of the file all go on the arena handed in, and
// are freed by rewinding or destroying it.
#pragma once

#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// What Create writes, and the most a file may hold.
#define VOE_ASSETS_LANDSCAPE_CELLS 512
#define VOE_ASSETS_LANDSCAPE_SIZE_MIN 16.0f
#define VOE_ASSETS_LANDSCAPE_SIZE_MAX 8192.0f
#define VOE_ASSETS_LANDSCAPE_SIZE_DEFAULT 256.0f

typedef struct {
	// Metres a side.
	float size;
	// Cells per side, a multiple of 4 from 4 to VOE_ASSETS_LANDSCAPE_CELLS.
	uint32_t cells;
	// (cells + 1)² metres, row-major.
	float *heights;
} voe_assets_landscape;

typedef struct {
	const char *text;
	size_t size;
} voe_assets_landscape_text;

// Every height 0. Asserts size in range and cells a multiple of 4 from 4 to
// VOE_ASSETS_LANDSCAPE_CELLS.
voe_assets_landscape voe_assets_landscape_flat(float size, uint32_t cells,
					       voe_base_arena *arena);

// Reads `size` bytes of text, which need not be NUL-terminated. MALFORMED for
// text the sectioned reader refuses, a missing key or row, a row with too few
// or too many numbers or one not a whole number; UNSUPPORTED for a size out of
// range or cells not a multiple of 4 from 4 to 512. A line on stderr names
// what. `out` untouched on failure; `error` may be NULL.
[[nodiscard]] bool voe_assets_landscape_read(const char *text, size_t size,
					     voe_base_arena *arena,
					     voe_assets_landscape *out,
					     voe_base_error *error);

// The text voe_assets_landscape_read takes, NUL-terminated, `size` not
// counting the NUL. Cannot fail.
voe_assets_landscape_text
voe_assets_landscape_write(const voe_assets_landscape *landscape,
			   voe_base_arena *arena);
