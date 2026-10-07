// A landscape's heights cooked into C source: one array a game tree compiles in
// (ADR-0379 point 7).
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
