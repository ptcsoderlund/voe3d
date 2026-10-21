// A batch of model files loaded in timed phases over the store's pool (0448
// points 3 and 4): game/src/models.c's update, watch and reload fill one a
// chunk at a time, prepare it and put its files in order. Internal to `game`.
//
//     voe_game_models_batch *batch = voe_game_models_batch_new(folder, room);
//     voe_game_models_batch_add(batch, path, known, forced);
//     voe_game_models_batch_prepare(batch, workers, &times);
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
// THE PHASES, each one voe_base_workers_run, each task under its file's path
// as the subject: reading (stamp; bytes, skipped at the known stamp unless
// forced), parsing (the `.glb`, pictures left encoded), making LODs (its
// levels file read or made over the pool, and written), decoding pictures (one
// task a picture of the batch), baking (3d's prepare). No store and no device
// is touched in them; a `.landscape` or picture path is only read, and loaded
// whole at its put.
//
// THE PUT, one file on the device's thread: one with no stamp, known at
// another or not yet held, or that will not read, is kept failed at 0 with its
// line; a `.glb` refused by its parse, levels make, a picture or its bake is
// kept failed at its stamp, the cause said where it was met (0445 point 4);
// each failure counted.
//
// THE TIMES are each phase's wall seconds over every chunk, and the files read;
// their line, `game: models: <n> files in <s> s: reading <s> s, …`, or the
// materials' with their three phases (game/src/models_materials.c), goes to
// stderr only when a file was read, and is no report: nothing went wrong.
//
// Constraints: the folder is kept by pointer and must outlive the batch. A
// batch holds at most its room; a whole chunk's files are in memory at once,
// which is why chunks are the pool's threads and not every path.
#pragma once

#include <game/models.h>

#include <3d/models.h>

#include <base/arena.h>
#include <base/error.h>
#include <base/workers.h>

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
	VOE_GAME_MODELS_PHASES
};

// The phases a line prints, a bit a slot: a model's six, a material's three.
enum {
	VOE_GAME_MODELS_LINE = (1u << VOE_GAME_MODELS_PHASES) - 1,
	VOE_GAME_MATERIALS_LINE = 1u << VOE_GAME_MODELS_READING |
				  1u << VOE_GAME_MODELS_DECODING |
				  1u << VOE_GAME_MODELS_UPLOADING
};

// Each phase's wall seconds, and the files whose bytes were read.
typedef struct {
	double seconds[VOE_GAME_MODELS_PHASES];
	uint32_t files;
} voe_game_models_times;

typedef struct voe_game_models_batch voe_game_models_batch;

// An empty batch for up to `room` files of `folder`.
voe_game_models_batch *voe_game_models_batch_new(const char *folder,
						 uint32_t room);
// Every arena it still holds given back.
void voe_game_models_batch_destroy(voe_game_models_batch *batch);

// `path`, copied, known at `known`, loaded whatever its stamp when `forced`.
// Asserts room is left.
void voe_game_models_batch_add(voe_game_models_batch *batch, const char *path,
			       uint64_t known, bool forced);
uint32_t voe_game_models_batch_count(const voe_game_models_batch *batch);

// The five phases before the put, each over `workers` (NULL in order).
void voe_game_models_batch_prepare(voe_game_models_batch *batch,
				   voe_base_workers *workers,
				   voe_game_models_times *times);

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
// is false, its swaps patched to a `.parts` that reads, else made over
// `workers` and written, `*made` then true. A failed write is a line on
// stderr, and the levels just made or patched are handed back still. NULL,
// `error` set, only when `glb` will not parse.
const uint8_t *voe_game_models_levels_for(const char *folder, const char *path,
					  const uint8_t *glb, size_t glb_size,
					  bool remake, voe_base_arena *scratch,
					  voe_base_workers *workers,
					  size_t *size, bool *made,
					  voe_base_error *error);
