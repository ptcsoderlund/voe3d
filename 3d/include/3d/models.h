// The model store: every model file a world's things name, loaded once and
// keyed by its project-relative path (ADR-0277 point 2). A program fills it; what
// draws, picks and outlines walks the model rows through it.
//
//     voe_3d_models *models = voe_3d_models_new();
//     if (!voe_3d_models_load(models, device, "Assets/hull.glb", stamp,
//                             bytes, size, &error))
//             ... // kept as a failed entry; say so once
//     const voe_3d_model_entry *hull = voe_3d_models_find(models, path);
//     voe_3d_models_clear(models, device);   // a different project
//     voe_3d_models_destroy(models);
//
// A STORE AND NOT A COMPONENT, because a file is uploaded once however many
// things wear it. A row names a path; the store holds the one upload behind it,
// so a hundred copies of a hull cost one hull on the card, and replacing that
// upload later changes every copy at once.
//
// NODES ARE BAKED AND PARTS MERGED. Each node's world matrix is applied to its
// vertices, its normal matrix to its normals, and every primitive wearing one
// material joins one part: one geometry and one material, at most
// VOE_3D_MODEL_PARTS. A thing then draws a model as a handful of draws with its
// own transform, and a Blender export with baked textures is one draw. What is
// lost is the file's tree, which nothing here moves on its own.
//
// A FAILED ENTRY IS KEPT, with the stamp it failed at, so that a caller asking
// each frame for a path it lacks does not read and parse a broken file each
// frame. The entry's `loaded` is false and it has no parts.
//
// THE MEMORY IS THE STORE'S (rule 11's long-lived exception): the store and each
// entry own their memory, given back by _destroy. _destroy frees nothing on the
// card; the device's own destroy does that, so a program that keeps its device
// calls _clear first.
//
// A PATH LOADED AGAIN REPLACES ITSELF (ADR-0277 point 5): success swaps in new
// parts and shape and frees the old ones on the card; failure keeps the old
// parts and `loaded` as they were at the new stamp. So a failed entry loaded
// again with good bytes becomes loaded, and a part's ids change with each load.
//
// THE STORE IS CHANGED ONLY BETWEEN FRAMES, never while one is recorded: every
// upload and every free waits for the card to go idle.
#pragma once

#include <3d/material_component.h>
#include <3d/shape_geometry.h>

#include <base/error.h>
#include <render/device.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// How many files the store holds, and how many materials one file may use.
#define VOE_3D_MODELS 128
#define VOE_3D_MODEL_PARTS 16

// The device room the store's models are given, which a program adds to its
// own capacities: 2 M vertices, 6 M indices, and 512 geometries and shading
// records, four parts for each of the VOE_3D_MODELS files.
#define VOE_3D_MODELS_VERTICES (1u << 21)
#define VOE_3D_MODELS_INDICES (3u << 21)
#define VOE_3D_MODELS_GEOMETRIES 512
#define VOE_3D_MODELS_SHADINGS 512

// One material's worth of a model, in model space.
typedef struct {
	voe_render_geometry geometry;
	voe_3d_material material;
} voe_3d_model_part;

// One file: loaded with its parts, or failed with none.
typedef struct {
	const char *path;
	uint64_t stamp;
	bool loaded;
	voe_3d_model_part parts[VOE_3D_MODEL_PARTS];
	uint32_t part_count;
	// The whole model on the CPU, model space, edges welded, for the ray
	// and the outline (3d/shape_geometry.h). Empty for a failed entry.
	voe_3d_shape_geometry shape;
} voe_3d_model_entry;

typedef struct voe_3d_models voe_3d_models;

voe_3d_models *voe_3d_models_new(void);
void voe_3d_models_destroy(voe_3d_models *models);
// Frees every entry's parts on the card and its memory; the store is empty
// and takes loads again.
void voe_3d_models_clear(voe_3d_models *models, voe_render_device *device);

// Reads `bytes` as a `.glb` and uploads it as `path`'s entry. False, with the
// entry kept as failed at `stamp` and `error` set: MALFORMED for bytes that are
// not a model, UNSUPPORTED for more than VOE_3D_MODEL_PARTS materials or nodes
// nested past VOE_3D_IMPORT_MAX_DEPTH, REFUSED when the device or the store has
// no room. What a failed load had put on the card is given back. A path already
// held is replaced when this load succeeds and kept, at `stamp`, when it fails.
[[nodiscard]] bool voe_3d_models_load(voe_3d_models *models,
				      voe_render_device *device,
				      const char *path, uint64_t stamp,
				      const uint8_t *bytes, size_t size,
				      voe_base_error *error);

// Keeps `path` as a failed entry at `stamp`: a file that could not be read. A
// path already held keeps what it had, at `stamp`.
void voe_3d_models_fail(voe_3d_models *models, const char *path,
			uint64_t stamp);

// `path`'s entry, loaded or failed; NULL when it was never asked for.
const voe_3d_model_entry *voe_3d_models_find(const voe_3d_models *models,
					     const char *path);

uint32_t voe_3d_models_count(const voe_3d_models *models);
const voe_3d_model_entry *voe_3d_models_at(const voe_3d_models *models,
					   uint32_t index);
