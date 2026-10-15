// A `.glb`'s levels file: the levels the engine makes of a model, as the bytes
// kept in the project's Cache (0426 point 1) and read back by the model store.
// `game` makes and reads the file; the store reads it at load.
//
//     const uint8_t *file;
//     size_t file_size;
//     voe_3d_model_levels levels;
//
//     if (!voe_3d_model_levels_read(arena, file, file_size, glb, glb_size,
//                                   &levels)) {     // missing or stale
//             file = voe_3d_model_levels_make(arena, glb, glb_size,
//                                             &file_size, &error);
//             if (file == NULL)
//                     ... // the store's load fails the same way
//             ... // write file, file_size; read it back as above
//     }
//     voe_base_arena_rewind(arena, mark);  // the caller's, as everything here
//
// THE LEVELS (0425 point 1). Level 0 is the file as baked; levels 1 to 3 aim at
// a half, a quarter and an eighth of level 0's triangles, each part simplified
// on its own and always from level 0, until it reaches its target or a collapse
// would cost more than a quarter of the model's bounding radius (the farthest
// baked vertex from its box's centre). A level's error is the largest of its
// parts', in model metres. A level holding more than three quarters of the
// previous level's triangles ends the list: it and the ones after are dropped.
//
// A FILE WITH ITS OWN LEVELS IS NEVER SIMPLIFIED (0425 point 4): a node named
// `_LOD1` to `_LOD3` anywhere makes a file of one level, `own` set, no parts.
//
// THE LAYOUT, native-endian, every field a uint32_t but the hash:
//
//     magic "VOEL" | version 1 | FNV-1a 64 of the .glb (uint64_t)
//     own | level count | part count
//     per made level k >= 1: its error (float), then per part
//         its index count | that many indices into level 0's baked part
//
// STALE IS NEVER AN ERROR (0426 point 1): a file whose magic, version or hash
// differs, a count past its limit (levels, parts, whole triangles), an error
// that is no finite non-negative number, bytes cut short or left over: _read
// says false and the file is made again. An index is not checked against its
// part's vertices, which this file does not know: the hash ties it to the
// `.glb` it was made from, and the store holds the vertex counts.
#pragma once

#include <3d/models.h>

#include <base/arena.h>
#include <base/error.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	// The file has its own levels and was not simplified.
	bool own;
	// Levels including level 0: 1 when own or none was made.
	uint32_t level_count;
	// Level 0's baked parts; 0 when own.
	uint32_t part_count;
	// Each level's error in model metres; level 0's nought.
	float errors[VOE_3D_MODEL_LEVELS];
	// Level k's indices into level 0's part p, counted from the part's first
	// vertex; level 0's are unused, the bake's own indices being level 0.
	uint32_t *indices[VOE_3D_MODEL_LEVELS][VOE_3D_MODEL_PARTS];
	uint32_t index_counts[VOE_3D_MODEL_LEVELS][VOE_3D_MODEL_PARTS];
} voe_3d_model_levels;

// FNV-1a 64 of `size` bytes.
uint64_t voe_3d_model_levels_hash(const uint8_t *bytes, size_t size);

// Parses `glb`, bakes level 0, makes its levels and writes the file's bytes,
// returned with their count in `*size`, all on `arena`. The bytes are the
// return and not an out-parameter, since rule 6 allows no `**`. NULL with
// `error` set as the store's load would fail: MALFORMED or UNSUPPORTED as the
// reader says, UNSUPPORTED or REFUSED as the bake says.
[[nodiscard]] const uint8_t *
voe_3d_model_levels_make(voe_base_arena *arena, const uint8_t *glb,
			 size_t glb_size, size_t *size, voe_base_error *error);

// Reads a levels file made for `glb` into `out`, its indices copied onto
// `arena`. False when the file is stale, `out` then not to be read.
[[nodiscard]] bool voe_3d_model_levels_read(voe_base_arena *arena,
					    const uint8_t *bytes, size_t size,
					    const uint8_t *glb, size_t glb_size,
					    voe_3d_model_levels *out);
