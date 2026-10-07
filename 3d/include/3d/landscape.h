// The arithmetic on a landscape's grid of heights (assets/landscape.h) that
// the model store, the pick and the editor share (0379 points 2 and 3): the
// height under a point, the ray that meets the ground, a brush's stamp, and the
// 4 × 4 chunks it is drawn as. CPU only; no device.
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
// CHUNKS SHARE THEIR EDGE HEIGHTS. Chunk (cx, cz) holds heights cx·k to
// cx·k + k with k = cells/4, so neighbours hold the same edge row and their
// vertices meet without a crack. A stamp on that row touches both chunks.
#pragma once

#include <assets/landscape.h>
#include <base/arena.h>
#include <math/float3.h>
#include <render/device.h>

#include <stdbool.h>
#include <stdint.h>

// Chunks per side: 16 in all, one per model part.
#define VOE_3D_LANDSCAPE_CHUNKS 4

// Height indices, end exclusive; empty when x0 ≥ x1 or z0 ≥ z1. Chunk
// ranges come back in the same struct.
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

typedef struct {
	voe_render_vertex *vertices;
	uint32_t vertex_count;
	uint32_t *indices;
	uint32_t index_count;
} voe_3d_landscape_mesh;

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

// Chunk cz·4 + cx's (cells/4 + 1)² vertices and its triangles, on `arena`.
voe_3d_landscape_mesh
voe_3d_landscape_chunk(const voe_assets_landscape *landscape, uint32_t chunk,
		       voe_base_arena *arena);

// The chunk x and z ranges a height rect touches; empty for an empty rect.
voe_3d_landscape_rect
voe_3d_landscape_chunks(const voe_assets_landscape *landscape,
			voe_3d_landscape_rect heights);

// The grid's box in its own space: the square, from the lowest height to the
// highest.
void voe_3d_landscape_box(const voe_assets_landscape *landscape,
			  voe_math_float3 *min, voe_math_float3 *max);
