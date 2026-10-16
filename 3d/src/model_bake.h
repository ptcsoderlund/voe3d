// A read model flattened into one set of arrays: every node's world matrix baked
// into its vertices, its normal matrix into its normals, the file's vertex
// colours carried as each vertex's colour complement (white without them), and
// the primitives merged into one part per material, one of the file's own levels
// at a time. Internal to 3d: the model store's first step after `assets` has
// read the file.
//
//     voe_3d_model_bake bake;
//     uint32_t levels = voe_3d_model_bake_levels(&model); // 1 without any
//
//     if (!voe_3d_model_bake_create(arena, &model, 0, &bake, &error))
//             ... // UNSUPPORTED or REFUSED, said on stderr
//     part = &bake.parts[0]; // bake.vertices + part->first_vertex, ...
//
// A NODE'S LEVEL IS TOLD BY ITS NAME (ADR-0425 point 4), as Blender exports
// them: a name ending `_LOD<n>`, LOD in any case and n all digits, makes the
// node and its subtree level n; a name without the suffix is its parent's
// level, a root's level 0. A node of n above 5 (VOE_3D_MODEL_LEVELS less one)
// is left out with its subtree.
// Only the asked level's primitives are baked. Matching a level's parts to
// level 0's by material is the store's, not the bake's.
//
// THE TREE IS WALKED WITH AN EXPLICIT STACK (rule 14), as import.c walks it: as
// long as the file has nodes, since `assets` has checked no node is claimed as a
// child twice, and refused past VOE_3D_IMPORT_MAX_DEPTH.
//
// A PART IS A MATERIAL AS THE PRIMITIVES NAME IT: a glTF material index, or
// VOE_ASSETS_MODEL_NONE for glTF's default, which a primitive naming none and one
// naming an index past the file's both wear (model_upload.h). Parts come in the
// order their first primitive is met; more than VOE_3D_MODEL_PARTS is refused.
//
// A PART'S INDICES COUNT FROM ITS OWN FIRST VERTEX, so a part is handed to
// `render` as it stands. A primitive with no vertices or no indices draws
// nothing and is left out. Everything is pushed into the caller's arena.
#pragma once

#include <3d/models.h>

#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint32_t material;
	uint32_t first_vertex;
	uint32_t vertex_count;
	uint32_t first_index;
	uint32_t index_count;
} voe_3d_model_bake_part;

typedef struct {
	voe_render_vertex *vertices;
	uint32_t vertex_count;
	uint32_t *indices;
	uint32_t index_count;
	voe_3d_model_bake_part parts[VOE_3D_MODEL_PARTS];
	uint32_t part_count;
} voe_3d_model_bake;

// Bakes `model`'s nodes of `level` into `out`, its arrays in `arena`, parts in
// the order met within that level. False with `error` set: UNSUPPORTED for too
// many materials or too deep a tree, REFUSED for more vertices or indices than
// a 32-bit count holds.
[[nodiscard]] bool voe_3d_model_bake_create(voe_base_arena *arena,
				     const voe_assets_model *model,
				     uint32_t level, voe_3d_model_bake *out,
				     voe_base_error *error);

// One more than the highest own level any node of `model` carries: 1 for a
// file with none.
uint32_t voe_3d_model_bake_levels(const voe_assets_model *model);
