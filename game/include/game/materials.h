// The materials a game loads into its model store (0399 point 7): each one's
// path, as a shape or a model's part names it, and the values its file holds;
// and each model's parts (0420 point 1): its `.glb`'s path and the `.material`
// each of its glTF materials wears.
//
//     const voe_game_materials voe_game_materials_cooked = {
//             (const voe_game_material[]){ { "Assets/brick.material",
//                     { .colour = { 1, 1, 1 }, .roughness = 0.5f,
//                       .repeat = 1.0f, .colormap = "Assets/brick.png" } } },
//             1 };
//     const voe_game_parts voe_game_parts_cooked = {
//             (const voe_game_model_parts[]){ { "Assets/wall.glb",
//                     { .count = 1,
//                       .materials = { "Assets/brick.material" } } } },
//             1 };
//
// DEFINED BY THE COOKED materials.c IN A PROJECT'S GAME TREE, NOT BY THIS
// FOLDER, both tables, as game/landscapes.h's is by the cooked landscapes.c.
// Only game/src/run.c names them; game/models.h takes tables as data.
//
// COOKED AND NOT READ (0236): a game parses no project text, so the editor
// turns every `.material` under Assets/ into the first table and every
// `.parts` into the second. The editor builds its own tables from the files
// and hands them to the same loader.
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

// One model's parts: its `.glb`'s path and its `.parts` file's values.
typedef struct {
	// Project-relative, `/`-separated, as a model row names it.
	const char *path;
	voe_assets_parts parts;
} voe_game_model_parts;

// The parts table a game loads.
typedef struct {
	const voe_game_model_parts *models;
	uint32_t count;
} voe_game_parts;

// Defined by the game tree's cooked materials.c.
extern const voe_game_materials voe_game_materials_cooked;
extern const voe_game_parts voe_game_parts_cooked;
