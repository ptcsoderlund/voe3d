// The model loader beside the frames (0448 point 5): what voe_game_models_update
// and voe_game_models_watch load, read to baked on a thread while frames go on
// and uploaded one file a frame between them, so a model placed or re-exported
// in the editor never hitches a frame (0386).
//
//     voe_game_models_loader *loader = voe_game_models_loader_new();
//     // each frame, between frames:
//     failures = voe_game_models_loader_step(loader, world, models, device,
//                                            folder, watch_due, scratch);
//     voe_game_models_loader_destroy(loader);
//
// WHAT RUNS WHERE. A step, on the caller between frames: with no batch out, it
// gathers the paths an update would read (model rows, emitters, loaded
// landscapes' kinds; none when the store is full) and, while a watch pass
// runs, the entries the watch would reload, up to a chunk of the store's
// pool's threads, and starts their reading, parsing, LODs, pictures and baking
// on a `thrd_t` of the loader's over that pool. A `watch` true starts a pass
// over every entry when none runs, carried on chunk by chunk. Once that thread
// is done, each step joins it and puts one file of the batch (game/models.h's
// put): its upload is all a step adds to a frame, its time the line's
// uploading; the times' line is written after the last. Each step then loads
// the dot and the water as an update does, short as they are.
//
// THE DROP RULES. A prepared batch for another folder than the step's is
// destroyed whole, nothing put; its paths are gathered again for the new one.
// A prepared file whose path the store now holds at the stamp its reading
// found is not put: another call loaded it meanwhile.
//
// A SPLASH WAIT still calls voe_game_models_update on its worker: the whole
// load at once, with its progress, is what a wait is for.
//
// Constraints: one caller thread. The store's pool must outlive the loader,
// which runs it from its own thread too (base/workers.h allows that). A step
// gathers as an update does, every row looked up in the store each step.
// Destroy joins a running prepare, so it may wait out one chunk's reading to
// baking; nothing is put and no store is touched.
#pragma once

#include <game/models.h>

#include <3d/models.h>

#include <base/arena.h>

#include <ecs/world.h>

#include <render/device.h>

#include <stdbool.h>

typedef struct voe_game_models_loader voe_game_models_loader;

voe_game_models_loader *voe_game_models_loader_new(void);
// Joins its thread, then gives back the batch it held; no store is touched.
void voe_game_models_loader_destroy(voe_game_models_loader *loader);

// One step between frames: a prepared batch's next file put, or a new batch
// gathered and started, then the dot and the water. `watch` asks for a watch
// pass; `scratch` is as the update's. The failures of what this call put.
// See the header.
voe_game_models_failures voe_game_models_loader_step(
	voe_game_models_loader *loader, const voe_ecs_world *world,
	voe_3d_models *models, voe_render_device *device, const char *folder,
	bool watch, voe_base_arena *scratch);
