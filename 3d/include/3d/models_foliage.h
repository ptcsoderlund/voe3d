// The model store's foliage (0442 points 1-3): a loaded landscape's kinds and
// copies, scattered, erased, put back and named through the store, and each
// copy's placed transform read by what draws them. Beside 3d/models.h, whose
// header is full; the arithmetic is 3d/foliage.h's.
//
//     uint64_t seed = 0x9e3779b97f4a7c15u;
//     voe_assets_landscape_copy added[VOE_3D_FOLIAGE_TRIES];
//     const uint32_t n = voe_3d_models_landscape_scatter(
//             models, path, &brush, hit.x, hit.z, seconds, &seed, added);
//     ... // the stroke's undo step keeps added[0..n)
//
// EVERY CALL IS NOTHING FOR A PATH THAT IS NO LOADED LANDSCAPE, and each marks
// the entry edited when it changes a copy or a kind, as a brush does.
//
// THE ROOM IS HELD PER LANDSCAPE: its entry owns VOE_ASSETS_LANDSCAPE_COPIES
// copies and as many placed transforms in its arena, about 4 MB, so a scatter
// never allocates in the middle of a stroke.
//
// PLACED TRANSFORMS ARE KEPT because a copy stores no height: where it stands
// is worked from the heights and the ground's normal, too dear to do for 65536
// copies every frame. They are placed at the load, with each copy a scatter or
// a put adds, and again over the heights a brush or a put changes
// (models.h's _landscape_brush and _landscape_put), so sculpting keeps them
// standing. An erase moves the last copy in, its transform with it.
//
// Constraints: a path is looked up by a walk over the store's entries; the
// copies' own constraints are 3d/foliage.h's.
#pragma once

#include <3d/foliage.h>
#include <3d/models.h>
#include <assets/landscape.h>
#include <scene/transform_component.h>

#include <stdbool.h>
#include <stdint.h>

// One scatter stamp of `brush` at (x, z) on `path`'s grid, as
// voe_3d_foliage_scatter, the added copies placed; how many.
uint32_t voe_3d_models_landscape_scatter(voe_3d_models *models,
					 const char *path,
					 const voe_3d_foliage_brush *brush,
					 float x, float z, float seconds,
					 uint64_t *seed,
					 voe_assets_landscape_copy *added);

// One erase stamp on `path`'s grid, as voe_3d_foliage_erase; how many.
uint32_t voe_3d_models_landscape_erase(voe_3d_models *models, const char *path,
				       const voe_3d_foliage_brush *brush,
				       float x, float z,
				       voe_assets_landscape_copy *removed,
				       uint32_t room);

// Undo's: `copies` added to or removed from `path`'s grid, as
// voe_3d_foliage_put.
void voe_3d_models_landscape_put_copies(voe_3d_models *models,
					const char *path,
					const voe_assets_landscape_copy *copies,
					uint32_t count, bool add);

// Kind `index` of `path`'s grid takes `model`'s path; `index` equal to its
// kind_count, below VOE_ASSETS_LANDSCAPE_KINDS, adds a kind. Asserts past
// that and on a path of VOE_ASSETS_LANDSCAPE_KIND_PATH bytes or more.
void voe_3d_models_landscape_kind_set(voe_3d_models *models, const char *path,
				      uint32_t index, const char *model);

// `entry`'s copies' placed transforms, `entry->landscape->copy_count` of them;
// NULL for an entry that is no loaded landscape of this store.
const voe_scene_transform *
voe_3d_models_landscape_placed(const voe_3d_models *models,
			       const voe_3d_model_entry *entry);
