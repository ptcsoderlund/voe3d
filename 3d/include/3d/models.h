// The model store: every model file a world's things name, loaded once and
// keyed by its project-relative path (ADR-0277 point 2). A program fills it; what
// draws, picks and outlines walks the model rows through it.
//
//     voe_3d_models *models = voe_3d_models_new();
//     if (!voe_3d_models_load(models, device, path, stamp, bytes, size, &error))
//             ... // kept as a failed entry; say so once
//     const voe_3d_model_entry *hull = voe_3d_models_find(models, path);
//     voe_3d_models_clear(models, device);   // a different project
//     voe_3d_models_destroy(models);
//
// A STORE AND NOT A COMPONENT: a file is uploaded once however many things wear
// it, and replacing that upload changes every copy at once.
//
// NODES ARE BAKED AND PARTS MERGED: each node's matrices applied to its vertices
// and normals, every primitive of one material joined into one part, at most
// VOE_3D_MODEL_PARTS, so a model is a handful of draws. The file's tree is lost.
//
// A FAILED ENTRY IS KEPT at the stamp it failed at, `loaded` false and no parts,
// so a caller asking each frame for a broken file does not parse it each frame.
//
// THE MEMORY IS THE STORE'S (rule 11's long-lived exception), given back by
// _destroy, which frees nothing on the card: a program keeping its device calls
// _clear first.
//
// A PATH LOADED AGAIN REPLACES ITSELF (ADR-0277 point 5): success swaps in new
// parts and frees the old; failure keeps the old at the new stamp. A part's ids
// change with each load.
//
// THE STORE IS CHANGED ONLY BETWEEN FRAMES: every upload and free waits for the
// card to go idle. _landscape_frame alone runs inside one, and uploads nothing.
//
// PICTURES SHARE THE STORE (ADR-0298 point 5): a `.png`, `.jpg` or `.jpeg` path
// has two BLENDED parts on the store's one quad, 0 lit and 1 unlit, the glow,
// since `render` has no additive blend. Its shape is empty.
//
// THE SOFT DOT AND THE WATER ARE HELD APART (ADR-0305 point 6), made in code,
// never re-read, not in _count or _at: the dot found at "", the water read by
// _water, one BLENDED `water` record on the quad. _clear frees both.
//
// EACH PART NOT ALREADY BLENDED HAS A TWIN (ADR-0336 point 2), its material
// BLENDED, uploaded at load and freed with it; otherwise `faded` is `shading`.
//
// A `.landscape` IS 16 CHUNK PARTS (0379 points 2, 4, 6), its grid owned by the
// entry, all wearing one lit opaque ground and one twin, so 085 paints one
// record. Its shape is empty: the pick and the bounds meet the heights, not
// 2 × 512² triangles. A brush or _put marks chunks dirty and the entry edited;
// _landscape_frame draws them transient, since a static upload waits for the
// card and would stutter a drag; _landscape_settle makes them static again
// between frames. The static ids are kept apart from the parts, so a part
// pointed at a frame's transient never loses the one to draw or destroy.
#pragma once

#include <3d/landscape.h>
#include <3d/material_component.h>
#include <3d/shape_geometry.h>

#include <assets/landscape.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// How many files the store holds, and how many materials one file may use.
#define VOE_3D_MODELS 128
#define VOE_3D_MODEL_PARTS 16

// The device room the store's models are given, which a program adds to its
// own capacities: 2 M vertices, 6 M indices, 512 geometries, four parts for each
// of the VOE_3D_MODELS files, and one geometry more for the pictures' quad; 1024
// shading records, two for each of those parts (its own and its blended twin),
// and one more for the water.
#define VOE_3D_MODELS_VERTICES (1u << 21)
#define VOE_3D_MODELS_INDICES (3u << 21)
#define VOE_3D_MODELS_GEOMETRIES 513
#define VOE_3D_MODELS_SHADINGS 1025

// One whole 512-cell landscape's 16 chunks drawn transient in one frame, which
// a program adds to its transient capacities: 129² vertices, 128² · 6 indices
// and one range each.
#define VOE_3D_LANDSCAPE_TRANSIENT_VERTICES (16u * 129u * 129u)
#define VOE_3D_LANDSCAPE_TRANSIENT_INDICES (16u * 128u * 128u * 6u)
#define VOE_3D_LANDSCAPE_TRANSIENT_RANGES 16u

// One material's worth of a model, in model space.
typedef struct {
	voe_render_geometry geometry;
	voe_3d_material material;
	// The record the part is drawn with while its row fades.
	voe_render_shading faded;
} voe_3d_model_part;

// One file: loaded with its parts, or failed with none.
typedef struct {
	const char *path;
	uint64_t stamp;
	bool loaded;
	// A picture: its parts are 0 lit and 1 glow on the store's quad.
	bool picture;
	voe_3d_model_part parts[VOE_3D_MODEL_PARTS];
	uint32_t part_count;
	// The whole model on the CPU, model space, edges welded, for the ray
	// and the outline (3d/shape_geometry.h). Empty for a failed entry.
	voe_3d_shape_geometry shape;
	// A loaded landscape's grid, owned by the entry; NULL for any other.
	voe_assets_landscape *landscape;
	// Changed since loaded or _landscape_saved.
	bool edited;
} voe_3d_model_entry;

typedef struct voe_3d_models voe_3d_models;

voe_3d_models *voe_3d_models_new(void);
void voe_3d_models_destroy(voe_3d_models *models);
// Frees every entry's parts on the card and its memory; the store is empty
// and takes loads again.
void voe_3d_models_clear(voe_3d_models *models, voe_render_device *device);

// Reads `bytes` as a picture when `path` ends `.png`, `.jpg` or `.jpeg` in any
// case, as a landscape's text when it ends `.landscape`, else as a `.glb`, and
// uploads it as `path`'s entry. False, with the
// entry kept as failed at `stamp` and `error` set: for a picture, MALFORMED or
// UNSUPPORTED as the decoder says, for a landscape as its reader says;
// MALFORMED for bytes that are
// not a model, UNSUPPORTED for more than VOE_3D_MODEL_PARTS materials or nodes
// nested past VOE_3D_IMPORT_MAX_DEPTH, REFUSED when the device or the store has
// no room. What a failed load had put on the card is given back. A path already
// held is replaced when this load succeeds and kept, at `stamp`, when it fails.
[[nodiscard]] bool voe_3d_models_load(voe_3d_models *models,
				      voe_render_device *device,
				      const char *path, uint64_t stamp,
				      const uint8_t *bytes, size_t size,
				      voe_base_error *error);

// `landscape`'s grid copied into `path`'s entry and uploaded as one part per
// chunk, all wearing one ground material and one blended twin; its shape
// empty. Replaces and fails as _load does.
[[nodiscard]] bool voe_3d_models_load_landscape(
	voe_3d_models *models, voe_render_device *device, const char *path,
	uint64_t stamp, const voe_assets_landscape *landscape,
	voe_base_error *error);

// One stamp of `brush` at (x, z) on `path`'s grid (3d/landscape.h): the
// chunks it touched marked dirty and the entry edited. The height rect it
// changed; empty for a path that is no loaded landscape.
voe_3d_landscape_rect
voe_3d_models_landscape_brush(voe_3d_models *models, const char *path,
			      const voe_3d_brush *brush, float x, float z,
			      float seconds, voe_base_arena *scratch);

// `rect`'s heights, row-major, written into `path`'s grid; dirty and edited
// as a brush. Nothing for a path that is no loaded landscape.
void voe_3d_models_landscape_put(voe_3d_models *models, const char *path,
				 voe_3d_landscape_rect rect,
				 const float *values);

// After the frame's begin: each dirty chunk built on `scratch`, which is
// rewound, and made transient, its part drawing that this frame. A refused
// one keeps its static geometry and says so on stderr.
void voe_3d_models_landscape_frame(voe_3d_models *models,
				   voe_render_device *device,
				   voe_base_arena *scratch);

// Between frames: each dirty chunk uploaded static again, its old static
// destroyed, its dirt cleared. True at once with nothing dirty; false, `error`
// REFUSED and the chunk still dirty, when the device has no room.
[[nodiscard]] bool voe_3d_models_landscape_settle(voe_3d_models *models,
						  voe_render_device *device,
						  voe_base_arena *scratch,
						  voe_base_error *error);

// Clears `path`'s `edited`, once its grid is written to its file.
void voe_3d_models_landscape_saved(voe_3d_models *models, const char *path);

// Every entry whose path is `from` or begins `from/` takes `to` in its place.
void voe_3d_models_rename(voe_3d_models *models, const char *from,
			  const char *to);

// Uploads the built-in soft dot as a picture entry apart from the others: not
// in _count or _at, found by _find at "", freed by _clear. True at once when it
// is already there; false, `error` set, when the device has no room.
[[nodiscard]] bool voe_3d_models_load_dot(voe_3d_models *models,
					  voe_render_device *device,
					  voe_base_error *error);

// Makes the store's quad if not yet made and one shading record for water:
// BLENDED, `water` set, white, metallic 0, roughness 0.05. True at once when it
// is already there; false, `error` set, when the device has no room.
[[nodiscard]] bool voe_3d_models_load_water(voe_3d_models *models,
					    voe_render_device *device,
					    voe_base_error *error);

// The water's part, the quad and its record; NULL until _load_water.
const voe_3d_model_part *voe_3d_models_water(const voe_3d_models *models);

// Keeps `path` as a failed entry at `stamp`: a file that could not be read. A
// path already held keeps what it had, at `stamp`.
void voe_3d_models_fail(voe_3d_models *models, const char *path,
			uint64_t stamp);

// `path`'s entry, loaded or failed; NULL when it was never asked for. The empty
// path is the soft dot, NULL until _load_dot.
const voe_3d_model_entry *voe_3d_models_find(const voe_3d_models *models,
					     const char *path);

uint32_t voe_3d_models_count(const voe_3d_models *models);
const voe_3d_model_entry *voe_3d_models_at(const voe_3d_models *models,
					   uint32_t index);
