// The model store's private shape, shared by models.c, models_landscape.c and
// the draw files: the table, what each entry holds that its readers do not
// see, and the few helpers they call. Internal to this folder.
//
// A LANDSCAPE'S TEXTURE, PYRAMID AND DIRT ARE HELD, NOT SHOWN (0396 points 3
// and 5): its one part draws the store's shared grid, and what makes that grid
// its ground is here. The pyramid lives in the entry's arena; the dirty rect is
// the heights not yet written into the texture, empty when x0 ≥ x1.
//
// THE GRID IS 33 × 33 VERTICES at x, z in 0..1, normals up, uv = xz, y 0 but
// the last row's 1, so its box is a unit cube; the shader ignores y. Made at
// the first landscape load, given back by _clear.
#pragma once

#include <3d/landscape.h>
#include <3d/models.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

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
	// The rows one frame writes, gathered row-major for the write.
	float written[VOE_3D_LANDSCAPE_WRITE_TEXELS];
	// The soft dot, found at "", when `has_dot`.
	voe_3d_model_entry dot;
	entry_held dot_held;
	bool has_dot;
	// The water's part on the quad, its record the store's, when `has_water`.
	voe_3d_model_part water;
	bool has_water;
};

// What a draw needs of a loaded landscape beside its part.
typedef struct {
	voe_render_texture heights;
	const voe_3d_landscape_lod *lod;
	voe_render_geometry grid;
} voe_3d_models_terrain;

// `entry`'s heights texture, pyramid and the store's grid in `out`; false,
// `out` untouched, for an entry that is no loaded landscape of this store.
bool voe_3d_models_terrain_of(const voe_3d_models *models,
			      const voe_3d_model_entry *entry,
			      voe_3d_models_terrain *out);

// `path`'s index, or VOE_3D_MODELS when the store does not hold it.
uint32_t voe_3d_models_index(const voe_3d_models *models, const char *path);

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
