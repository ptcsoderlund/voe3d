// A read model flattened into one set of arrays: every node's world matrix baked
// into its vertices, its normal matrix into its normals, and the primitives
// merged into one part per material. Internal to 3d: the model store's first
// step after `assets` has read the file.
//
//     voe_3d_model_bake bake;
//
//     if (!voe_3d_model_bake_create(arena, &model, &bake, &error))
//             ... // UNSUPPORTED or REFUSED, said on stderr
//     part = &bake.parts[0]; // bake.vertices + part->first_vertex, ...
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

// Bakes `model` into `out`, its arrays in `arena`. False with `error` set:
// UNSUPPORTED for too many materials or too deep a tree, REFUSED for more
// vertices or indices than a 32-bit count holds.
[[nodiscard]] bool voe_3d_model_bake_create(voe_base_arena *arena,
				     const voe_assets_model *model,
				     voe_3d_model_bake *out,
				     voe_base_error *error);
