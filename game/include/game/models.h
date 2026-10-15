// The model files a world names, read from a folder into a store (0277 points
// 4 and 5): the one loader the game and the editor share.
//
//     voe_game_models_failures failures = voe_game_models_update(
//             world, models, device, folder, scratch, NULL);
//     if (failures.count > 0)
//             ... // failures.first is the first path that would not load
//     voe_game_models_watch(models, device, folder, scratch);  // re-exports
//
// UPDATE reads every path a model row names that the store lacks: the folder
// (the project's in the editor, the program's in the game, 0266 point 3) joined
// with the row's `/`-separated path, stamped, read and loaded. A file with no
// stamp or that will not read is kept failed at 0; one that will not parse, at
// its stamp: a broken file is read once, not once a frame. Empty paths are
// skipped. Each emitter's texture is read the same way, a `.png` or `.jpg` made
// a picture (0298 point 5); with any emitter the soft dot is loaded at "" if
// the store lacks it, a failure counted with first "".
//
// LEVELS (0426 point 2): a `.glb` path, in any case, loads with its levels file
// `<folder>/Cache/levels/<path>.levels`, made first when missing or stale, in
// update and watch alike, so a re-export or a hand-copied file is made where it
// is loaded. The write is atomic, its folders made as needed; a failed one is
// a line on stderr, and the levels just made are still drawn. `Cache/` may be
// deleted at any time. _prepare makes the file alone, for the editor's import.
//
// PROGRESS (0370 point 3) is NULL outside a splash wait. With one, the
// distinct paths the call will read are counted first, then "Loading models"
// done/total is set before each file and once at the end. Once stop is asked
// the call returns before the next file, the failures so far counted, and
// loads no dot or water; the paths left unread are read by a later call.
//
// WATCH, a stamp of time and size asked once a second, no watcher thread (0277
// point 5), reads and loads again every entry, pictures too, whose stamp
// differs: a success replaces the parts, a failure keeps them (3d/models.h). A
// failed entry whose file now has a stamp differs from its 0 and is tried
// again; a loaded file that has gone is kept failed at 0, tried again when it
// is back. The dot is made in code and not watched, nor a landscape (0379 point
// 2), nor a material: its path is no model file; the editor reloads one itself.
//
// MATERIALS come from tables too (game/materials.h, 0399 point 7, 0420): each
// loaded model first takes its parts from the parts table, none for a path it
// lacks; then each shape's material, part's `wears` and landscape's layer (0402
// point 8) the store lacks is loaded from the material table at stamp 0, its
// maps read as files are, or kept failed when the table has none, each failure
// counted once. Call it after a watch too, so a reloaded model wears its parts.
//
// LANDSCAPES come from a table, never a file (game/landscapes.h, 0236): each
// one's millimetres made metres in scratch, its layers and paint carried, and
// loaded at stamp 0, a failure counted as an update's. The game loads its
// cooked table before its first update, so landscape rows find their entries.
//
// BOTH RETURN how many paths failed and the first one's path, the store's own
// copy, valid until that entry loads again or the store is cleared; NULL for
// none. Each failure is one line on stderr naming the path and the category.
//
// Constraints: called between frames only, since a load waits for the card to
// go idle. `scratch` holds one file, its levels and their making at a time,
// rewound after each. A full store (VOE_3D_MODELS) ends an update: the store
// said so once, and new paths are not read again each frame; a larger store
// would lift it.
#pragma once

#include <3d/models.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <game/landscapes.h>
#include <game/materials.h>
#include <game/progress.h>

#include <render/device.h>

#include <stdint.h>

// How many paths failed in one call, and the first of them (NULL for none).
typedef struct {
	uint32_t count;
	const char *first;
} voe_game_models_failures;

// Reads and loads every model row's path and emitter's texture the store
// lacks, and the dot for any emitter, reporting into `progress` (NULL for
// none) and stopping when it asks. See the header.
voe_game_models_failures voe_game_models_update(const voe_ecs_world *world,
						voe_3d_models *models,
						voe_render_device *device,
						const char *folder,
						voe_base_arena *scratch,
						voe_game_progress *progress);

// Loads every landscape of `landscapes` into the store at stamp 0. See the
// header.
voe_game_models_failures
voe_game_models_landscapes(voe_3d_models *models, voe_render_device *device,
			   const voe_game_landscapes *landscapes,
			   voe_base_arena *scratch);

// `material`'s non-empty maps read from `folder` into scratch and loaded with
// its values as its path's entry at `stamp`. False, `error` set and the entry
// kept failed, when a map will not read or the store refuses it.
[[nodiscard]] bool voe_game_models_material_load(
	voe_3d_models *models, voe_render_device *device, const char *folder,
	const voe_game_material *material, uint64_t stamp,
	voe_base_arena *scratch, voe_base_error *error);

// Gives every loaded model its parts from `parts`, then loads every shape's
// material path, model part's `wears` and loaded landscape's layer path the
// store lacks from `table`, failing one it has not. See the header.
voe_game_models_failures
voe_game_models_materials(const voe_ecs_world *world, voe_3d_models *models,
			  voe_render_device *device, const char *folder,
			  const voe_game_materials *table,
			  const voe_game_parts *parts, voe_base_arena *scratch);

// Reads `<folder>/<path>` and its levels file, making and writing that when
// missing or stale, `made` saying so. False, `error` set, only when the `.glb`
// will not read or parse; a write that fails is a line on stderr and true.
// Scratch rewound to where it stood. See the header.
[[nodiscard]] bool voe_game_models_prepare(const char *folder, const char *path,
					   voe_base_arena *scratch, bool *made,
					   voe_base_error *error);

// Reads and loads again every entry whose file's stamp changed, landscapes
// and materials aside. See the header.
voe_game_models_failures voe_game_models_watch(voe_3d_models *models,
					       voe_render_device *device,
					       const char *folder,
					       voe_base_arena *scratch);
