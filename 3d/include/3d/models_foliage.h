// The model store's foliage (0442 points 1-3, 0476, 0520): a loaded
// landscape's groups, their glbs' looks and what each layer grows, its copies
// scattered, erased, put back and named through the store, and each copy's
// placed transform and the grown copies read by what draws them. Beside
// 3d/models.h, whose header is full; the arithmetic is 3d/foliage.h's and 3d/foliage_grow.h's.
//
//     uint64_t seed = 0x9e3779b97f4a7c15u;
//     voe_assets_landscape_copy added[VOE_3D_FOLIAGE_TRIES];
//     const uint32_t n = voe_3d_models_landscape_scatter(
//             models, path, &brush, hit.x, hit.z, seconds, &seed, added);
//     ... // the stroke's undo step keeps added[0..n)
//
// EVERY CALL IS NOTHING FOR A PATH THAT IS NO LOADED LANDSCAPE, and each marks
// the entry edited when it changes a copy, a group or glb, a look or a growth,
// as a brush does.
//
// THE ROOM IS HELD PER LANDSCAPE: its entry owns VOE_ASSETS_LANDSCAPE_COPIES
// copies and VOE_3D_FOLIAGE_GROWN grown ones, each with a placed transform, in
// its arena, about 8 MB, so a scatter or a remake never allocates.
//
// GROWN COPIES ARE THE MATERIAL'S (0473 point 4): scatter, erase and the copy
// puts touch only the file's copies. The grown are made at the load and
// remade from the paint over what a stamp or put changes, and for one group
// over the grid when a layer's growth of it, a glb added, or a glb's spreads,
// size or share change.
// They are not saved (0476 point 3): made from the paint they follow it with
// no file lines, and the game makes the same ones from the same file.
//
// PLACED TRANSFORMS ARE KEPT because a copy stores no height: where it stands
// is worked from the heights and the ground's normal, too dear to do for 65536
// copies every frame. They are placed at the load, with each copy a scatter or
// a put adds, and again over the heights a brush or a put changes
// (models.h's _landscape_brush and _landscape_put), so sculpting keeps them
// standing, and a group's copies again when a glb's base size or share
// changes or a glb is added: each copy's glb is picked live (0521 point 3), so
// those move which glb a copy wears. An erase moves the last copy in, its
// transform with it.
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

// Glb `glb` of group `group` of `path`'s grid takes `model`'s path: `group`
// equal to its group_count adds a group of that one glb (`glb` 0), as
// voe_assets_landscape_group_add, which no layer grows; `glb` equal to the
// group's glb_count adds a glb at the default look, as _glb_add, placing the
// group's copies again and remaking its grown; else the path is replaced, its
// look kept, as _glb_set. False, nothing changed, when assets refuses or the
// path is no loaded landscape; else the entry is edited and `changes` raised.
// Asserts past the next group or glb and on an empty path or one of
// VOE_ASSETS_LANDSCAPE_GLB_PATH bytes or more.
bool voe_3d_models_landscape_glb_set(voe_3d_models *models, const char *path,
				     uint32_t group, uint32_t glb,
				     const char *model);

// Glb `glb` of group `group`, both held, of `path`'s grid takes `look`, its
// ranges asserted as the file's, a blend height 0 or 5-1000 cm (0481 point
// 4). An equal look, every field counted, does nothing (0460 point 3); else a
// new size or share places the group's copies again, a new size, spread or
// share remakes its grown copies over the grid, and the entry is edited and
// `changes` raised.
void voe_3d_models_landscape_look_set(voe_3d_models *models, const char *path,
				      uint32_t group, uint32_t glb,
				      voe_assets_landscape_look look);

// Layer `layer`, below layer_count, of `path`'s grid grows group `group`,
// below group_count, at `density` tenths of copies per 100 m², up to
// VOE_ASSETS_LANDSCAPE_GROW_MAX, 0 none; asserted. Equal does nothing; else
// that group is remade over the grid, the entry edited and `changes` raised.
void voe_3d_models_landscape_grow_set(voe_3d_models *models, const char *path,
				      uint32_t layer, uint32_t group,
				      uint16_t density);

// A loaded landscape's grown copies and their placed transforms.
typedef struct {
	const voe_assets_landscape_copy *copies;
	const voe_scene_transform *placed;
	uint32_t count;
} voe_3d_models_grown;

// `entry`'s grown copies; zeroed for an entry that is no loaded landscape of
// this store.
voe_3d_models_grown voe_3d_models_landscape_grown(const voe_3d_models *models,
						  const voe_3d_model_entry *entry);

// `entry`'s copies' placed transforms, `entry->landscape->copy_count` of them;
// NULL for an entry that is no loaded landscape of this store.
const voe_scene_transform *
voe_3d_models_landscape_placed(const voe_3d_models *models,
			       const voe_3d_model_entry *entry);
