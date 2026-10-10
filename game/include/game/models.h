// The model files a world names, read from a folder into a store (0277 points
// 4 and 5): the one loader the game and the editor share.
//
//     voe_game_models_failures failures = voe_game_models_update(
//             world, models, device, folder, scratch, NULL);
//     if (failures.count > 0)
//             ... // failures.first is the first path that would not load
//     voe_game_models_watch(models, device, folder, scratch);  // re-exports
//
// UPDATE reads every path a model row names that the store holds no entry for:
// `folder` joined with the row's path, its stamp taken, the file read and
// loaded. A file with no stamp, or one that will not read, is kept as a failed
// entry at stamp 0; one that will not parse is kept failed at its stamp. So a
// broken file is read once, not once a frame. Empty paths are skipped. Every
// emitter row's texture is read the same way, the store making a picture of a
// `.png` or `.jpg` (0298 point 5); when the world has any emitter, the soft dot
// is loaded at "" if the store lacks it, a failure counted with first "".
//
// PROGRESS (0370 point 3) is NULL outside a splash wait. With one, the
// distinct paths the call will read are counted first, then "Loading models"
// done/total is set before each file and once at the end. Once stop is asked
// the call returns before the next file, the failures so far counted, and
// loads no dot or water; the paths left unread are read by a later call.
//
// WATCH asks every entry's file for its stamp again, pictures as models. A
// stamp that differs from the entry's is read and loaded again: a success
// replaces the parts, a failure keeps them (3d/models.h). A failed entry whose
// file now has a stamp differs from its 0, so it is tried again; a loaded file
// that has gone is kept, failed at 0, and tried again when it comes back. The
// dot is made in code and not watched, nor a landscape (0379 point 2), nor a
// material: its path is no model file, and the editor reloads one itself.
//
// MATERIALS come from a table too (game/materials.h, 0399 point 7): each shape
// and model row's material path the store lacks is loaded from its entry at
// stamp 0, maps read from the folder as files are, or kept failed when the
// table has none; each failure counted as an update's, so it is tried once.
//
// LANDSCAPES come from a table, never a file (game/landscapes.h, 0236): each
// one's millimetres turned into metres in scratch and loaded at stamp 0, a
// failure counted as an update counts one. The game loads its cooked table so
// before its first update, so a scene's landscape rows find their entries.
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

// Loads every shape's and model row's material path the store lacks from
// `table`, failing one it has not. See the header.
voe_game_models_failures
voe_game_models_materials(const voe_ecs_world *world, voe_3d_models *models,
			  voe_render_device *device, const char *folder,
			  const voe_game_materials *table,
			  voe_base_arena *scratch);

// Reads and loads again every entry whose file's stamp changed, landscapes
// and materials aside. See the header.
voe_game_models_failures voe_game_models_watch(voe_3d_models *models,
					       voe_render_device *device,
					       const char *folder,
					       voe_base_arena *scratch);
