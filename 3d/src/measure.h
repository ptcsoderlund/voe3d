// How far apart two bakes of one model are, internal to 3d: the error of a
// `.glb`'s own `_LOD<n>` level, measured against level 0 when the levels file is
// made, so own levels swap by their error on screen as made ones do (ADR-0435
// point 3, ADR-0436 point 6).
//
//     float error = voe_3d_measure_apart(scratch, &level0, &own);
//
// THE MEASURE IS SYMMETRIC: the largest distance from any vertex of `a` to the
// nearest triangle of `b`, and from any vertex of `b` to the nearest triangle of
// `a`, in the bakes' own metres. One way alone misses a level that drops a
// spike: every vertex it keeps lies on level 0, but the spike's tip lies far
// from it. The same bake is 0. Positions are the vertices' as
// `voe_render_vertex` holds them; triangles are every part's, indices counted
// from the part's first vertex (model_bake.h). Both bakes hold a triangle,
// asserted.
//
// A UNIFORM GRID FINDS THE NEAREST TRIANGLE. Each way builds one over the
// searched bake's triangles' box, about one triangle per cell (an axis on which
// the box is too thin for a cell gets one), each triangle listed in every cell
// its box overlaps. A vertex's search visits rings of cells outward from the
// cell of its nearest point in the box and stops once the next ring is farther
// than the best triangle found, so a vertex near the surface costs a few cells
// and the whole is about linear in the two bakes' sizes. A vertex far from the
// other bake visits rings up to the grid's size (rule 14 in ADR-0168), and a
// triangle much larger than its neighbours is listed in many cells: the ceiling
// a bounding-volume tree would lift.
//
// The grid, its lists and a copy of the triangles' corners are on `scratch`,
// rewound before it returns.
#pragma once

#include "model_bake.h"

#include <base/arena.h>

float voe_3d_measure_apart(voe_base_arena *scratch,
			   const voe_3d_model_bake *a,
			   const voe_3d_model_bake *b);
