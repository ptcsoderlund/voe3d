// A `.glb`'s levels file: the levels the engine makes of a model, as the bytes
// kept in the project's Cache (0426 point 1) and read back by the model store;
// and the level a view draws of a loaded model. `game` makes and reads the
// file; the store reads it at load; the draws and the Inspector choose with it.
//
//     const uint8_t *file;
//     size_t file_size;
//     voe_3d_model_levels levels;
//
//     if (!voe_3d_model_levels_read(arena, file, file_size, glb, glb_size,
//                                   &levels)) {     // missing or stale
//             file = voe_3d_model_levels_make(arena, glb, glb_size,
//                                             VOE_ASSETS_LEVELS_DEFAULT,
//                                             &file_size, &error);
//             if (file == NULL)
//                     ... // the store's load fails the same way
//             ... // write file, file_size; read it back as above
//     }
//     voe_base_arena_rewind(arena, mark);  // the caller's, as everything here
//
// THE LEVELS (0425 point 1, 0430 point 3, 0436 point 5). Level 0 is the file as
// baked; `levels.count` (1 to 6) is how many there are at most, level 0
// included. Each part is simplified on its own and always from level 0, aiming
// at its triangles × `levels.shares[k]` ÷ 100 rounded down, until it gets there
// or a collapse would cost more than a quarter of the model's bounding radius
// (the farthest baked vertex from its box's centre). Normals within 60° share a
// side. Both are fixed (0437). A level's error is the largest of
// its parts', in model metres. A level not below the one before in triangles
// ends the list: it and the ones after are dropped. Level k swaps in at its own
// `levels.swaps[k]` times the distance its error allows (0436 point 4), not
// times the level before's. One that lands at or nearer than the level before
// is pushed out to it (0435 point 2), and of two at one distance the later is
// drawn: the earlier never shows. Swaps 1 1 1 2 1 most often skip level 4.
//
// A FILE WITH ITS OWN LEVELS (0435 point 3, 0436 point 6): a node named `_LOD1`
// to `_LOD5` anywhere makes the file `own`, never simplified, no parts; each
// own level is baked and measured both ways against level 0 and stored with
// its error, one whose bake fails or holds no triangle ending the list.
//
// THE LAYOUT, native-endian, every field a uint32_t but the hash and floats:
//
//     magic "VOEL" | version 5 | FNV-1a 64 of the .glb (uint64_t)
//     own | level count | part count | swaps of levels 1 to 5 (5 floats)
//     per level k >= 1: its error (float), then per part (none when own)
//         its index count | that many indices into level 0's baked part
//
// STALE IS NEVER AN ERROR (0426 point 1): a file whose magic, version or hash
// differs, a count past its limit (levels, parts, whole triangles), an error
// not finite and at least nought, a swap not finite and above nought, bytes
// cut short or left over: _read says false and the file is made again.
//
// THE CHOICE HAS NO STATE (0425 point 3), so every asker gets one answer. `s` =
// world radius × |P[1][1]| ÷ the centre's clip w, the world radius the model's
// radius times its largest scale component: the radius as a fraction of half
// the screen. The level is the largest k below `level_count` whose
// `levels[k].bound` b_k is above `s`; level 0 when the eye is inside the world
// sphere, when the w is not above nought, or for an entry with one level. From
// b_k down to low, the larger of 0.8 × b_k and b_(k+1), is a band (0432 point
// 3): `shown` = (b_k − s) ÷ (b_k − low), level k − 1 drawing the rest; whole
// below it, for level 0 and an infinite bound.
#pragma once

#include <3d/models.h>

#include <assets/parts.h>
#include <base/arena.h>
#include <base/error.h>

#include <math/double3.h>
#include <render/device.h>
#include <scene/transform_component.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	// The file has its own levels, measured and not simplified.
	bool own;
	// Levels including level 0, made or own: 1 when none was kept.
	uint32_t level_count;
	// Level 0's baked parts; 0 when own, whose levels hold no indices.
	uint32_t part_count;
	// Each level's error in model metres; level 0's nought.
	float errors[VOE_3D_MODEL_LEVELS];
	// How many times the distance its error allows level k swaps in at
	// (0436 point 4); `swaps[0]` 1; own files carry them too.
	float swaps[VOE_3D_MODEL_LEVELS];
	// Level k's indices into level 0's part p, counted from the part's first
	// vertex; level 0's are unused, the bake's own indices being level 0.
	uint32_t *indices[VOE_3D_MODEL_LEVELS][VOE_3D_MODEL_PARTS];
	uint32_t index_counts[VOE_3D_MODEL_LEVELS][VOE_3D_MODEL_PARTS];
} voe_3d_model_levels;

// FNV-1a 64 of `size` bytes.
uint64_t voe_3d_model_levels_hash(const uint8_t *bytes, size_t size);

// Parses `glb` with its pictures left encoded (none is decoded), bakes level 0,
// makes at most `levels.count` levels in all, each cut to its
// `levels.shares[k]`, and writes the file's bytes with `levels.swaps[1]` to
// `[5]`, each asserted within VOE_ASSETS_LEVELS_SWAP_LEAST to _MOST, returned
// with their count in `*size`, all on `arena`. No task, no pool: the made
// levels are simplified in order on the caller. The bytes
// are the return and not an out-parameter, since rule 6 allows no `**`. NULL
// with `error` set as the store's load would fail: MALFORMED or UNSUPPORTED as
// the reader says, UNSUPPORTED or REFUSED as the bake says.
[[nodiscard]] const uint8_t *
voe_3d_model_levels_make(voe_base_arena *arena,
			 const uint8_t *glb, size_t glb_size,
			 voe_assets_levels levels, size_t *size,
			 voe_base_error *error);

// Reads a levels file made for `glb` into `out`, its indices copied onto
// `arena`. False when the file is stale, `out` then not to be read. Versions
// 1 to 3 predate the strength, the swap and six levels (0430 point 7, 0432
// point 2); 4 remade the files made when the default count was four (0433)
// and had one swap (0436). An index is not checked against its part's vertices,
// which this file does not know: the hash ties it to its `.glb`, and the store
// holds the vertex counts.
[[nodiscard]] bool voe_3d_model_levels_read(voe_base_arena *arena,
					    const uint8_t *bytes, size_t size,
					    const uint8_t *glb, size_t glb_size,
					    voe_3d_model_levels *out);

// Writes `swaps[1]` to `swaps[5]` into the head of the levels file `bytes`, of
// `size` bytes, in place; `swaps[0]` is not read. The file stays fresh for its
// `.glb`, which is how a change of swaps alone avoids a remake (0435 point 4).
// Asserts the magic, version 5, room for the head, and each swap within
// VOE_ASSETS_LEVELS_SWAP_LEAST to _MOST.
void voe_3d_model_levels_swaps_set(uint8_t *bytes, size_t size,
				   const float swaps[VOE_3D_MODEL_LEVELS]);

// Each level's triangles into `triangles`, for the LODs panel: how many levels
// it wrote, 0 when `glb` will not parse or bake. `*own` whether the `.glb` has
// its own levels, `*fresh` whether `levels` read as a fresh file of it. Own:
// each own level's bake, ending as _make ends. Made: level 0's bake, then a
// fresh file's index counts ÷ 3 per level; level 0 alone when `levels` is NULL
// or stale. No picture decoded. Everything on `arena`, the caller rewinding.
uint32_t voe_3d_model_levels_triangles(voe_base_arena *arena,
				       const uint8_t *glb, size_t glb_size,
				       const uint8_t *levels, size_t levels_size,
				       uint32_t triangles[VOE_3D_MODEL_LEVELS],
				       bool *own, bool *fresh);

// A view's choice of a model's level.
typedef struct {
	// The level, as voe_3d_model_level answers.
	uint32_t level;
	// In (0, 1]: the share of the model's pixels `level` draws, `level` − 1
	// drawing the rest when under 1; 1 for level 0.
	float shown;
} voe_3d_model_level_choice;

// The level `view` draws of `model` placed at `placed`, the view about `eye`
// (eye-relative, 0250), and its share, as the paragraph above says.
voe_3d_model_level_choice
voe_3d_model_level_choose(const voe_3d_model_entry *model,
			  const voe_scene_transform *placed,
			  voe_math_double3 eye, const voe_render_view *view);

// voe_3d_model_level_choose's level.
uint32_t voe_3d_model_level(const voe_3d_model_entry *model,
			    const voe_scene_transform *placed,
			    voe_math_double3 eye, const voe_render_view *view);
