// The foliage arithmetic on a landscape's copies (assets/landscape.h), beside
// the sculpting stamps of 3d/landscape.h: where a copy stands, a scatter
// stamp, an erase stamp, undo's put and the copies placed again over changed
// heights (0442 points 2 and 3). CPU only; no device.
//
//     uint64_t seed = 0x9e3779b97f4a7c15u;
//     voe_assets_landscape_copy added[VOE_3D_FOLIAGE_TRIES];
//     const uint32_t n = voe_3d_foliage_scatter(&land, placed, &brush, hit.x,
//                                               hit.z, seconds, &seed, added);
//
// PLACED ON THE GROUND (0442 point 2): a copy stands at (x, height(x, z), z)
// in the grid's own space, its up tilted from +Y toward the ground's normal
// (central differences one cell apart) by lean / 255, then turned about +Y,
// then scaled uniformly by its kind's base size times its own scale (0460
// point 3). No height is stored, so `placed` is the caller's
// cache, one transform per copy in step with `copies`, and _replace keeps it
// true after a sculpt.
//
// A SCATTER STAMP (0442 point 3) tries ceil(density / 100 · πr² · min(1, 4s))
// points uniform in the disc, s clamped to 0.1, at most VOE_3D_FOLIAGE_TRIES.
// A point is kept where the brush's soft weight beats a random draw, it lies
// on the square, and no copy, old or kept this stamp, is within the spacing
// 0.84 · √(100 / density) m. Kind uniform among the picked, scale 1 ± size
// vary, turn ± turn vary, all from a xorshift64 the caller keeps.
//
// THE SPACING IS WHY A HELD BRUSH STOPS. Discs of the spacing packed at random
// fill to about 0.55 of the plane, 0.55 / (π · 0.42² · 100 / density) ≈ the
// density per 100 m²; past that, nearly every try lands within the spacing of
// a copy and is refused, so holding the brush longer adds next to nothing.
//
// ERASE KEEPS NO ORDER: a removed copy's place takes the last copy, in
// `copies` and `placed` alike, so a removal is O(1) and nothing shifts 65536
// entries. File order means nothing to a copy, and undo puts back by value.
//
// Constraints: `copies` and `placed` each have room for
// VOE_ASSETS_LANDSCAPE_COPIES; a stamp past that cap adds what fits. A scatter
// walks every copy once (its box test), then the near ones against each try;
// _put's removal finds each copy by a linear scan, count × copies, which an
// index by position would lift.
#pragma once

#include <3d/landscape.h>
#include <assets/landscape.h>
#include <scene/transform_component.h>

#include <stdbool.h>
#include <stdint.h>

// The most points one scatter stamp tries.
#define VOE_3D_FOLIAGE_TRIES 256

typedef struct {
	// Metres, above 0, and the share of it that fades, 0..1.
	float radius;
	float softness;
	// Copies per 100 m², above 0.
	float density;
	// 0..1: a copy's scale is 1 ± this.
	float size_vary;
	// Radians, 0..π: a copy's turn is ± this.
	float turn_vary;
	// 0..1: how far up leans toward the ground's normal.
	float lean;
	// Bit k: kind k; kinds at or past the landscape's kind_count are ignored.
	uint32_t picked;
} voe_3d_foliage_brush;

// The copy's transform in the grid's own space, as point 2 above.
voe_scene_transform voe_3d_foliage_place(const voe_assets_landscape *landscape,
					 voe_assets_landscape_copy copy);

// One scatter stamp at (x, z) over `seconds`, `*seed` nonzero and advanced.
// The kept copies are appended to `copies` up to the cap, each placed into
// `placed[i]` when `placed` is not NULL, and written to `added` (room
// VOE_3D_FOLIAGE_TRIES); how many. None picked adds none.
uint32_t voe_3d_foliage_scatter(voe_assets_landscape *landscape,
				voe_scene_transform *placed,
				const voe_3d_foliage_brush *brush, float x,
				float z, float seconds, uint64_t *seed,
				voe_assets_landscape_copy *added);

// Every copy of a picked kind within the radius of (x, z), at most `room`,
// removed by moving the last into its place (`placed` alike, when not NULL)
// and written to `removed`; how many.
uint32_t voe_3d_foliage_erase(voe_assets_landscape *landscape,
			      voe_scene_transform *placed,
			      const voe_3d_foliage_brush *brush, float x, float z,
			      voe_assets_landscape_copy *removed, uint32_t room);

// Undo's: `add` appends each copy (to the cap) and places it; otherwise each
// copy equal in every field is removed as erase removes, one not found passed
// over. `placed` may be NULL.
void voe_3d_foliage_put(voe_assets_landscape *landscape,
			voe_scene_transform *placed,
			const voe_assets_landscape_copy *copies, uint32_t count,
			bool add);

// Every copy within the rect's points widened by one cell placed again.
void voe_3d_foliage_replace(const voe_assets_landscape *landscape,
			    voe_scene_transform *placed,
			    voe_3d_landscape_rect heights);
