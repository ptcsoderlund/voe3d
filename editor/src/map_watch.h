// The maps the project's materials name, watched for a re-export (0477 point
// 5): each map's stamp at the last look, so a map written over itself is seen
// and the materials naming it load again. models.c keeps one and looks once a
// second, after the models' watch:
//
//     voe_editor_map_changes changed;
//     uint32_t count = voe_editor_map_watch_look(&watch, folder, table,
//                                                &changed, scratch);
//     voe_editor_map_watch_clear(&watch);   // on a different folder
//
// THE EDITOR'S AND NOT `game`'S: the game trusts its cooked copies, made
// before it runs, and no map changes under it; only the editor has a person
// re-exporting a picture while it is open.
//
// THE COST is one stamp a map a second, a map named by several rows stamped
// once, as the models' files are (0277 point 5); nothing is read.
//
// A PATH SEEN FOR THE FIRST TIME is noted and never reported: its material
// loads it fresh anyway. A path no row names any more is let go. A map that is
// not there has stamp 0, so one that appears or goes is a change.
//
// Constraints: at most VOE_EDITOR_MAP_WATCH paths, four maps a material, which
// every table fits. A changed map past VOE_EDITOR_MAP_CHANGES keeps its old
// stamp and is reported at a later look. Each path is found by a scan of the table, at most
// a million compares a second at the largest; a hash would lift it.
#pragma once

#include "materials.h"

#include <assets/material.h>

#include <base/arena.h>

#include <game/materials.h>

#include <stdbool.h>
#include <stdint.h>

// How many map paths a watch holds: four for each material.
#define VOE_EDITOR_MAP_WATCH (VOE_EDITOR_MATERIALS * 4)

// Each watched path, `count` in use, with its stamp at the last look; `seen`
// is the look's own mark of the paths a row still names.
typedef struct {
	char paths[VOE_EDITOR_MAP_WATCH][VOE_ASSETS_MATERIAL_PATH];
	uint64_t stamps[VOE_EDITOR_MAP_WATCH];
	bool seen[VOE_EDITOR_MAP_WATCH];
	uint32_t count;
} voe_editor_map_watch;

// How many changed maps one look reports: the queue's room (0477 point 4).
#define VOE_EDITOR_MAP_CHANGES 16

// The maps one look found changed, `count` of them, each pointing into the
// table's rows; held in a struct since a header takes one star.
typedef struct {
	const char *paths[VOE_EDITOR_MAP_CHANGES];
	uint32_t count;
} voe_editor_map_changes;

// Stamps every non-empty map path of `table`'s rows under `folder` and fills
// `changed` with those whose stamp differs from the last look, at most
// VOE_EDITOR_MAP_CHANGES; how many. `scratch` is rewound.
uint32_t voe_editor_map_watch_look(voe_editor_map_watch *watch,
				   const char *folder, voe_game_materials table,
				   voe_editor_map_changes *changed,
				   voe_base_arena *scratch);

// Forgets every path, so the next look notes them all as new.
void voe_editor_map_watch_clear(voe_editor_map_watch *watch);
