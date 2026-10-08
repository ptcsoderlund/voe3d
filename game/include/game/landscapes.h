// The cooked landscapes a game loads into its model store (0379 point 7):
// each one's path, as a model row names it, its size, its cells and its
// heights in whole millimetres.
//
//     static const int32_t hill[25] = { 0, 0, 0, 0, 0, ... };
//     const voe_game_landscapes voe_game_landscapes_cooked = {
//             (const voe_game_landscape[]){
//                     { "Terrain/Hill.landscape", 256.0f, 4, hill } }, 1 };
//
// DEFINED BY THE COOKED landscapes.c IN A PROJECT'S GAME TREE, NOT BY THIS
// FOLDER, as game/prefabs.h's table is by the cooked prefabs.c. Only
// game/src/run.c names it; game/models.h takes it as data.
//
// COOKED AND NOT READ (0236): a game parses no project text, so the editor
// turns every `.landscape` under Assets/ into this table as saved. Whole
// millimetres are what the file holds, so nothing is lost on the way.
//
// Constraints: `millimetres` holds (cells + 1)² heights, row-major, as
// assets/landscape.h lays them out; `cells` a multiple of 4 from 4 to 512.
#pragma once

#include <stdint.h>

// One cooked landscape.
typedef struct {
	// Project-relative, `/`-separated, as a model row names it.
	const char *path;
	// Metres a side.
	float size;
	// Cells per side.
	uint32_t cells;
	// (cells + 1)² heights in whole millimetres.
	const int32_t *millimetres;
} voe_game_landscape;

// The table a game loads.
typedef struct {
	const voe_game_landscape *landscapes;
	uint32_t count;
} voe_game_landscapes;

// Defined by the game tree's cooked landscapes.c.
extern const voe_game_landscapes voe_game_landscapes_cooked;
