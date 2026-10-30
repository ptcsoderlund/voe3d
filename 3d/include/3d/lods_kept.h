// Each copy's LOD as one view last showed it, kept from frame to frame so a
// copy swaps coarser at its bound and back only once 10% nearer, and dissolves
// over a quarter second at each swap (0495 point 2, 0500 point 1, 0501).
//
//     voe_3d_lods_kept *kept = voe_3d_lods_kept_create(arena);  // once
//     voe_3d_lods_kept_begin(kept, voe_platform_clock_now());   // a frame
//     choice = voe_3d_lods_kept_choose(kept, voe_3d_lods_kept_row_key(row),
//                                      model, &placed, eye, &view);
//
// ONE PER TARGET, AND THE CALLER KEEPS IT, as bounce_casters.h's memory: one
// per editor view, one for the preview, one for the game's window, made once
// on an arena that lives as long as the target. Only this file writes it. A
// NULL memory chooses with no state and draws one LOD whole (dev, tests, the
// bounce's captures).
//
// THE CLOCK is the caller's wall clock in seconds, handed to _begin every
// frame drawn, never going back (0501 point 2): a paused world does not stop
// a dissolve.
//
// FIRST SIGHT is a key not chosen in the memory's previous frame (0501 point
// 3): a copy culled last frame, a view not drawn last frame, a new view. It
// takes its distance's LOD whole. A kept LOD no longer below the model's
// level count (a reload, or two keys colliding) is first sight too.
//
// A KEY is a copy's place (0501 point 4): a model row's entity, or a foliage
// copy's landscape, kind and painted x and z mixed, the top bit set. Two keys
// that collide share one memory.
//
// THE ROOM: VOE_3D_LODS_KEPT keys a frame. Two open-addressed tables of twice
// that many slots, this frame's and the last, swap roles at _begin; a slot is
// this frame's when its stamp is this frame's number, so nothing is cleared.
// A key past the room chooses with no state and draws whole, unremembered.
//
// Constraints: 24 bytes a slot, 2 × 2^18 slots, 12 MiB a memory, pushed at
// _create; a smaller entry or a growable table would lift it. Frame numbers
// are 32-bit: past 2^32 frames (2 years at 60 a second) a slot left since
// then could read as live. Single-threaded.
#pragma once

#include <3d/model_levels.h>

#include <base/arena.h>
#include <ecs/world.h>

#include <stdbool.h>
#include <stdint.h>

// How many copies a memory keeps a frame, the foliage instance cap (0501 pt 6).
#define VOE_3D_LODS_KEPT (1u << 17)
// Seconds a swap dissolves over (0500 point 1).
#define VOE_3D_LODS_DISSOLVE 0.25
// The share of `s` a finer LOD must clear to swap back: 10% nearer (0495).
#define VOE_3D_LODS_BACK 0.9f

typedef struct voe_3d_lods_kept voe_3d_lods_kept;

// One memory on `arena`, empty; it lives as long as the arena is not rewound
// past it.
voe_3d_lods_kept *voe_3d_lods_kept_create(voe_base_arena *arena);

// A new frame at `now` seconds, not before the last: this frame's table
// becomes last frame's and the other, by its stamp, empty.
void voe_3d_lods_kept_begin(voe_3d_lods_kept *kept, double now);

// `key`'s choice of `model`'s level this frame, as the view `view` about `eye`
// sees it placed at `placed`. With s from voe_3d_model_level_size, D its level
// and D' that of 0.9 s: first sight, a NULL `kept` or no room takes D whole;
// else from the kept L, D > L swaps to D, D' < L to D', else L stays. A swap
// dissolves from L over VOE_3D_LODS_DISSOLVE, `shown` the time since ÷ it; after,
// whole. A second choose of a key in a frame answers as the first.
voe_3d_model_level_choice
voe_3d_lods_kept_choose(voe_3d_lods_kept *kept, uint64_t key,
			const voe_3d_model_entry *model,
			const voe_scene_transform *placed, voe_math_double3 eye,
			const voe_render_view *view);

// `key`'s choice this frame into `out`, else last frame's as it stood then;
// false when neither frame chose it.
[[nodiscard]] bool voe_3d_lods_kept_find(const voe_3d_lods_kept *kept,
					 uint64_t key,
					 voe_3d_model_level_choice *out);

// A model row's key: its entity, the top bit clear.
uint64_t voe_3d_lods_kept_row_key(voe_ecs_entity row);

// A foliage copy's key: its landscape row, kind and painted x and z mixed,
// the top bit set.
uint64_t voe_3d_lods_kept_copy_key(voe_ecs_entity landscape, uint32_t kind,
				   int32_t x, int32_t z);
