// The tree species a game grows into its model store (0515 points 3 and 5):
// each one's `.tree` path and the values its file holds, grown into its
// variations `<path>#<v>` and the species entry at `<path>` that holds them.
//
//     const voe_game_trees voe_game_trees_cooked = {
//             (const voe_game_tree[]){ { "Assets/pine.tree",
//                     { .bark = "Assets/bark.material",
//                       .height = 12.0f, ..., .variations = 4 } } },
//             1 };
//
//     failures = voe_game_trees_grow(models, device, &voe_game_trees_cooked,
//                                    scratch, progress);
//
// DEFINED BY THE COOKED materials.c IN A PROJECT'S GAME TREE, NOT BY THIS
// FOLDER, as game/materials.h's tables are; a game parses no `.tree` (0236).
//
// TWO WAYS TO GROW. _grow, at start under a splash or on the owner's thread:
// every variation of every row a task on one burst (0485, 0504), each on an
// arena of its own, then each uploaded on the caller wearing its row's bark
// and leaf, and the species set. A job, between frames (0386): one species'
// variations grown on a burst the caller polls, then one put a call.
//
// THREADS PER 0370: the uploads use the device from the owner's thread
// between frames or the one other thread, the splash wait's; a task touches
// only its own arena, never the store or the device.
//
// Constraints: _grow holds every variation of every row until its upload; a
// burst per species would lift that. A row's path fits a kind path
// (VOE_ASSETS_LANDSCAPE_KIND_PATH) and its variations are 1 to 16, asserted.
#pragma once

#include <assets/tree.h>

#include <base/arena.h>
#include <base/error.h>

#include <game/models.h>
#include <game/progress.h>

#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// One species: its `.tree` path and its file's values.
typedef struct {
	// Project-relative, `/`-separated, as a foliage kind names it.
	const char *path;
	voe_assets_tree values;
} voe_game_tree;

// The table a game grows.
typedef struct {
	const voe_game_tree *rows;
	uint32_t count;
} voe_game_trees;

// Defined by the game tree's cooked materials.c.
extern const voe_game_trees voe_game_trees_cooked;

// Grows every variation of every row of `trees` on one burst, as wide as a
// splash's when `progress` is given, else as a running program's; then on
// the caller puts each wearing its row's bark and leaf and sets the species.
// "Growing trees" done/total over every variation into `progress` (NULL for
// none); a stop asked returns before the next species. Scratch rewound. A
// variation that will not prepare is kept failed; each failure counted.
voe_game_models_failures voe_game_trees_grow(voe_3d_models *models,
					     voe_render_device *device,
					     const voe_game_trees *trees,
					     voe_base_arena *scratch,
					     voe_game_progress *progress);

// One species grown between frames: started, polled, put a variation a call.
typedef struct voe_game_trees_job voe_game_trees_job;

voe_game_trees_job *voe_game_trees_job_new(void);
// Waits for a burst still running, then frees what it grew.
void voe_game_trees_job_destroy(voe_game_trees_job *job);

// Copies `tree` and starts growing its variations on a burst as wide as a
// running program's. False, nothing started, while the job is busy: started
// and not yet put whole.
[[nodiscard]] bool voe_game_trees_job_start(voe_game_trees_job *job,
					    const voe_game_tree *tree);

// Whether the started burst has ended, so a put may follow. Never waits;
// false when nothing is started.
bool voe_game_trees_job_done(const voe_game_trees_job *job);

// Puts the next variation, as _grow does, between frames on the device's
// owner; asserts _done. True when that was the last and the species is set,
// the job idle again. `error` VOE_BASE_OK, or why this call's variation or
// the species set failed; the variation is then kept failed.
bool voe_game_trees_job_put(voe_game_trees_job *job, voe_3d_models *models,
			    voe_render_device *device, voe_base_error *error);
