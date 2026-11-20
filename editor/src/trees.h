// The project's tree species as game/trees.h's table (0515 point 5, the
// editor's side): every `.tree` under `Assets/` read leniently, each row named
// by its `Assets/...` path, handed to game/trees.h's grow and job as the game
// hands its cooked table.
//
//     voe_editor_trees_read(trees, folder, scratch);
//     voe_game_trees table = voe_editor_trees_table(trees);
//     voe_game_tree *row = voe_editor_trees_find(trees, path);
//
// THE TABLE IS WHAT THE COOK WRITES: the game tree's trees are this table in
// C, so the game and the editor grow the same species. It is read again on a
// project and after each Assets command, and edited in place through _find,
// the store regrowing what changed.
//
// `regrow` IS THE STORE'S MARK (models.h): a read leaves every row unmarked;
// the store marks a row read anew or changed and clears it when it starts
// growing it.
//
// A FILE THAT WILL NOT READ is left out with one stderr line, and a kind
// naming it draws as nothing; one bad file does not cost the rest. Its values
// are clamped and defaulted, so a row's variations are always 1 to 16.
//
// Constraints: at most VOE_EDITOR_TREES rows, the rest left out and said once;
// a larger constant lifts it, though the store's VOE_3D_MODELS entries fill
// first. A path of VOE_ASSETS_LANDSCAPE_KIND_PATH bytes or more is left out,
// the room a foliage kind gives its path. Each row's path points into the
// struct's own bytes, so the struct is never copied; it is held by pointer.
// _find is a scan.
#pragma once

#include <assets/landscape.h>
#include <assets/tree.h>

#include <base/arena.h>

#include <game/trees.h>

#include <stdbool.h>
#include <stdint.h>

// How many species a project's table holds.
#define VOE_EDITOR_TREES 32

// The table's rows, the bytes of their paths and the store's marks; `count`
// rows in use.
typedef struct {
	voe_game_tree rows[VOE_EDITOR_TREES];
	char paths[VOE_EDITOR_TREES][VOE_ASSETS_LANDSCAPE_KIND_PATH];
	bool regrow[VOE_EDITOR_TREES];
	uint32_t count;
} voe_editor_trees;

// Empties `trees`, then reads every `.tree` under `<folder>/Assets/` in byte
// order of path, each row unmarked. NULL `folder` leaves it empty. `scratch`
// is rewound.
void voe_editor_trees_read(voe_editor_trees *trees, const char *folder,
			   voe_base_arena *scratch);

// The rows as the table game/trees.h grows, pointing into `trees`.
voe_game_trees voe_editor_trees_table(const voe_editor_trees *trees);

// `path`'s row, to edit in place; NULL when none.
voe_game_tree *voe_editor_trees_find(voe_editor_trees *trees, const char *path);
