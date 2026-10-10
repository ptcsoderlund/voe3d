// The materials a game loads into its model store (0399 point 7): each one's
// path, as a shape or a model row names it, and the values its file holds.
//
//     const voe_game_materials voe_game_materials_cooked = {
//             (const voe_game_material[]){ { "Assets/brick.material",
//                     { .colour = { 1, 1, 1 }, .roughness = 0.5f,
//                       .repeat = 1.0f, .colormap = "Assets/brick.png" } } },
//             1 };
//
// DEFINED BY THE COOKED materials.c IN A PROJECT'S GAME TREE, NOT BY THIS
// FOLDER, as game/landscapes.h's table is by the cooked landscapes.c. Only
// game/src/run.c names it; game/models.h takes a table as data.
//
// COOKED AND NOT READ (0236): a game parses no project text, so the editor
// turns every `.material` under Assets/ into this table. The editor builds its
// own table from the files and hands it to the same loader.
//
// Constraints: the maps are project-relative paths, read from the folder the
// loader is handed, as a model file is; "" for none.
#pragma once

#include <3d/models.h>

#include <stdint.h>

// One material: its path and its file's values.
typedef struct {
	// Project-relative, `/`-separated, as a row names it.
	const char *path;
	voe_assets_material_file values;
} voe_game_material;

// The table a game loads.
typedef struct {
	const voe_game_material *materials;
	uint32_t count;
} voe_game_materials;

// Defined by the game tree's cooked materials.c.
extern const voe_game_materials voe_game_materials_cooked;
