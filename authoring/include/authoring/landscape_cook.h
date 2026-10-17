// A landscape's heights, paint and copies cooked into C source: arrays a game
// tree compiles in (ADR-0379 point 7, ADR-0402 point 8, ADR-0442 point 1).
//
//     static const int32_t hill[25] = {
//             0, 0, 0, 0, 0, 0, 120, 250,
//             ...
//     };
//
// C SOURCE, BECAUSE THE GAME PARSES NO PROJECT TEXT (ADR-0236). What is cooked
// is the heights alone, (cells + 1)² of them, row-major as assets/landscape.h
// lays them out; the size and cells go beside the array in the table that
// names it.
//
// WHOLE MILLIMETRES, ROUNDED AS THE FILE WRITES THEM, so Play loads exactly the
// heights the saved file holds and no float is printed.
//
// PAINT IS COOKED BESIDE THE HEIGHTS (ADR-0402 point 8): the RGBA8 planes as
// assets/landscape.h lays them out, plane 0 (layers 0-3) then plane 1 (4-7).
// Up to four layers plane 1 is all nought, so plane 0 alone is cooked; past
// four, both. WHOLE BYTES, so the game reads exactly the weights saved.
//
// A game table entry with no layers names no paint, NULL: the caller does not
// call the paint cook then. The layer paths are the table's, written by the
// game tree; this writes no path.
//
// COPIES ARE COOKED WHOLE (ADR-0442 point 1), one `{x, z, turn, scale, kind,
// lean},` line a copy in the landscape's order, so the game stands each where
// the editor did. The kind paths are the table's, written by the game tree,
// not here; a landscape with no copies names none and is not cooked.
//
// THE TABLE NAMING THE ARRAYS IS THE GAME TREE'S (editor): this knows no game
// type and writes no include. Nothing here opens a file.
#pragma once

#include <assets/landscape.h>
#include <authoring/scene_write.h>
#include <base/arena.h>

// `static const int32_t <name>[<(cells + 1)²>] = { ... };\n` pushed into
// `arena`, NUL-terminated, `size` not counting the NUL; the caller rewinds.
// Cannot fail. Asserts `name` is a C identifier and every height fits an
// int32_t in millimetres.
voe_authoring_text
voe_authoring_landscape_cook(const voe_assets_landscape *landscape,
			     const char *name, voe_base_arena *arena);

// `static const uint8_t <name>[<n>] = { ... };\n` pushed into `arena` as above,
// n being one plane's (cells + 1)² · 4 bytes when layer_count ≤ 4, both
// planes' when more. Cannot fail. Asserts `paint` is not NULL, layer_count is
// above nought and `name` is a C identifier.
voe_authoring_text
voe_authoring_landscape_cook_paint(const voe_assets_landscape *landscape,
				   const char *name, voe_base_arena *arena);

// `static const voe_assets_landscape_copy <name>[<n>] = {\n` then one
// `\t{x, z, turn, scale, kind, lean},\n` a copy, then `};\n`, pushed into
// `arena` as above. Cannot fail. Asserts copy_count is above nought and `name`
// is a C identifier.
voe_authoring_text
voe_authoring_landscape_cook_copies(const voe_assets_landscape *landscape,
				    const char *name, voe_base_arena *arena);
