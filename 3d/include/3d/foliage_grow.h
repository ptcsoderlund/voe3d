// The foliage a landscape's layers grow by themselves (0476 point 3): the
// copies a remake makes over an area from the paint and each layer's
// `grows`, beside the hand-painted copies of 3d/foliage.h. CPU only; no
// device. The model store keeps what it returns.
//
//     count = voe_3d_foliage_grow(&land, rect, 1u << kind, grown, placed,
//                                 count);
//
// THE LATTICE. Kind k's points lie on a lattice from the grid's corner at
// spacing √(1000 / D) m, D the most tenths of copies per 100 m² any layer
// grows of k. Each cell's point is jittered inside it, and its keep, scale and
// turn drawn, from a hash of (k, cell x, cell z). The point is kept where the
// keep draw is under d / D, d each layer's bilinear weight / 255 times its
// tenths, summed (paint NULL is all layer 0). Scale and turn come from the
// kind's look as a scatter draws them (3d/foliage.h), lean its tilt % of 255.
// THE LATTICE IS AT THE MOST ANY LAYER ASKS because the keep is a share and a
// share is never above one: a sparser lattice could not reach the thickest
// layer, and a denser one would make more points to throw away.
//
// NO STATE. A point and its draws depend on nothing but the kind, the cell and
// the paint and grows at it, so a remake over any area gives exactly what a
// remake of the whole grid gives there: an edit remakes only what it changed,
// and the game grows the editor's copies from the same file. A copy belongs to
// an area by its position, in whole millimetres, half-open.
//
// Constraints: a remake walks every grown copy once, then the lattice cells
// over the area, kind by kind; nothing allocates. `grown` and `placed` have
// room for VOE_3D_FOLIAGE_GROWN; past it a remake adds what fits, so which
// copies stand then depends on the order of remakes.
#pragma once

#include <3d/landscape.h>
#include <assets/landscape.h>
#include <scene/transform_component.h>

#include <stdint.h>

// The most grown copies a landscape holds.
#define VOE_3D_FOLIAGE_GROWN VOE_ASSETS_LANDSCAPE_COPIES

// The new count. The area is the rect's points widened by one cell, in metres,
// half-open. Every copy in grown[0..count) of a kind in `kinds` (bit k) standing
// in the area is removed by moving the last in, `placed` alike; then for each
// such kind every kept lattice point in the area is appended, up to the cap,
// and placed by voe_3d_foliage_place. A kind no layer grows adds none; kinds at
// or past the landscape's kind_count are ignored.
uint32_t voe_3d_foliage_grow(const voe_assets_landscape *landscape,
			     voe_3d_landscape_rect rect, uint32_t kinds,
			     voe_assets_landscape_copy *grown,
			     voe_scene_transform *placed, uint32_t count);
