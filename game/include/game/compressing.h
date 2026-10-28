// The queue that compresses material maps on the GPU between frames and
// writes their copies (0477 point 3, 0489). Outside a splash a map with no
// fresh copy is not read by the materials call: it is added here by path,
// kind and stamp, and its material loads as if that map were empty. Each
// frame a step moves the oldest job on, and a map whose copy is written,
// `Cache/pictures/<path>.<colour|normal|data>`, is handed back for its
// materials to load again from.
//
//     voe_game_compressing *queue = voe_game_compressing_new();
//     voe_game_compressing_add(queue, folder, path, kind, stamp);
//     // between frames, on the device's thread
//     const char *written = voe_game_compressing_step(queue, device, scratch);
//     if (written != NULL)
//             ... // load the materials naming `written` again
//     voe_game_compressing_destroy(queue, device);
//
// THE STAGES (0489 point 2), each step moving the oldest job at most one on:
// read, decoded and mipped by its kind on a one-worker burst (platform/
// burst.h, 0487 point 5); voe_render_compress_start on the caller; _ready
// polled each step, then _finish; the copy written on a one-worker burst;
// its path handed back. Only the oldest job moves, so one job is on the GPU
// at a time. A step never waits: its longest part is a start's copy of the
// chain into the GPU's buffer.
//
// A map that will not read or decode is one line under its path, said on
// the worker, and its job dropped. A start that fails is render's line and
// the job tries again at the next step.
//
// THE MEMORY IS THE QUEUE'S (rule 11's exception, as the store's is): each
// job has an arena of its own holding its strings, the file's bytes, the
// decoded picture, its chain and its blocks, given back when it is written,
// cleared or destroyed. A 2048² map is about 50 MB while it runs.
//
// Constraints: at most VOE_GAME_COMPRESSING jobs wait; one more is dropped
// with one line, and that map waits for the next open, Play or Ship to be
// cooked. A second add of a waiting path and kind replaces it and starts
// over: at once when nothing of it runs, else when its running stage ends,
// keeping its folder (the queue is cleared when the folder changes). Jobs
// are found by a scan, which is all sixteen allow.
#pragma once

#include <assets/image.h>

#include <base/arena.h>

#include <render/device.h>

#include <stdint.h>

// How many maps wait at most (0477 point 4).
#define VOE_GAME_COMPRESSING 16

typedef struct voe_game_compressing voe_game_compressing;

voe_game_compressing *voe_game_compressing_new(void);
// Cleared as _clear does, then freed.
void voe_game_compressing_destroy(voe_game_compressing *queue,
				  voe_render_device *device);

// The map `path` in `folder` of `kind` at `stamp`, to be compressed, replacing
// a waiting job of the same path and kind, which starts over.
void voe_game_compressing_add(voe_game_compressing *queue, const char *folder,
			      const char *path, voe_assets_image_kind kind,
			      uint64_t stamp);

// The oldest job moved one stage on, never waiting. Its path, copied onto
// `scratch`, when its copy keyed by its stamp is written, else NULL.
const char *voe_game_compressing_step(voe_game_compressing *queue,
				      voe_render_device *device,
				      voe_base_arena *scratch);

// How many jobs wait.
uint32_t voe_game_compressing_waiting(const voe_game_compressing *queue);

// Every waiting job dropped and its memory given back: a running burst
// finished, waiting for it, and a job on the GPU dropped.
void voe_game_compressing_clear(voe_game_compressing *queue,
				voe_render_device *device);
