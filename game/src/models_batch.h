// A batch of model files loaded in timed phases on the calling thread (0457,
// 0459 point 1): game/src/models.c's update, watch and reload fill one with a
// single file at a time, prepare it and put it, and prepare one to cook its
// copy. Internal to `game`.
//
//     voe_game_models_batch *batch = voe_game_models_batch_new(
//             folder, 1, VOE_GAME_MODELS_COOK_STALE, progress);
//     voe_game_models_batch_add(batch, path, known, forced);
//     voe_game_models_batch_prepare(batch, &times);
//     for (uint32_t i = 0; i < voe_game_models_batch_count(batch); i++)
//             if (!voe_game_models_batch_put(batch, models, device, i,
//                                            &failures, &error, &times))
//                     break;   // the store is full
//     voe_game_models_batch_destroy(batch);
//     voe_game_models_times_write(&times, "models", VOE_GAME_MODELS_LINE);
//
// A FILE is a copy of its path, the stamp it is known at and whether it loads
// whatever its stamp; read, it gets an arena of its own, and each picture of
// its `.glb` one more, all destroyed after its put.
//
// THE PHASES run in order on the calling thread, each one's tasks index by
// index, each under its file's path as the subject: reading (stamp; a
// `.glb`'s cooked copy taken, prepared then, or the bytes, all skipped at the
// known stamp unless forced), parsing (the `.glb`, pictures left encoded),
// making LODs (its levels file read, or made and written), decoding pictures
// (one task a picture), baking (3d's prepare), and after them cooking (the
// copy written, as the batch's mode says: models_cooked.h). No store and no
// device is touched in them; a `.landscape` or picture path is only read, and
// loaded whole at its put. The progress, when given, shows a file's phase
// from its reading: "Preparing models" when it will be cooked, else "Loading
// models".
//
// THE PUT, one file on the device's thread: one with no stamp, known at
// another or not yet held, or that will not read, is kept failed at 0 with its
// line; a `.glb` refused by its parse, levels make, a picture or its bake is
// kept failed at its stamp, the cause said where it was met (0445 point 4);
// each failure counted.
//
// THE TIMES are each phase's wall seconds over every batch, and the files
// or copies read; their line, `game: models: <n> files in <s> s: reading <s> s, …`,
// or the materials' with their three phases (game/src/models_materials.c),
// goes to stderr only when a file was read, and is no report: nothing went
// wrong.
//
// Constraints: the folder is kept by pointer and must outlive the batch. A
// batch holds at most its room, its files in memory at once: the loader's
// hold one file, so one file's arenas at a time.
#pragma once

#include "models_cooked.h"

#include <game/models.h>
#include <game/progress.h>

#include <3d/models.h>

#include <base/arena.h>
#include <base/error.h>

#include <render/device.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The phases of 0448 point 3, in order: the slots of the times.
enum {
	VOE_GAME_MODELS_READING,
	VOE_GAME_MODELS_PARSING,
	VOE_GAME_MODELS_MAKING_LODS,
	VOE_GAME_MODELS_DECODING,
	VOE_GAME_MODELS_BAKING,
	VOE_GAME_MODELS_UPLOADING,
	VOE_GAME_MODELS_COOKING,
	VOE_GAME_MODELS_PHASES
};

// The phases a line prints, a bit a slot: a model's seven, a material's three.
enum {
	VOE_GAME_MODELS_LINE = (1u << VOE_GAME_MODELS_PHASES) - 1,
	VOE_GAME_MATERIALS_LINE = 1u << VOE_GAME_MODELS_READING |
				  1u << VOE_GAME_MODELS_DECODING |
				  1u << VOE_GAME_MODELS_UPLOADING
};

// Each phase's wall seconds, and the files whose bytes or copy were read.
typedef struct {
	double seconds[VOE_GAME_MODELS_PHASES];
	uint32_t files;
} voe_game_models_times;

typedef struct voe_game_models_batch voe_game_models_batch;

// An empty batch for up to `room` files of `folder`, taking and writing
// copies as `cooking` says, each file's phase shown in `progress` (NULL for
// none).
voe_game_models_batch *
voe_game_models_batch_new(const char *folder, uint32_t room,
			  enum voe_game_models_cooking cooking,
			  voe_game_progress *progress);
// Every arena it still holds given back.
void voe_game_models_batch_destroy(voe_game_models_batch *batch);

// `path`, copied, known at `known`, loaded whatever its stamp when `forced`.
// Asserts room is left.
void voe_game_models_batch_add(voe_game_models_batch *batch, const char *path,
			       uint64_t known, bool forced);
uint32_t voe_game_models_batch_count(const voe_game_models_batch *batch);

// Every phase but uploading, in order on the calling thread.
void voe_game_models_batch_prepare(voe_game_models_batch *batch,
				   voe_game_models_times *times);

// Whether prepared file `index`'s copy was made in cooking.
bool voe_game_models_batch_cooked(const voe_game_models_batch *batch,
				  uint32_t index);

// The phase prepared file `index` shows: "Preparing models" when it is
// cooked, else "Loading models".
const char *voe_game_models_batch_phase(const voe_game_models_batch *batch,
					uint32_t index);

// File `index` into the store, then its arenas given back; a failure counted
// into `failures`, its category in `error` (OK for none). False, nothing done,
// when the path is not held and the store is full.
[[nodiscard]] bool voe_game_models_batch_put(voe_game_models_batch *batch,
					     voe_3d_models *models,
					     voe_render_device *device,
					     uint32_t index,
					     voe_game_models_failures *failures,
					     voe_base_error *error,
					     voe_game_models_times *times);

// Whether the store holds prepared file `index`'s path at the stamp its
// reading found, 0 for none: a put of it would change nothing it should.
bool voe_game_models_batch_held(const voe_game_models_batch *batch,
				const voe_3d_models *models, uint32_t index);

// The kind paths an update reads after the world's: the store's own copies,
// in the landscapes' arenas, which a load elsewhere in the store never moves.
typedef struct {
	const char *paths[VOE_3D_MODELS];
	uint32_t count;
} voe_game_models_kinds;

// Every distinct kind path of the store's loaded landscapes an update reads
// and the world's model rows and emitters do not name, at most VOE_3D_MODELS.
void voe_game_models_kinds_gather(const voe_ecs_world *world,
				  const voe_3d_models *models,
				  voe_game_models_kinds *kinds);

// Adds to `batch`, until it holds `room`, every path from the `next`th on an
// update reads, named first there: the model rows', the emitters', then
// `kinds`. The index after the last one looked at.
uint32_t voe_game_models_update_gather(const voe_ecs_world *world,
				       const voe_3d_models *models,
				       const voe_game_models_kinds *kinds,
				       uint32_t next, uint32_t room,
				       voe_game_models_batch *batch);

// Adds to `batch`, until it holds `room`, every store entry from the `next`th
// on the watch reads, at its stamp: landscapes and materials aside. The index
// after the last one looked at.
uint32_t voe_game_models_watch_gather(const voe_3d_models *models,
				      uint32_t next, uint32_t room,
				      voe_game_models_batch *batch);

// The soft dot when the world has an emitter and the water record when it
// has a water, each loaded when the store lacks it, a failure counted.
void voe_game_models_dot_and_water(const voe_ecs_world *world,
				   voe_3d_models *models,
				   voe_render_device *device,
				   voe_game_models_failures *failures);

// The times' line on stderr, when a file was read: `game: <noun>: …` with the
// phases `line` names, a VOE_GAME_*_LINE.
void voe_game_models_times_write(const voe_game_models_times *times,
				 const char *noun, uint32_t line);

// Counts one failure of `path`, the store's copy kept when it is the first.
void voe_game_models_failure_count(voe_game_models_failures *failures,
				   const voe_3d_models *models,
				   const char *path);

// `levels`, a folder's Cache/levels, joined with `path` and `.levels`.
const char *voe_game_models_levels_file(const char *levels, const char *path,
					voe_base_arena *scratch);

// The levels file of `folder`'s `path`, whose bytes are `glb`, on scratch with
// its count in `*size`: the one in the Cache when it fits `glb` and `remake`
// is false, its swaps patched to a `.parts` that reads, else made and
// written, `*made` then true. A failed write is a line on stderr, and the
// levels just made or patched are handed back still. NULL, `error` set, only
// when `glb` will not parse.
const uint8_t *voe_game_models_levels_for(const char *folder, const char *path,
					  const uint8_t *glb, size_t glb_size,
					  bool remake, voe_base_arena *scratch,
					  size_t *size, bool *made,
					  voe_base_error *error);
