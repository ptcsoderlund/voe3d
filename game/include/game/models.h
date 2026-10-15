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
// LEVELS (0426 point 2): a `.glb` path, any case, loads with its levels file
// `<folder>/Cache/levels/<path>.levels`, made when missing or stale, in update
// and watch alike. The write is atomic, its folders made as needed; a failed
// one is a line on stderr, the levels made still drawn. `Cache/` may go any
// time. The file is made with the count, strength and swap of the `.parts`
// beside the `.glb`, the defaults when none (0430 point 5, 0432 point 2), so a
// shipped game, which has none, uses the files it ships.
//
// PROGRESS (0370 point 3) is NULL outside a splash wait. With one, the
// distinct paths the call will read are counted first, then "Loading models"
// done/total is set before each file and once at the end. Once stop is asked
// the call returns before the next file, the failures so far counted, and
// loads no dot or water; the paths left unread are read by a later call.
//
// WATCH, a stamp of time and size asked once a second, no watcher thread (0277
// point 5), reloads every entry, pictures too, whose stamp differs: a success
// replaces the parts, a failure keeps them (3d/models.h). A failed entry whose
// file now has a stamp differs from its 0 and is tried again; a loaded file
// that has gone is kept failed at 0, tried again when it is back.
//
// MATERIALS come from tables (game/materials.h, 0399 point 7, 0420): each
// loaded model takes its parts from the parts table, none for a path it lacks;
// then each shape's material, part's `wears` and landscape's layer (0402 point
// 8) the store lacks loads from the material table at stamp 0, its maps read as
// files are, or stays failed if that table has none, each failure counted once.
//
// LANDSCAPES come from a table, never a file (game/landscapes.h, 0236): each
// one's millimetres made metres in scratch, its layers and paint carried, and
// loaded at stamp 0, a failure counted as an update's.
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
// header. The game loads its cooked table before its first update, so
// landscape rows find their entries.
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
// store lacks from `table`, failing one it has not. See the header. Call it
// after a watch too, so a reloaded model wears its parts.
voe_game_models_failures
voe_game_models_materials(const voe_ecs_world *world, voe_3d_models *models,
			  voe_render_device *device, const char *folder,
			  const voe_game_materials *table,
			  const voe_game_parts *parts, voe_base_arena *scratch);

// Reads `<folder>/<path>` and its levels file, making and writing that when
// missing or stale, `made` saying so. `remake` false keeps a fresh file; true
// makes and writes it whatever is there, `made` then true whenever the `.glb`
// reads and parses. False, `error` set, only when the `.glb` will not read or
// parse; a write that fails is a line on stderr and true. Scratch rewound to
// where it stood. It makes the file alone: the editor's import keeps a fresh
// one, its Make levels makes it again (0429). See the header.
[[nodiscard]] bool voe_game_models_prepare(const char *folder, const char *path,
					   voe_base_arena *scratch, bool remake,
					   bool *made, voe_base_error *error);

// A prepare rewrites only the levels file, which no stamp covers, so the
// watch would keep drawing the levels it loaded (0431).
//
// Reads `<folder>/<path>` again when the store holds `path`
// (`voe_3d_models_find`) and loads it with its levels file whatever its stamp,
// at the file's stamp; a path it lacks is nothing and true. False, `error`
// set, when the load failed, the entry kept as the store keeps a failed
// reload. Between frames; call the materials call after, as after a watch.
// For the editor's prepare (0431).
[[nodiscard]] bool voe_game_models_reload(voe_3d_models *models,
					  voe_render_device *device,
					  const char *folder, const char *path,
					  voe_base_arena *scratch,
					  voe_base_error *error);

// Reads and loads again every entry whose file's stamp changed, landscapes
// and materials aside. See the header. The dot is made in code and not
// watched, nor a landscape (0379 point 2), nor a material: its path is no
// model file; the editor reloads one itself.
voe_game_models_failures voe_game_models_watch(voe_3d_models *models,
					       voe_render_device *device,
					       const char *folder,
					       voe_base_arena *scratch);
