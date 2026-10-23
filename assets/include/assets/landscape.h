// A landscape file (`<name>.landscape`, ADR 0379 point 1, 0402 points 1-2, 0442
// point 1, 0476 points 1-2): a square grid of heights, up to eight painted
// material layers and the foliage on it, in sectioned text, read and written.
//
//     [Landscape]
//     size=256
//     cells=4
//
//     [Layers]
//     layer0="Assets/grass.material"
//     layer1="Assets/rock.material"
//     grow1_0=40
//
//     [Foliage]
//     kind0="Assets/pine.glb"
//     size0=2000
//     vary0=30
//     turn0=90
//     tilt0=50
//     shadow0=0
//
//     [Heights]
//     row0=0 0 0 0 0
//     ...
//
//     [Paint]
//     paint1_0=0 120 255 120 0
//
//     [Copies]
//     c0=0 -1500 32000 16384 1000 0
//
// `size` is metres a side and `cells` the cells per side: cells + 1 rows of
// cells + 1 heights, row r at z = -size/2 + r*size/cells and value c at x =
// -size/2 + c*size/cells about the origin, a height metres up from it. Every
// grid is row-major. ONE FILE, NO PICTURE BESIDE IT (0379 point 1): diffable,
// and the editor's file operations follow it; 512 cells is 2-3 MB. AT MOST 2048
// CELLS (0396 point 1): 2049² texels fit Vulkan's 4096 a side. HEIGHTS ARE
// WHOLE MILLIMETRES, so a value read back is the value written.
//
// LAYERS ARE QUOTED .material PATHS from layer0= to the first missing key,
// quoted so the follow (0378) rewrites them; layer 0 is the ground. PAINT IS A
// WEIGHT 0-255 PER LAYER AT EVERY POINT, summing to 255: `paint<k>_<r>` is
// layer k's row r from k = 1, LAYER 0 NEVER WRITTEN but 255 minus the rest, so
// a sum can be too high but never short, and `v*n` is n copies of v. No
// [Paint] and NULL paint are all layer 0, so a struct built by hand is valid.
// A RESAMPLE IS BILINEAR, layers 1-7 rounded and layer 0 given the rest.
//
// KINDS ARE QUOTED .glb PATHS, read as layers are. A KIND'S LOOK is `size<k>`
// (thousandths), `vary<k>` (size ±%), `turn<k>` (±°), `tilt<k>` (% with slope)
// and `shadow<k>=0` (casting none): a missing key is the default, and only a key
// off the default is written. LAYER l GROWS KIND k at `grow<l>_<k>` tenths of
// copies per 100 m², written only above 0. THE CHOICE IS THE LAYER ROW'S, so a
// row given another material keeps it (0476 point 2). COPY i IS `c<i>=kind x z
// turn scale lean` and has no height: it stands on the ground a sculpt moves.
// [Copies] IS WALKED ONCE IN ORDER: 65536 lookups would be 2·10⁹ compares.
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
// The most foliage kinds and copies a landscape holds, and a kind path's room.
#define VOE_ASSETS_LANDSCAPE_KINDS 16
#define VOE_ASSETS_LANDSCAPE_COPIES 65536
#define VOE_ASSETS_LANDSCAPE_KIND_PATH 128
// A kind's base size in thousandths: the file's range and the editor's slider's.
#define VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN 100
#define VOE_ASSETS_LANDSCAPE_KIND_SIZE_MAX 10000
// A kind's three spreads, most and default: size ±%, turn ±°, tilt % with slope.
#define VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX 100
#define VOE_ASSETS_LANDSCAPE_TURN_VARY_MAX 180
#define VOE_ASSETS_LANDSCAPE_TILT_MAX 100
#define VOE_ASSETS_LANDSCAPE_SIZE_VARY_DEFAULT 20
#define VOE_ASSETS_LANDSCAPE_TURN_VARY_DEFAULT 180
#define VOE_ASSETS_LANDSCAPE_TILT_DEFAULT 0
// The most tenths of copies per 100 m² a layer grows of a kind: 50 copies.
#define VOE_ASSETS_LANDSCAPE_GROW_MAX 500

// How a kind's copies look (0476 point 1). Compare field by field: padding.
typedef struct {
	// Base size in thousandths, KIND_SIZE_MIN to KIND_SIZE_MAX, 0 meaning 1×;
	// read as a factor only through voe_assets_landscape_kind_size.
	uint16_t size;
	// Size ±%, 0 to SIZE_VARY_MAX.
	uint8_t size_vary;
	// Turn ±degrees, 0 to TURN_VARY_MAX.
	uint8_t turn_vary;
	// How far a copy leans with the slope, %, 0 to TILT_MAX.
	uint8_t tilt;
	// Casts no shadow.
	bool shadowless;
} voe_assets_landscape_look;

// One foliage copy, in the order a cooked `{x, z, turn, scale, kind, lean}`
// fills. Compare field by field, never with memcmp: the struct has padding.
typedef struct {
	// Millimetres in the grid's own space, as the heights' x and z.
	int32_t x, z;
	// 1/65536 of a turn about +Y.
	uint16_t turn;
	// Thousandths, 1 to 65535.
	uint16_t scale;
	// Below the landscape's kind_count.
	uint8_t kind;
	// 0 stands straight up, 255 along the ground's normal.
	uint8_t lean;
} voe_assets_landscape_copy;

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
	// Project-relative `.glb` paths, NUL-terminated; the first `kind_count`
	// are the landscape's foliage kinds.
	char kinds[VOE_ASSETS_LANDSCAPE_KINDS][VOE_ASSETS_LANDSCAPE_KIND_PATH];
	uint32_t kind_count;
	// Each kind's look.
	voe_assets_landscape_look looks[VOE_ASSETS_LANDSCAPE_KINDS];
	// grows[l][k] is layer l's density of kind k in tenths of copies per
	// 100 m², 1 to GROW_MAX, 0 not grown. The layer row's, not its material's.
	uint16_t grows[VOE_ASSETS_LANDSCAPE_LAYERS][VOE_ASSETS_LANDSCAPE_KINDS];
	// `copy_count` copies, NULL with none.
	voe_assets_landscape_copy *copies;
	uint32_t copy_count;
} voe_assets_landscape;

typedef struct {
	const char *text;
	size_t size;
} voe_assets_landscape_text;

// Every height 0, no layers, paint NULL, no kinds, every look zeroed, every
// grow 0, copies NULL. Asserts size in range and cells a multiple of 4 from 4
// to VOE_ASSETS_LANDSCAPE_CELLS_MAX.
voe_assets_landscape voe_assets_landscape_flat(float size, uint32_t cells,
					       voe_base_arena *arena);

// The look a missing key reads as: size 0 (1×), size ±20 %, turn ±180°, tilt
// 0 %, casting shadows.
voe_assets_landscape_look voe_assets_landscape_look_default(void);

// Kind `kind`'s base size as a factor on its copies' scale: looks[kind].size
// in thousandths, 1 for 0. Asserts kind below VOE_ASSETS_LANDSCAPE_KINDS.
float voe_assets_landscape_kind_size(const voe_assets_landscape *landscape,
				     uint32_t kind);

// `from` at the same size and `cells` cells a side, each height and each
// layer's weight bilinear from `from`'s at the same point of the square, layers
// 1-7 rounded and layer 0 given the rest; the same count is an exact copy. The
// layers and grows are carried, and paint NULL stays NULL. Kinds, looks and copies
// are carried unchanged, the copies copied onto `arena`. Asserts cells a
// multiple of 4 from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX.
voe_assets_landscape
voe_assets_landscape_resample(const voe_assets_landscape *from, uint32_t cells,
			      voe_base_arena *arena);

// Reads `size` bytes of text, which need not be NUL-terminated. MALFORMED for
// text the sectioned reader refuses, a missing key or row, a row with too few
// or too many numbers or one not a whole number, a layer path not quoted or of
// VOE_ASSETS_MATERIAL_PATH bytes or more, a weight past 255, or a point whose
// layers 1-7 pass 255. MALFORMED too for a kind path not quoted or of
// VOE_ASSETS_LANDSCAPE_KIND_PATH bytes or more, a kind's size not a whole
// number from KIND_SIZE_MIN to KIND_SIZE_MAX, a spread not a whole number up to
// its maximum, a `shadow<k>` other than 0 or 1, a `grow` key not `grow<l>_<k>`
// with l below layer_count and k below kind_count or its density not 1 to
// GROW_MAX, and a copy key out of order or misnamed, of other than six whole
// numbers, with a kind not below kind_count, or a turn, scale or lean out of
// range or a scale of 0. UNSUPPORTED for a size out of range, cells not a
// multiple of 4 from 4 to VOE_ASSETS_LANDSCAPE_CELLS_MAX, or past
// VOE_ASSETS_LANDSCAPE_COPIES copies. Paint is allocated when there are layers
// and NULL with none; no [Copies] is no copies; a kind's missing look key is
// the default's, a missing grow 0. A line on stderr names what. `out`
// untouched on failure; `error` may be NULL.
[[nodiscard]] bool voe_assets_landscape_read(const char *text, size_t size,
					     voe_base_arena *arena,
					     voe_assets_landscape *out,
					     voe_base_error *error);

// The text voe_assets_landscape_read takes, NUL-terminated, `size` not
// counting the NUL. [Layers] only when there are any, each grow of a kind
// below kind_count after the paths when above 0; [Paint] only when paint is
// not NULL and there are two layers or more, runs of 3 or more as `v*n`;
// [Foliage] only when there are kinds, a look key only off the default (size
// neither 0 nor 1000, `shadow<k>=0` only when shadowless), and [Copies] only
// when there are copies, one line a copy. Cannot fail; asserts each layer and
// kind path is NUL-terminated with no line break, each look and grow in range.
voe_assets_landscape_text
voe_assets_landscape_write(const voe_assets_landscape *landscape,
			   voe_base_arena *arena);
