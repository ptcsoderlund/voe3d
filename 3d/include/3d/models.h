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
// A `.glb` HAS LEVELS (0425 points 3, 4, 6; 0426; 0432): own levels baked per
// level and joined to level 0's parts by material, the bound 2^-k; made ones,
// from a levels file, uploaded compacted to the vertices they use, the bound
// 0.002 × radius ÷ error (infinite for error nought); each ÷ the file's swap,
// 1 with no fresh file, then made non-increasing; freed with the parts.
//
// A FAILED ENTRY IS KEPT at its stamp, `loaded` false and no parts, so a broken
// file asked for each frame is not parsed each frame. A PATH LOADED AGAIN
// REPLACES ITSELF (ADR-0277 point 5): success swaps in new parts and frees the
// old; failure keeps the old at the new stamp. Ids change with each load.
//
// THE MEMORY IS THE STORE'S (rule 11's exception), given back by _destroy, which
// frees nothing on the card: a program keeping its device calls _clear. THE STORE
// IS CHANGED ONLY BETWEEN FRAMES: every upload and free waits for the card to go
// idle; the _frame calls alone run inside one, writing what exists.
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
// A `.landscape` IS ONE PART (0379 points 2, 6; 0396 points 3, 5): the shared
// grid wearing one lit opaque ground and its twin; the entry owns its grid's
// copy, a heights texture and a min/max pyramid; its shape is empty, the pick
// and bounds meeting the heights. ITS PAINT TOO (0402 points 1, 3, 4): layer
// paths and both planes, all layer 0 when given none, as two RGBA8 DATA
// textures FIELD-sampled (bilinear, one level, weights not a picture to mip);
// its own dirt, one budget for both.
//
// A `.material` IS AN ENTRY WITH NO SHAPE (0399 points 4, 5): one part with no
// geometry holding its record, its blended twin and its own maps, here because
// reload, rename, clear and the twin already live here. A map two materials
// name is uploaded twice.
#pragma once

#include <3d/landscape.h>
#include <3d/material_component.h>
#include <3d/shape_geometry.h>

#include <assets/landscape.h>
#include <assets/material.h>
#include <assets/model.h>
#include <assets/parts.h>
#include <base/arena.h>
#include <base/error.h>
#include <math/float3.h>
#include <render/device.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// How many files the store holds, and how many materials one file may use.
#define VOE_3D_MODELS 128
#define VOE_3D_MODEL_PARTS 16
// Levels one model may have, level 0 the file as it is (0425 point 1).
#define VOE_3D_MODEL_LEVELS 6

// The device room the store's models are given, which a program adds to its
// own capacities: 3074 geometries, four parts for each of the VOE_3D_MODELS
// files at each of VOE_3D_MODEL_LEVELS levels, and one geometry more each for
// the pictures' quad and the landscapes' shared grid; 3 M vertices and 9 M
// indices, level 0's 2 M and 6 M and the levels' under thirty-one
// thirty-seconds more, since they aim at a half, a quarter and so on down to a
// thirty-second (0425 point 6, 0433); 1024
// shading records, two for each of those parts (its own and its blended twin),
// and one more for the water. Each landscape also takes three textures: its
// heights and two paint planes.
#define VOE_3D_MODELS_VERTICES (3u << 20)
#define VOE_3D_MODELS_INDICES (9u << 20)
#define VOE_3D_MODELS_GEOMETRIES 3074
#define VOE_3D_MODELS_SHADINGS 1025

// Nodes one landscape draws in one pass at most (0396 point 4), which a
// program adds to its objects for each landscape and pass.
#define VOE_3D_LANDSCAPE_NODES 1024
// Landscape rows drawn a frame at most, by which a program multiplies
// VOE_3D_LANDSCAPE_NODES in its objects.
#define VOE_3D_LANDSCAPES_DRAWN 4
// Texels written a frame, every landscape together (0396 point 5), which a
// program adds to its heights_texels. Heights and paint share it (0402 point
// 4): a paint point costs one texel in each of its two planes.
#define VOE_3D_LANDSCAPE_WRITE_TEXELS (512u * 512u)

// One material's worth of a model, in model space.
typedef struct {
	voe_render_geometry geometry;
	voe_3d_material material;
	// The record the part is drawn with while its row fades.
	voe_render_shading faded;
	// The glTF material the part was merged from; VOE_ASSETS_MODEL_NONE
	// for glTF's default and for a picture's, landscape's or material's.
	uint32_t source;
	// The `.material` path it wears, "" for the file's own.
	char wears[VOE_ASSETS_MATERIAL_PATH];
} voe_3d_model_part;

// One level of a model: the geometry each level-0 part draws at it, a zeroed id
// (generation 0 is never handed out) where the level lacks the part, and the
// screen size `s` it is drawn under (0425 point 3), ÷ the file's swap (0432).
typedef struct {
	voe_render_geometry geometries[VOE_3D_MODEL_PARTS];
	float bound;
} voe_3d_model_level_parts;

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
	// A loaded landscape's grid, owned by the entry with its layers and
	// paint, never NULL paint; NULL for any other.
	voe_assets_landscape *landscape;
	// Changed since loaded or _landscape_saved.
	bool edited;
	// A loaded `.material`: one part with no geometry, no shape.
	bool material;
	// Level 0 mirrors the parts' geometries under an infinite bound, so a
	// draw reads levels[k] for any k below `level_count`, which is 1 for
	// every entry without levels: pictures, landscapes and materials too.
	voe_3d_model_level_parts levels[VOE_3D_MODEL_LEVELS];
	uint32_t level_count;
	// The levels are the file's own `_LOD<n>` nodes, not made ones.
	bool levels_own;
	// The model-space bounding sphere: its box's centre and the farthest
	// level-0 vertex from it; nought for a non-`.glb`.
	voe_math_float3 centre;
	float radius;
} voe_3d_model_entry;

// One map's file bytes, read by the caller; a size of 0 is no map.
typedef struct {
	const uint8_t *bytes;
	size_t size;
} voe_3d_material_map;

// A material's four maps, as its file names them.
typedef struct {
	voe_3d_material_map colour;
	voe_3d_material_map normal;
	voe_3d_material_map orm;
	voe_3d_material_map emission;
} voe_3d_material_maps;

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
// The same as _load_levelled with NULL levels.
[[nodiscard]] bool voe_3d_models_load(voe_3d_models *models,
				      voe_render_device *device,
				      const char *path, uint64_t stamp,
				      const uint8_t *bytes, size_t size,
				      voe_base_error *error);

// _load, a `.glb` with levels: its own when its nodes name them, else the made
// ones `levels` holds, a levels file (3d/model_levels.h) read against `bytes`.
// NULL `levels`, a stale file or one naming an index past its part loads level
// 0 alone, never a failure; REFUSED when the device has no room for a level.
[[nodiscard]] bool voe_3d_models_load_levelled(
	voe_3d_models *models, voe_render_device *device, const char *path,
	uint64_t stamp, const uint8_t *bytes, size_t size, const uint8_t *levels,
	size_t levels_size, voe_base_error *error);

// `landscape`'s grid copied into `path`'s entry with its layers and paint (all
// layer 0 when NULL), uploaded as its heights texture and two paint textures,
// RGBA8 DATA FIELD of (cells + 1)² each, and built into its pyramid; one part,
// the store's grid wearing one ground material and its blended twin; its shape
// empty. Replaces and fails as _load does.
[[nodiscard]] bool voe_3d_models_load_landscape(
	voe_3d_models *models, voe_render_device *device, const char *path,
	uint64_t stamp, const voe_assets_landscape *landscape,
	voe_base_error *error);

// `material` as `path`'s entry: each map with bytes decoded as a picture by its
// path's extension and uploaded mipped, colour and emission as COLOUR, normal
// and ORM as DATA, the ORM map's one texture in the metal-roughness and the
// occlusion slots both (0400), the emission map in the emissive slot; colour,
// metal, roughness, repeat and shader from the values, `cutout` > 0 as CUTOUT
// at that cutoff else OPAQUE, `emission` × `emission_strength` as emissive, and
// `double_sided` (0420 point 2); one part with no geometry and its blended
// twin. Replaces and fails as _load does; MALFORMED when a map will not decode,
// REFUSED when the device or the store has no room.
[[nodiscard]] bool voe_3d_models_load_material(
	voe_3d_models *models, voe_render_device *device, const char *path,
	uint64_t stamp, const voe_assets_material_file *material,
	voe_3d_material_maps maps, voe_base_error *error);

// `material`'s colour, metal, roughness, repeat, shader, cutout, emission ×
// strength and double_sided taken into `path`'s part as _load_material takes
// them, and its two records marked to write; its maps ignored, a new map being
// a load. Nothing for a path that is no loaded material.
void voe_3d_models_material_set(voe_3d_models *models, const char *path,
				const voe_assets_material_file *material);

// Each part of `path`'s loaded model takes `parts->materials[source]` into
// `wears`, "" for a `source` of NONE or at or past `parts->count`; NULL
// `parts` empties every `wears`. Nothing for a path that is no loaded model.
// So a part wears what its model's parts file names for its glTF `source`
// (0404, 0420); a load leaves `wears` empty, the file's own.
void voe_3d_models_parts_set(voe_3d_models *models, const char *path,
			     const voe_assets_parts *parts);

// Inside a frame, before its first pass: every marked material's record and
// twin rewritten (voe_render_shading_write), the marks cleared, so a drag that
// _material_set marks needs no upload.
void voe_3d_models_material_frame(voe_3d_models *models,
				  voe_render_device *device);

// One stamp of `brush` at (x, z) on `path`'s grid (3d/landscape.h): the
// pyramid updated over it, the dirty rect grown by it and the entry edited; a
// PAINT brush stamps the paint and grows the paint's dirt instead. The rect it
// changed; empty for a path that is no loaded landscape.
voe_3d_landscape_rect
voe_3d_models_landscape_brush(voe_3d_models *models, const char *path,
			      const voe_3d_brush *brush, float x, float z,
			      float seconds, voe_base_arena *scratch);

// `rect`'s heights, row-major, written into `path`'s grid; the pyramid, dirt
// and edited as a brush. Nothing for a path that is no loaded landscape.
void voe_3d_models_landscape_put(voe_3d_models *models, const char *path,
				 voe_3d_landscape_rect rect,
				 const float *values);

// `rect`'s points written into both paint planes of `path`'s grid, `values`
// being plane 0's rect of RGBA8 row-major then plane 1's; the paint's dirt
// grown and the entry edited. Nothing for a path that is no loaded landscape.
void voe_3d_models_landscape_put_paint(voe_3d_models *models, const char *path,
				       voe_3d_landscape_rect rect,
				       const uint8_t *values);

// Layer `index` of `path`'s grid takes `material`'s path; `index` equal to its
// layer_count, below VOE_ASSETS_LANDSCAPE_LAYERS, adds a layer whose weights
// are nought. The entry edited. Asserts past that; nothing for a path that is
// no loaded landscape.
void voe_3d_models_landscape_layer_set(voe_3d_models *models, const char *path,
				       uint32_t index, const char *material);

// Inside a frame, before its first pass: each entry's dirty heights and then
// its dirty paint written into their textures, whole rows while
// VOE_3D_LANDSCAPE_WRITE_TEXELS last, the rest kept dirty for the next frame.
// A refused write keeps its rows dirty and says so on stderr. A budget a frame,
// inside one: an upload between frames would stutter a drag.
void voe_3d_models_landscape_frame(voe_3d_models *models,
				   voe_render_device *device);

// Clears `path`'s `edited`, once its grid is written to its file.
void voe_3d_models_landscape_saved(voe_3d_models *models, const char *path);

// Every entry whose path is `from` or begins `from/` takes `to` in its place,
// and so does every landscape's layer path; one that would not fit is kept.
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
