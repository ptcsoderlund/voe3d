// The editor's model store's struct, private to the two files that carry out
// models.h: models.c, which owns its life, update, reload, frame, save, revert,
// takes, materials, species and its map queue, and models_edit.c, which edits its
// landscapes and their foliage. Nothing else includes it; every other caller
// holds the opaque handle models.h gives, and the fields are written in those
// two files alone. A landscape taken in waits as a copy in an arena of its
// own, at most LANDSCAPE_TAKES at once; a bigger count lifts it.
#pragma once

#include "map_watch.h"
#include "models.h"

#include <base/arena.h>

#include <game/models.h>
#include <game/trees.h>

#include <stdint.h>

// How many landscapes may wait to be taken in at once, and the block size of
// the arena each waiting copy lives in (a default, not a limit).
#define LANDSCAPE_TAKES 8
#define LANDSCAPE_TAKE_ARENA (1u * 1024u * 1024u)

// A landscape waiting to be loaded over its entry, copied with its path into an
// arena of its own; `edited` marks the entry edited once loaded.
struct landscape_take {
	voe_base_arena *arena;
	char *path;
	voe_assets_landscape *landscape;
	bool edited;
};

struct voe_editor_models {
	voe_3d_models *store;
	// The maps queued outside a wait, compressed on the GPU a step a frame;
	// emptied on a different folder.
	voe_game_compressing *compressing;
	// The project's materials and parts, read on a different folder.
	voe_editor_materials *materials;
	voe_editor_parts *parts;
	// The materials as the read before the last, to find what it changed;
	// swapped with `materials` at each read, since a table is never copied.
	voe_editor_materials *before;
	// The project's species and the read before the last, swapped as the
	// materials are, so a row's mark outlives a read.
	voe_editor_trees *trees;
	voe_editor_trees *trees_before;
	// The one species grown between frames, and its path, "" while idle.
	voe_game_trees_job *job;
	char growing[VOE_ASSETS_LANDSCAPE_KIND_PATH];
	// The folder the store was last read against, the store's own copy;
	// NULL before any and while the project is untitled.
	char *folder;
	// The frame clock's reading at the last watch.
	double looked;
	// The materials' maps' stamps at the last watch; cleared on a
	// different folder.
	voe_editor_map_watch maps;
	// A landscape whose size was written, to load again at the next
	// update, the store's own copy; NULL for none.
	char *resized;
	// The materials to load again at the next update, `reloads` of them.
	char reload[VOE_EDITOR_MATERIALS][VOE_ASSETS_MATERIAL_PATH];
	uint32_t reloads;
	// The landscapes taken in to load at the next update, `takes` of them.
	struct landscape_take take[LANDSCAPE_TAKES];
	uint32_t takes;
};
