// A part's triangles made fewer, internal to 3d: the engine's own simplifier
// (0425 point 2, rule 5: no meshoptimizer). A model's levels are made by calling
// it once per part per level, always from level 0's vertices and indices.
//
//     float error;
//     uint32_t written = voe_3d_simplify(scratch, vertices, vertex_count,
//                                        indices, index_count, target,
//                                        max_error, edge_degrees, out,
//                                        &error);
//
// HALF-EDGE COLLAPSE ONTO AN EXISTING VERTEX, CHEAPEST FIRST. A collapse moves
// one position onto a neighbour's; no point is made, so every index written
// names a vertex of `vertices` and level 0's vertices serve every level.
//
// THE COST IS A QUADRIC. Each position holds its triangles' planes weighted by
// their areas, plus, along each open border edge, the plane through that edge
// upright on its triangle, weighted by the edge's length squared. A collapse's
// cost is the two positions' weighted mean squared distance at the kept one, so
// its root is in model metres: the unit of `max_error` and of `*error`.
//
// TOPOLOGY IS WELDED BY EXACT POSITION: vertices with the same three floats are
// one position whatever else they carry. A position's vertices fall into sides:
// equal uv and colour complement, normals within `edge_degrees` (0 to 180) of
// the side's first vertex, 0 meaning equal (0430 point 1). An edge not shared
// by exactly two triangles is a border; an edge of two triangles naming
// different sides at either end is a seam edge. One side moves as before:
// freely, or as a border only along a border edge, so an outline keeps its
// shape; a soft position's corners take the kept vertex, normal and all. Two
// sides on exactly two seam edges and no border edge are a seam line, moving
// only along a seam edge, each side's corners onto the vertex the same side's
// triangle on that edge names at the kept position, so no seam tears. Anything
// else never moves. A collapse that turns a remaining triangle past 90°, or
// flattens it, is refused.
//
// It stops when at most `target` indices remain or the next collapse's root cost
// passes `max_error`. `out` takes at most `index_count` indices, whole triangles
// in their input order, none with two corners at one position (an input one that
// has is dropped); `*error` is the root of the largest cost taken, nought when
// none; the count written is returned. Indices below `vertex_count` and whole
// triangles are the caller's to give, asserted.
//
// Everything is on the caller's scratch, rewound before it returns; no recursion.
#pragma once

#include <base/arena.h>

#include <render/device.h>

#include <stdint.h>

uint32_t voe_3d_simplify(voe_base_arena *scratch,
			 const voe_render_vertex *vertices,
			 uint32_t vertex_count, const uint32_t *indices,
			 uint32_t index_count, uint32_t target, float max_error,
			 float edge_degrees, uint32_t *out, float *error);
