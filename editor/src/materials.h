// The project's materials as game/materials.h's table (0399 point 7, the
// editor's side): every `.material` under `Assets/` read and parsed, each row
// named by its `Assets/...` path, handed to game/models.h's loader as the game
// hands its cooked table.
//
//     voe_editor_materials_read(materials, folder, scratch);
//     voe_game_materials table = voe_editor_materials_table(materials);
//     voe_game_material *open = voe_editor_materials_find(materials, path);
//
// THE TABLE IS WHAT THE COOK WRITES (card 37): the game tree's materials.c is
// this table in C, so the game and the editor load the same rows. It is read
// again on a project and after each Assets command (card 36), and edited in
// place by the Inspector through _find, the store reloading what changed.
//
// A FILE THAT WILL NOT READ OR PARSE is left out with one stderr line, and a
// row naming it draws as before (0399 point 6); one bad file does not cost
// the rest.
//
// Constraints: at most VOE_EDITOR_MATERIALS rows, the rest left out and said
// once; a larger constant lifts it. A path of VOE_ASSETS_MATERIAL_PATH bytes
// or more is left out, the room a shape's or model's row gives a path. Each
// row's path points into the struct's own bytes, so the struct is never
// copied; it is held by pointer. _find is a scan, as game/models.h's is.
#pragma once

#include <assets/material.h>

#include <base/arena.h>

#include <game/materials.h>

#include <stdint.h>

// How many materials a project's table holds.
#define VOE_EDITOR_MATERIALS 256

// The table's rows and the bytes of their paths; `count` rows in use.
typedef struct {
	voe_game_material rows[VOE_EDITOR_MATERIALS];
	char paths[VOE_EDITOR_MATERIALS][VOE_ASSETS_MATERIAL_PATH];
	uint32_t count;
} voe_editor_materials;

// Empties `materials`, then reads every `.material` under `<folder>/Assets/`
// in byte order of path. NULL `folder` leaves it empty. `scratch` is rewound.
void voe_editor_materials_read(voe_editor_materials *materials,
			       const char *folder, voe_base_arena *scratch);

// The rows as the table game/models.h loads, pointing into `materials`.
voe_game_materials
voe_editor_materials_table(const voe_editor_materials *materials);

// `path`'s row, to edit in place; NULL when none.
voe_game_material *voe_editor_materials_find(voe_editor_materials *materials,
					     const char *path);
