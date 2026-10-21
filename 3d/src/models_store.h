// The model store's private shape, shared by models.c, models_landscape.c and
// the draw files: the table, what each entry holds that its readers do not
// see, and the few helpers they call. Internal to this folder.
//
// A LANDSCAPE'S TEXTURE, PYRAMID AND DIRT ARE HELD, NOT SHOWN (0396 points 3
// and 5, 0402 point 4): its one part draws the store's shared grid, and what
// makes that grid its ground is here. The pyramid lives in the entry's arena;
// its two dirty rects are the heights and the paint not yet written into their
// textures, empty when x0 ≥ x1. The paint textures sit in `textures` too,
// freed with the rest. The draw files ask voe_3d_models_terrain_of for it.
// Its copies' placed transforms (0442 point 2) live in the arena too, room for
// the cap, in step with the grid's copies.
//
// THE STORE COUNTS ITS CHANGES (0451 point 3): `changes` rises whenever an
// entry is put loaded, replaced or cleared, a foliage copy or kind changes, or
// copies are placed again over changed heights; it only rises, and wraps. The
// bounce hands it to every begin as `shade`, so a change the stale spheres do
// not mark, a forest erased, still relights the probes (draw_bounce.c).
//
// A MATERIAL'S WRITE MARK is its records changed since last written. The
// helpers below are models.c's, lent to the other store files.
//
// THE GRID IS 33 × 33 VERTICES at x, z in 0..1, normals up, uv = xz, y 0 but
// the last row's 1, so its box is a unit cube; the shader ignores y. Made at
// the first landscape load, given back by _clear.
//
// A `.glb`'S LEVELS ARE models_levels.c's: models.c calls them after the parts
// and frees them before the parts wherever it frees those, made levels walking
// level 0's vertices (0444). What they upload model_prepare.c baked or read
// into the prepared struct below, which those three files share.
#pragma once

#include "model_bake.h"

#include <3d/landscape.h>
#include <3d/model_levels.h>
#include <3d/model_prepare.h>
#include <3d/models.h>
#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>
#include <base/workers.h>
#include <render/device.h>
#include <scene/transform_component.h>

#include <stdbool.h>
#include <stdint.h>

// What an entry holds that its readers do not see: its arena, every texture
// and shading record its load made, and a landscape's heights and dirt.
typedef struct {
	voe_base_arena *memory;
	voe_render_texture *textures;
	uint32_t texture_count;
	voe_render_shading *shadings;
	uint32_t shading_count;
	// A loaded landscape's heights texture, pyramid on `memory`, and the
	// heights changed since last written.
	voe_render_texture heights;
	voe_3d_landscape_lod lod;
	voe_3d_landscape_rect dirty;
	// Its two paint textures, planes 0 and 1, also listed in `textures`, and
	// the points whose paint changed since last written.
	voe_render_texture paint[2];
	voe_3d_landscape_rect paint_dirty;
	// Each copy's transform as 3d/foliage.h places it, in step with the
	// grid's copies, room for VOE_ASSETS_LANDSCAPE_COPIES.
	voe_scene_transform *placed;
	// A loaded material's records changed since last written; its maps are
	// `textures` above, freed with the entry.
	bool write;
} entry_held;

struct voe_3d_models {
	voe_3d_model_entry entries[VOE_3D_MODELS];
	entry_held held[VOE_3D_MODELS];
	uint32_t count;
	// The quad every picture's parts are drawn on, when `has_quad`.
	voe_render_geometry quad;
	bool has_quad;
	// The grid every landscape's part is drawn on, when `has_grid`.
	voe_render_geometry grid;
	bool has_grid;
	// The rows one frame writes, gathered row-major for the write: heights
	// as floats, paint as bytes.
	float written[VOE_3D_LANDSCAPE_WRITE_TEXELS];
	// The soft dot, found at "", when `has_dot`.
	voe_3d_model_entry dot;
	entry_held dot_held;
	bool has_dot;
	// The water's part on the quad, its record the store's, when `has_water`.
	voe_3d_model_part water;
	bool has_water;
	// The program's pool its loaders run on, NULL until set; never run,
	// destroyed or cleared by the store.
	voe_base_workers *workers;
	// Changes to what the bounce's sun maps draw, as the header says.
	uint32_t changes;
};

// What a draw needs of a loaded landscape beside its part.
typedef struct {
	voe_render_texture heights;
	const voe_3d_landscape_lod *lod;
	voe_render_geometry grid;
	// Paint planes 0 and 1, layers 0-3 and 4-7.
	voe_render_texture paint[2];
} voe_3d_models_terrain;

// `entry`'s heights texture, pyramid, paint textures and the store's grid in
// `out`; false, `out` untouched, for an entry that is no loaded landscape of
// this store.
bool voe_3d_models_terrain_of(const voe_3d_models *models,
			      const voe_3d_model_entry *entry,
			      voe_3d_models_terrain *out);

// `path`'s index, or VOE_3D_MODELS when the store does not hold it.
uint32_t voe_3d_models_index(const voe_3d_models *models, const char *path);

// `path`'s index when it is a loaded landscape, else VOE_3D_MODELS.
uint32_t voe_3d_models_landscape_index(const voe_3d_models *models,
				       const char *path);

// False, `error` REFUSED and a line on stderr, when the store is full and does
// not hold `path`.
bool voe_3d_models_room(const voe_3d_models *models, const char *path,
			voe_base_error *error);

// A failed entry for `path` in a new arena of its own, not yet in the store.
void voe_3d_models_entry_new(const char *path, uint64_t stamp,
			     voe_3d_model_entry *entry, entry_held *held);

// Puts `entry` into the store, replacing or keeping as models.h says; what is
// not kept is given back.
bool voe_3d_models_keep(voe_3d_models *models, voe_render_device *device,
			const voe_3d_model_entry *entry,
			const entry_held *held);

// `part`'s blended twin as its `faded`, unless it is BLENDED already; false,
// `error` REFUSED, when the device has no room.
bool voe_3d_models_twin(voe_render_device *device, voe_3d_model_part *part,
			voe_base_error *error);

// A `.glb` prepared (3d/model_prepare.h), everything on the prepare's arena.
struct voe_3d_model_prepared {
	// The read model, its pictures decoded by the put.
	const voe_assets_model *model;
	// Level 0, and the CPU shape built from it.
	voe_3d_model_bake bake;
	voe_3d_shape_geometry shape;
	// The bounding sphere: the box's centre and the farthest level-0 vertex
	// from it; nought for a bake with no vertices.
	voe_math_float3 centre;
	float radius;
	// The file's own levels: `own` when its nodes name any, levels 1 to
	// `own_count` − 1 baked, each matching some level-0 part by material.
	bool own;
	voe_3d_model_bake own_levels[VOE_3D_MODEL_LEVELS];
	uint32_t own_count;
	// The levels file, read fresh: an own file's errors and swaps when
	// `measured`, made levels' indices fitting the bake when `made`.
	voe_3d_model_levels read;
	bool measured;
	bool made;
};

// The level-0 part `bake` merged from glTF material `material`;
// VOE_3D_MODEL_PARTS when none was.
uint32_t voe_3d_models_part_of(const voe_3d_model_bake *bake,
			       uint32_t material);

// `entry`'s levels after its level-0 parts, as `prepared` baked or read them;
// level 0 alone when it has neither. False, `error` set as the device says,
// what it made left in `entry` for _levels_free.
bool voe_3d_models_levels_upload(voe_render_device *device,
				 const voe_3d_model_prepared *prepared,
				 voe_3d_model_entry *entry,
				 voe_base_error *error);

// The geometries of `entry`'s levels 1 and on given back; level 0's are its
// parts'. Touches no device when there are none.
void voe_3d_models_levels_free(voe_render_device *device,
			       const voe_3d_model_entry *entry);

// levels[0] made the parts' own geometries.
void voe_3d_models_levels_mirror(voe_3d_model_entry *entry);
