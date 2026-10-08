// The model store's private shape, shared by models.c and models_landscape.c:
// the table, what each entry holds that its readers do not see, and the few
// helpers both files call. Internal to this folder.
//
// A LANDSCAPE'S STATIC CHUNK IDS LIVE IN `statics`, APART FROM ITS PARTS. A
// frame points a dirty chunk's part at a transient range that is gone at the
// next begin; the static id it replaced is still here to draw again, to
// destroy at the settle, and to free on a replace or a clear. `dirty` is one
// bit per chunk, cz·4 + cx.
#pragma once

#include <3d/models.h>
#include <base/arena.h>
#include <base/error.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// What an entry holds that its readers do not see: its arena, every texture
// and shading record its load made, and a landscape's static chunks and dirt.
typedef struct {
	voe_base_arena *memory;
	voe_render_texture *textures;
	uint32_t texture_count;
	voe_render_shading *shadings;
	uint32_t shading_count;
	voe_render_geometry statics[VOE_3D_MODEL_PARTS];
	uint16_t dirty;
} entry_held;

struct voe_3d_models {
	voe_3d_model_entry entries[VOE_3D_MODELS];
	entry_held held[VOE_3D_MODELS];
	uint32_t count;
	// The quad every picture's parts are drawn on, when `has_quad`.
	voe_render_geometry quad;
	bool has_quad;
	// The soft dot, found at "", when `has_dot`.
	voe_3d_model_entry dot;
	entry_held dot_held;
	bool has_dot;
	// The water's part on the quad, its record the store's, when `has_water`.
	voe_3d_model_part water;
	bool has_water;
};

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
