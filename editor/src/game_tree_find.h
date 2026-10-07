// The files the game tree cooks, found under a project's Assets/: every file
// whose name ends in a given suffix (`.prefab`, `.landscape`), in any case,
// hidden entries skipped, as paths under Assets/ with `/`, in byte order of
// path so a cooked file's bytes do not change with the listing's order.
//
// AN EXPLICIT STACK OF LISTINGS, one per folder level, and no recursion: a
// folder deeper than VOE_EDITOR_GAME_TREE_DEPTH (a symlink loop included) is
// refused with a notice rather than followed. A larger constant lifts it.
//
// Every path and listing is pushed into the caller's arena and is the
// caller's to rewind.
#pragma once

#include "notice.h"

#include <base/arena.h>

#include <stdbool.h>
#include <stdint.h>

// How many folder levels under Assets/ are looked in, Assets/ one.
#define VOE_EDITOR_GAME_TREE_DEPTH 16

// One path found, under Assets/ with `/`.
struct voe_editor_game_tree_path {
	const char *path;
};

// The paths found, in arena, room doubling as they grow.
struct voe_editor_game_tree_found {
	struct voe_editor_game_tree_path *items;
	uint32_t count;
	uint32_t room;
};

// Whether path is a folder, looked for in its parent's listing, because
// listing a path that is not there reports an error on stderr. Keeps nothing
// in arena.
bool voe_editor_game_tree_folder_there(const char *path, voe_base_arena *arena);

// Every file under assets ending in suffix, sorted by path; none when assets
// is not a folder. False with why on a folder that will not list or one
// deeper than VOE_EDITOR_GAME_TREE_DEPTH.
[[nodiscard]] bool voe_editor_game_tree_find(const char *assets, const char *suffix,
					     voe_base_arena *arena,
					     struct voe_editor_game_tree_found *out,
					     voe_editor_notice *why);
