// The arithmetic on a landscape's grid of heights (assets/landscape.h) that
// the model store, the pick and the editor share (0379 points 2 and 3): the
// height under a point, the ray that meets the ground, a brush's stamp, and
// the nodes chosen by the eye's distance from a min/max pyramid (0396 points 3
// and 4). CPU only; no device.
//
//     float distance;
//     if (voe_3d_landscape_ray(&land, origin, direction, &distance)) {
//             hit = origin + direction * distance;
//             changed = voe_3d_landscape_brush(&land, &brush, hit.x, hit.z,
//                                              seconds, scratch);
//     }
//
// THE GRID'S OWN SPACE (0379 point 1): x and z about the thing's origin, the
// grid centred on it, y metres up from it. A caller takes a world ray into
// this space with the thing's inverse transform and a hit back out with it.
//
// A MARCH AND A BISECT, NOT A TRIANGLE WALK. The ray is clipped to the grid's
// box, stepped half a cell at a time comparing its height with the bilinear
// ground, and the step that crosses is bisected. Half a cell cannot step over a
// ridge the bilinear ground has; a walk over 2 × 512² triangles per pick could.
// The steps are counted from the box, so a tall box costs more steps.
//
// THE WEIGHT AND THE RATES (0379 point 3): weight 1 inside radius·(1 −
// softness), smoothstepping to 0 at the radius. Raise and lower add ±
// strength·radius·0.5·weight·s metres; smooth and flatten move a height toward
// its neighbours' mean, or toward `target`, by min(1, 10·strength·weight·s).
// Seconds are clamped to 0.1 so a stalled frame stamps no more than a tenth of
// a second does: one long frame must not raise a mountain.
//
// THE NODE GRID. A level-L node is 32 × 32 quads stepping 2^L cells; levels
// run until one node covers the grid, and a node past its edge holds nothing
// and is never chosen. Level 0's range is twice a leaf's diagonal, each next
// doubling; a node morphs toward level L + 1 over the last 30% of its range.
//
// NEIGHBOURS DIFFER BY AT MOST ONE LEVEL, so the morph leaves no crack. A node
// splits when its box is within its children's range, the box's distance being
// the horizontal one to its square and the vertical one to the whole grid's
// heights: a peak in one node must not pull it nearer than the flat node
// beside it. A kept level L + 2 node is then past range L + 1, which a level L
// neighbour's parent, within range L and √5 level-L sides from their edge,
// cannot be. Past `capacity` refining stops and the bound may not hold.
//
// NO FRUSTUM TEST: every node over the square is chosen, seen or not; culling
// to the view is 093's (0395).
#pragma once

#include <assets/landscape.h>
#include <base/arena.h>
#include <math/float3.h>

#include <stdbool.h>
#include <stdint.h>

// Quads a node side; a level-L node steps 2^L cells.
#define VOE_3D_LANDSCAPE_NODE_QUADS 32

// Levels a pyramid holds at most: 32 · 2^6 is the most cells a grid has.
#define VOE_3D_LANDSCAPE_LOD_LEVELS 7

typedef struct {
	// The corner toward -x and -z in the grid's own space, and the side, in
	// metres.
	float x, z, side;
	// The lowest and highest height within it.
	float low, high;
	uint32_t level;
	// Metres from the eye where the morph toward level + 1 starts and ends.
	float morph_start, morph_end;
} voe_3d_landscape_node;

typedef struct {
	uint32_t levels;
	// Nodes a side per level, and the index of the level's first node.
	uint32_t sides[VOE_3D_LANDSCAPE_LOD_LEVELS];
	uint32_t first[VOE_3D_LANDSCAPE_LOD_LEVELS];
	// Each node's lowest and highest height, level by level, row-major.
	float *lows;
	float *highs;
} voe_3d_landscape_lod;

// Height indices, end exclusive; empty when x0 ≥ x1 or z0 ≥ z1.
typedef struct {
	uint32_t x0, z0, x1, z1;
} voe_3d_landscape_rect;

typedef enum {
	VOE_3D_BRUSH_RAISE,
	VOE_3D_BRUSH_LOWER,
	VOE_3D_BRUSH_SMOOTH,
	VOE_3D_BRUSH_FLATTEN,
} voe_3d_brush_kind;

typedef struct {
	voe_3d_brush_kind kind;
	// Metres, above 0.
	float radius;
	// 0..1.
	float strength;
	// 0..1: the share of the radius that fades.
	float softness;
	// Flatten's height, in metres.
	float target;
} voe_3d_brush;

// The bilinear height at (x, z), the point clamped onto the grid.
float voe_3d_landscape_height(const voe_assets_landscape *landscape, float x,
			      float z);

// Where origin + direction·t first goes below the ground, t ≥ 0, in
// `distance`. False, `distance` untouched, when it never does.
[[nodiscard]] bool voe_3d_landscape_ray(const voe_assets_landscape *landscape,
					voe_math_float3 origin,
					voe_math_float3 direction,
					float *distance);

// One stamp at (x, z) over `seconds`; smooth's copy goes on `scratch`, which
// is rewound. The rect of heights it changed, empty off the grid.
voe_3d_landscape_rect voe_3d_landscape_brush(voe_assets_landscape *landscape,
					     const voe_3d_brush *brush, float x,
					     float z, float seconds,
					     voe_base_arena *scratch);

// The grid's box in its own space: the square, from the lowest height to the
// highest.
void voe_3d_landscape_box(const voe_assets_landscape *landscape,
			  voe_math_float3 *min, voe_math_float3 *max);

// The pyramid of the grid's heights, on `arena`.
voe_3d_landscape_lod voe_3d_landscape_lod_build(const voe_assets_landscape *landscape,
						voe_base_arena *arena);

// The nodes over a changed height rect refreshed: leaves from the heights,
// each level above from the one below.
void voe_3d_landscape_lod_update(voe_3d_landscape_lod *lod,
				 const voe_assets_landscape *landscape,
				 voe_3d_landscape_rect heights);

// voe_3d_landscape_box from the pyramid's root, without walking the heights.
void voe_3d_landscape_lod_box(const voe_3d_landscape_lod *lod,
			      const voe_assets_landscape *landscape,
			      voe_math_float3 *min, voe_math_float3 *max);

// The nodes `eye`, in the grid's space, chooses, walked down from the root: a
// node split when its box is within its children's range and there is room
// for four more, else kept. How many were written, at most `capacity` (≥ 1).
uint32_t voe_3d_landscape_select(const voe_3d_landscape_lod *lod,
				 const voe_assets_landscape *landscape,
				 voe_math_float3 eye, voe_3d_landscape_node *nodes,
				 uint32_t capacity);
