// The measure measure.h describes: each way, the searched bake's triangles
// copied as corners, a grid sized and filled over them, and every vertex of the
// other bake searched ring by ring. The point-to-triangle distance is Ericson's
// closest point by regions (Real-Time Collision Detection 5.1.5), its edges
// through one segment helper so a flattened triangle divides by nothing.
// Single-threaded; everything is on the caller's scratch, rewound each way.
#include "measure.h"

#include <base/arena.h>
#include <base/assert.h>
#include <math/float3.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// A grid over one bake's triangles. Cell (x, y, z) is x + n[0] * (y + n[1] * z);
// its triangles are list[start[cell]] up to list[start[cell + 1]].
typedef struct {
	const voe_math_float3 *corners; // three per triangle
	uint32_t triangle_count;
	float lo[3];
	float hi[3];
	uint32_t n[3];
	float per_metre[3]; // cells per metre along an axis, 0 for one cell
	uint32_t *start;
	uint32_t *list;
	uint32_t *stamp; // the search that last visited a triangle
	uint32_t search;
} measure_grid;

static float measure_axis(voe_math_float3 v, int axis)
{
	return axis == 0 ? v.x : axis == 1 ? v.y : v.z;
}

static float measure_length_squared(voe_math_float3 v)
{
	return voe_math_float3_dot(v, v);
}

static float measure_segment_squared(voe_math_float3 p, voe_math_float3 a,
				     voe_math_float3 b)
{
	const voe_math_float3 ab = voe_math_float3_sub(b, a);
	const float length = voe_math_float3_dot(ab, ab);
	float t = 0.0f;

	if (length > 0.0f)
		t = voe_math_float3_dot(voe_math_float3_sub(p, a), ab) / length;
	t = fminf(fmaxf(t, 0.0f), 1.0f);
	return measure_length_squared(
		voe_math_float3_sub(p, voe_math_float3_lerp(a, b, t)));
}

// Ericson 5.1.5: which of the triangle's seven regions `p` projects into, and
// the squared distance to the closest point there.
static float measure_triangle_squared(voe_math_float3 p, voe_math_float3 a,
				      voe_math_float3 b, voe_math_float3 c)
{
	const voe_math_float3 ab = voe_math_float3_sub(b, a);
	const voe_math_float3 ac = voe_math_float3_sub(c, a);
	const voe_math_float3 ap = voe_math_float3_sub(p, a);
	const voe_math_float3 bp = voe_math_float3_sub(p, b);
	const voe_math_float3 cp = voe_math_float3_sub(p, c);
	const float d1 = voe_math_float3_dot(ab, ap);
	const float d2 = voe_math_float3_dot(ac, ap);
	const float d3 = voe_math_float3_dot(ab, bp);
	const float d4 = voe_math_float3_dot(ac, bp);
	const float d5 = voe_math_float3_dot(ab, cp);
	const float d6 = voe_math_float3_dot(ac, cp);
	const float vc = d1 * d4 - d3 * d2;
	const float vb = d5 * d2 - d1 * d6;
	const float va = d3 * d6 - d5 * d4;
	float v, w;

	if (d1 <= 0.0f && d2 <= 0.0f)
		return measure_length_squared(ap);
	if (d3 >= 0.0f && d4 <= d3)
		return measure_length_squared(bp);
	if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
		return measure_segment_squared(p, a, b);
	if (d6 >= 0.0f && d5 <= d6)
		return measure_length_squared(cp);
	if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
		return measure_segment_squared(p, a, c);
	if (va <= 0.0f && d4 - d3 >= 0.0f && d5 - d6 >= 0.0f)
		return measure_segment_squared(p, b, c);
	if (!(va + vb + vc > 0.0f)) // flattened: its nearest point is on an edge
		return fminf(measure_segment_squared(p, a, b),
			     fminf(measure_segment_squared(p, a, c),
				   measure_segment_squared(p, b, c)));
	v = vb / (va + vb + vc);
	w = vc / (va + vb + vc);
	return measure_length_squared(voe_math_float3_sub(
		ap, voe_math_float3_add(voe_math_float3_scale(ab, v),
					voe_math_float3_scale(ac, w))));
}

// Every part's triangles as three corners each, and their joint box.
static void measure_corners(voe_base_arena *scratch,
			    const voe_3d_model_bake *bake, measure_grid *grid)
{
	voe_math_float3 *corners;
	uint32_t count = 0;
	uint32_t written = 0;

	VOE_BASE_ASSERT(bake->part_count <= VOE_3D_MODEL_PARTS, "a bake's parts");
	for (uint32_t p = 0; p < bake->part_count; p++) {
		VOE_BASE_ASSERT(bake->parts[p].index_count % 3 == 0,
				"a part's indices are whole triangles");
		count += bake->parts[p].index_count / 3;
	}
	VOE_BASE_ASSERT(count > 0, "a measured bake holds a triangle");
	corners = voe_base_arena_push(scratch, sizeof *corners * 3 * (size_t)count);
	for (uint32_t p = 0; p < bake->part_count; p++) {
		const voe_3d_model_bake_part *part = &bake->parts[p];

		for (uint32_t i = 0; i < part->index_count; i++) {
			const uint32_t index = bake->indices[part->first_index + i];

			VOE_BASE_ASSERT(index < part->vertex_count,
					"an index counts from its part's first vertex");
			corners[written++] =
				bake->vertices[part->first_vertex + index].position;
		}
	}
	grid->corners = corners;
	grid->triangle_count = count;
	for (int a = 0; a < 3; a++) {
		grid->lo[a] = measure_axis(corners[0], a);
		grid->hi[a] = grid->lo[a];
	}
	for (uint32_t i = 1; i < written; i++) {
		for (int a = 0; a < 3; a++) {
			grid->lo[a] = fminf(grid->lo[a], measure_axis(corners[i], a));
			grid->hi[a] = fmaxf(grid->hi[a], measure_axis(corners[i], a));
		}
	}
}

// About one triangle per cell: a cell's side from the box's volume over its
// open axes, an axis shorter than that side closed to one cell and the side
// found again. Each round closes an axis or ends, so three rounds settle it.
static void measure_grid_size(measure_grid *grid)
{
	float extent[3];
	bool open[3];
	double side = 0.0;

	for (int a = 0; a < 3; a++) {
		extent[a] = grid->hi[a] - grid->lo[a];
		open[a] = extent[a] > 0.0f;
	}
	for (int round = 0; round < 4; round++) {
		double volume = 1.0;
		int dimensions = 0;
		bool closed = false;

		for (int a = 0; a < 3; a++) {
			if (open[a]) {
				volume *= extent[a];
				dimensions++;
			}
		}
		if (dimensions == 0)
			break;
		side = pow(volume / grid->triangle_count, 1.0 / dimensions);
		for (int a = 0; a < 3; a++) {
			if (open[a] && extent[a] < side) {
				open[a] = false;
				closed = true;
			}
		}
		if (!closed)
			break;
	}
	for (int a = 0; a < 3; a++) {
		grid->n[a] = 1;
		grid->per_metre[a] = 0.0f;
		if (open[a]) {
			grid->n[a] = (uint32_t)fmax(ceil(extent[a] / side), 1.0);
			grid->per_metre[a] = (float)grid->n[a] / extent[a];
		}
	}
}

static uint32_t measure_cell_of(const measure_grid *grid, int axis, float x)
{
	const float f = (x - grid->lo[axis]) * grid->per_metre[axis];

	if (!(f > 0.0f))
		return 0;
	if (f >= (float)(grid->n[axis] - 1))
		return grid->n[axis] - 1;
	return (uint32_t)f;
}

// One pass over every triangle's box's cells: counting into start[cell], or,
// once those are running sums, placing each triangle below its cell's end.
static void measure_grid_list(measure_grid *grid, bool place)
{
	for (uint32_t t = 0; t < grid->triangle_count; t++) {
		const voe_math_float3 *corner = &grid->corners[3 * t];
		uint32_t from[3], to[3];

		for (int a = 0; a < 3; a++) {
			const float x0 = measure_axis(corner[0], a);
			const float x1 = measure_axis(corner[1], a);
			const float x2 = measure_axis(corner[2], a);

			from[a] = measure_cell_of(grid, a, fminf(x0, fminf(x1, x2)));
			to[a] = measure_cell_of(grid, a, fmaxf(x0, fmaxf(x1, x2)));
		}
		for (uint32_t z = from[2]; z <= to[2]; z++) {
			for (uint32_t y = from[1]; y <= to[1]; y++) {
				for (uint32_t x = from[0]; x <= to[0]; x++) {
					const uint32_t cell =
						x + grid->n[0] * (y + grid->n[1] * z);

					if (place)
						grid->list[--grid->start[cell]] = t;
					else
						grid->start[cell]++;
				}
			}
		}
	}
}

static void measure_grid_fill(voe_base_arena *scratch, measure_grid *grid)
{
	const uint64_t cells = (uint64_t)grid->n[0] * grid->n[1] * grid->n[2];
	uint64_t sum = 0;

	VOE_BASE_ASSERT(cells < UINT32_MAX, "about one triangle per cell");
	grid->start = voe_base_arena_push(scratch,
					  sizeof *grid->start * (cells + 1));
	measure_grid_list(grid, false);
	for (uint64_t c = 0; c < cells; c++) {
		sum += grid->start[c];
		VOE_BASE_ASSERT(sum < UINT32_MAX, "the grid's lists fit a count");
		grid->start[c] = (uint32_t)sum;
	}
	grid->start[cells] = (uint32_t)sum;
	grid->list = voe_base_arena_push(scratch, sizeof *grid->list * sum);
	measure_grid_list(grid, true);
	grid->stamp = voe_base_arena_push(scratch, sizeof *grid->stamp *
							   grid->triangle_count);
	grid->search = 0;
}

static void measure_cell_visit(measure_grid *grid, uint32_t cell,
			       voe_math_float3 p, float *best)
{
	for (uint32_t i = grid->start[cell]; i < grid->start[cell + 1]; i++) {
		const uint32_t t = grid->list[i];
		const voe_math_float3 *corner = &grid->corners[3 * t];

		if (grid->stamp[t] == grid->search)
			continue;
		grid->stamp[t] = grid->search;
		*best = fminf(*best, measure_triangle_squared(p, corner[0],
							      corner[1], corner[2]));
	}
}

// The cells exactly `ring` steps from `centre` along their farthest axis,
// within the grid: the whole row where y or z is on the ring, else its two ends.
static void measure_ring(measure_grid *grid, const uint32_t centre[3],
			 uint32_t ring, voe_math_float3 p, float *best)
{
	int64_t from[3], to[3];

	for (int a = 0; a < 3; a++) {
		from[a] = (int64_t)centre[a] - ring;
		from[a] = from[a] < 0 ? 0 : from[a];
		to[a] = (int64_t)centre[a] + ring;
		to[a] = to[a] > grid->n[a] - 1 ? grid->n[a] - 1 : to[a];
	}
	for (int64_t z = from[2]; z <= to[2]; z++) {
		for (int64_t y = from[1]; y <= to[1]; y++) {
			const bool row = llabs(z - centre[2]) == ring ||
					 llabs(y - centre[1]) == ring;
			const uint32_t base =
				grid->n[0] * ((uint32_t)y + grid->n[1] * (uint32_t)z);

			for (int64_t x = from[0]; x <= to[0]; x++) {
				if (row || llabs(x - centre[0]) == ring)
					measure_cell_visit(grid, base + (uint32_t)x,
							   p, best);
				else
					x = (int64_t)centre[0] + ring - 1;
			}
		}
	}
}

// How near `q`, inside the grid's box in `centre`'s cell, is to any cell `ring`
// or more steps out: the nearest face of the cells within `ring` - 1 that has
// cells beyond it. Infinite when none has, every cell visited.
static float measure_ring_bound(const measure_grid *grid,
				const uint32_t centre[3], const float q[3],
				uint32_t ring)
{
	float bound = INFINITY;

	for (int a = 0; a < 3; a++) {
		if (grid->n[a] == 1)
			continue;
		if (centre[a] >= ring) {
			const float face = grid->lo[a] + (float)(centre[a] - ring + 1) /
							   grid->per_metre[a];

			bound = fminf(bound, fmaxf(q[a] - face, 0.0f));
		}
		if ((uint64_t)centre[a] + ring < grid->n[a]) {
			const float face = grid->lo[a] + (float)(centre[a] + ring) /
							   grid->per_metre[a];

			bound = fminf(bound, fmaxf(face - q[a], 0.0f));
		}
	}
	return bound;
}

// The squared distance from `p` to the grid's nearest triangle. `p` may lie
// outside the box: its nearest point in the box is no farther from any triangle,
// so the rings are measured from there.
static float measure_nearest(measure_grid *grid, voe_math_float3 p)
{
	uint32_t centre[3];
	float q[3];
	uint32_t reach = 1;
	float best = INFINITY;

	for (int a = 0; a < 3; a++) {
		q[a] = fminf(fmaxf(measure_axis(p, a), grid->lo[a]), grid->hi[a]);
		centre[a] = measure_cell_of(grid, a, q[a]);
		reach = grid->n[a] > reach ? grid->n[a] : reach;
	}
	grid->search++;
	for (uint32_t ring = 0; ring < reach; ring++) {
		if (ring > 0) {
			const float bound = measure_ring_bound(grid, centre, q, ring);

			if (bound * bound >= best)
				break;
		}
		measure_ring(grid, centre, ring, p, &best);
	}
	VOE_BASE_ASSERT(best < INFINITY, "every cell's triangles were in reach");
	return best;
}

// The largest distance from a vertex of `from` to the nearest triangle of `to`.
static float measure_one_way(voe_base_arena *scratch,
			     const voe_3d_model_bake *from,
			     const voe_3d_model_bake *to)
{
	const struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	measure_grid grid = { 0 };
	float farthest = 0.0f;

	measure_corners(scratch, to, &grid);
	measure_grid_size(&grid);
	measure_grid_fill(scratch, &grid);
	for (uint32_t v = 0; v < from->vertex_count; v++)
		farthest = fmaxf(farthest,
				 measure_nearest(&grid, from->vertices[v].position));
	voe_base_arena_rewind(scratch, mark);
	return sqrtf(farthest);
}

float voe_3d_measure_apart(voe_base_arena *scratch,
			   const voe_3d_model_bake *a,
			   const voe_3d_model_bake *b)
{
	float there, back;

	VOE_BASE_ASSERT(scratch != NULL && a != NULL && b != NULL,
			"a scratch and two bakes to measure");
	there = measure_one_way(scratch, a, b);
	back = measure_one_way(scratch, b, a);
	return fmaxf(there, back);
}
