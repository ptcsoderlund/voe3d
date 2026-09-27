// The model files a world names, read from a folder into a store (0277 points
// 4 and 5): the one loader the game and the editor share.
//
//     voe_game_models_failures failures = voe_game_models_update(
//             world, models, device, folder, scratch);
//     if (failures.count > 0)
//             ... // failures.first is the first path that would not load
//     voe_game_models_watch(models, device, folder, scratch);  // re-exports
//
// UPDATE reads every path a model row names that the store holds no entry for:
// `folder` joined with the row's path, its stamp taken, the file read and
// loaded. A file with no stamp, or one that will not read, is kept as a failed
// entry at stamp 0; one that will not parse is kept failed at its stamp. So a
// broken file is read once, not once a frame. Empty paths are skipped.
//
// WATCH asks every entry's file for its stamp again. A stamp that differs from
// the entry's is read and loaded again: a success replaces the parts, a failure
// keeps them (3d/models.h). A failed entry whose file now has a stamp differs
// from its 0, so it is tried again; a loaded file that has gone is kept, failed
// at 0, and tried again when it comes back.
//
// BOTH RETURN how many paths failed in the call and the first one's path, the
// store's own copy, valid until that entry is loaded again or the store is
// cleared; NULL when none failed. A struct, not a `const char **` (rule 6).
// Each failure is one line on stderr naming the path and the category.
//
// A STAMP AND NOT A WATCHER API (0277 point 5): modification time and size,
// asked once a second by the editor, is enough for a person exporting from
// Blender, and needs no per-platform notification code or thread.
//
// THE FOLDER is the project's in the editor and the program's in the game
// (0266 point 3); a row's `/`-separated path is joined onto it as it is.
//
// Constraints: called between frames only, since a load waits for the card to
// go idle. `scratch` holds one file at a time, rewound after each. A full store
// (VOE_3D_MODELS) ends an update: the store said so once, and new paths are
// not read again each frame; a larger store would lift it.
#pragma once

#include <3d/models.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <render/device.h>

#include <stdint.h>

// How many paths failed in one call, and the first of them (NULL for none).
typedef struct {
	uint32_t count;
	const char *first;
} voe_game_models_failures;

// Reads and loads every model row's path the store lacks. See the header.
voe_game_models_failures voe_game_models_update(const voe_ecs_world *world,
						voe_3d_models *models,
						voe_render_device *device,
						const char *folder,
						voe_base_arena *scratch);

// Reads and loads again every entry whose file's stamp changed. See the header.
voe_game_models_failures voe_game_models_watch(voe_3d_models *models,
					       voe_render_device *device,
					       const char *folder,
					       voe_base_arena *scratch);
