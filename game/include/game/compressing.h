// The queue that compresses material maps between frames and writes their
// copies (0477 points 3 and 4). Outside a splash a map with no fresh copy is
// uploaded uncompressed and added here; each frame a step encodes its blocks
// for a budget, and a map whose blocks are all made is written as its copy,
// `Cache/pictures/<path>.<colour|normal|data>`, for its materials to load
// again from.
//
//     voe_game_compressing *queue = voe_game_compressing_new();
//     voe_game_compressing_add(queue, folder, path, kind, stamp, &chain);
//     // between frames
//     const char *written = voe_game_compressing_step(queue, 0.002, scratch);
//     if (written != NULL)
//             ... // load the materials naming `written` again
//     voe_game_compressing_destroy(queue);
//
// ONE THREAD, THE OWNER'S, BETWEEN FRAMES. No worker compresses (0456, 0386):
// a worker would be a new threading decision for a few seconds of waiting
// that the uncompressed upload already hides. A step encodes the oldest job's
// blocks in runs of 64 and reads the clock between runs, so it overruns its
// budget by at most one run; it always encodes one, so a budget of 0 still
// moves.
//
// THE MEMORY IS THE QUEUE'S (rule 11's exception, as the store's is): each
// job holds its own copy of the RGBA8 chain and its blocks, given back when
// it is written, cleared or destroyed. A 2048² job is about 28 MB: the chain's
// 22 MB and the blocks' quarter of it. The write pushes the copy on the
// caller's scratch, rewound after it.
//
// Constraints: at most VOE_GAME_COMPRESSING jobs wait; one more is dropped
// with one line, and that map waits for the next open, Play or Ship to be
// cooked. A second add of a waiting path and kind replaces it and starts
// over. Jobs are found by a scan, which is all sixteen allow.
#pragma once

#include <assets/image.h>

#include <base/arena.h>

#include <stdint.h>

// How many maps wait at most (0477 point 4).
#define VOE_GAME_COMPRESSING 16

typedef struct voe_game_compressing voe_game_compressing;

voe_game_compressing *voe_game_compressing_new(void);
void voe_game_compressing_destroy(voe_game_compressing *queue);

// The map `path` in `folder`, its RGBA8 `chain` of `kind` at `stamp`, copied
// into the queue, replacing a waiting job of the same path and kind.
void voe_game_compressing_add(voe_game_compressing *queue, const char *folder,
			      const char *path, voe_assets_image_kind kind,
			      uint64_t stamp, const voe_assets_image *chain);

// The oldest job's blocks encoded for about `seconds`; an ended job written
// keyed by its stamp and freed. Its path, the queue's copy valid until the
// next call, or NULL when none ended.
const char *voe_game_compressing_step(voe_game_compressing *queue,
				      double seconds, voe_base_arena *scratch);

// How many jobs wait.
uint32_t voe_game_compressing_waiting(const voe_game_compressing *queue);

// Every waiting job dropped and its memory given back.
void voe_game_compressing_clear(voe_game_compressing *queue);
