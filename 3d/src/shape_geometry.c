// The three built-in shapes on the CPU: the cube pointed straight at, the
// capsule and the cylinder built into the caller's arena, and the edges of all
// three found by welding their vertices by position and walking their triangles —
// the one build any triangles go through, a model's too. Nothing here opens a
// device (3d/shape_geometry.h).
//
// THE EDGES ARE FOUND IN AN OPEN-ADDRESSED TABLE keyed by an edge's two welded
// vertex numbers, smaller first, and sized at the next power of two above three
// times the triangle count — one slot per edge of every triangle, so the table
// is at most half full whatever the surface is and a probe never walks far. The
// first triangle to reach an edge starts it and the second fills its other side.
// A third triangle along one edge would overwrite the second's normal; no
// built-in shape is that surface, and one that was would have no silhouette
// worth walking either.
//
// THE TRIANGLES ARE WALKED TWICE, once to count the edges and once to fill them,
// so that what is left in the arena is an array exactly edge_count long. The
// alternative is one walk into an array sized at the upper bound, which leaves
// slack an arena cannot give back. Both walks are linear in the triangles, a
// model's millions as well as a shape's few thousand.
//
// THE WELD GOES THROUGH CELLS the weld's size a side, hashed by their three
// coordinates into an open-addressed table of each cell's chain of
// representatives (ADR-0448 point 6). Two positions within the weld are at most
// one cell apart on every axis, so a vertex compared with the representatives of
// its own cell and the 26 around it meets every one it welds to, and taking the
// lowest-numbered of them gives the representative the old walk over every
// earlier vertex gave, in time linear in the vertices: two representatives in
// one cell would be within the weld, so a cell holds one but for rounding.
//
// A TRIANGLE WITH NO AREA HAS NO FACING, so it gives no edges: two corners on
// one point or three on one line make a cross of nought, and it is skipped in
// both walks alike. Exported models are full of them and they are not a fault.
// A real triangle whose neighbour along an edge was flat keeps that edge as a
// boundary edge, always on the silhouette, as an open surface's is.
#include "capsule.h"
#include "cube.h"
#include "cylinder.h"

#include <3d/shape_component.h>
#include <3d/shape_geometry.h>

#include <base/assert.h>

#include <math/float3.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define KIND_COUNT (sizeof(((voe_3d_shape_geometries *)0)->kinds) / \
		    sizeof(voe_3d_shape_geometry))

// An empty slot or the end of a chain. No vertex number can be this: there are
// fewer than 2^31 vertices, which the weld asserts.
#define EMPTY UINT32_MAX

// A cell coordinate is held inside this, so that it and its neighbours fit an
// int64_t whatever the float; holding is monotonic, so cells within the weld
// stay at most one apart.
#define CELL_LIMIT ((float)(INT64_C(1) << 40))

// Where a position falls in the weld's grid.
struct cell {
	int64_t x;
	int64_t y;
	int64_t z;
};

// One edge of the surface, as the table holds it while the triangles are walked:
// its two welded vertex numbers and which edge of the answer it is.
struct slot {
	uint32_t a;
	uint32_t b;
	uint32_t edge;
};

static bool same_position(voe_math_float3 a, voe_math_float3 b)
{
	return fabsf(a.x - b.x) < VOE_3D_SHAPE_GEOMETRY_WELD &&
	       fabsf(a.y - b.y) < VOE_3D_SHAPE_GEOMETRY_WELD &&
	       fabsf(a.z - b.z) < VOE_3D_SHAPE_GEOMETRY_WELD;
}

// A NaN goes to the lowest cell; it welds with nothing wherever it is.
static int64_t cell_coordinate(float x)
{
	const float cell = floorf(x / VOE_3D_SHAPE_GEOMETRY_WELD);

	if (!(cell > -CELL_LIMIT))
		return -(INT64_C(1) << 40);
	if (cell > CELL_LIMIT)
		return INT64_C(1) << 40;
	return (int64_t)cell;
}

static struct cell cell_of(voe_math_float3 p)
{
	return (struct cell){ cell_coordinate(p.x), cell_coordinate(p.y),
			      cell_coordinate(p.z) };
}

static uint32_t hash_cell(struct cell c)
{
	uint64_t h = (uint64_t)c.x * 0x9e3779b97f4a7c15u;

	h = (h ^ (uint64_t)c.y) * 0xc2b2ae3d27d4eb4fu;
	h = (h ^ (uint64_t)c.z) * 0x165667b19e3779f9u;
	h ^= h >> 33;
	return (uint32_t)(h ^ (h >> 29));
}

static uint32_t hash_pair(uint32_t a, uint32_t b)
{
	uint32_t h = (a * 2654435761u) ^ (b * 2246822519u);

	return h ^ (h >> 15);
}

// The smallest power of two greater than `n`.
static uint32_t power_of_two_above(uint32_t n)
{
	uint32_t size = 1;

	while (size <= n)
		size <<= 1;
	return size;
}

// The slot of `heads` that holds cell `c`'s chain, or the empty slot it would
// go in. A slot's cell is not stored: it is its first representative's.
static uint32_t find_cell(const uint32_t *heads, uint32_t slots,
			  const voe_3d_shape_geometry *geometry, struct cell c)
{
	uint32_t at = hash_cell(c) & (slots - 1);

	for (uint32_t probes = 0; probes < slots; probes++) {
		struct cell held;

		if (heads[at] == EMPTY)
			break;
		held = cell_of(geometry->vertices[heads[at]].position);
		if (held.x == c.x && held.y == c.y && held.z == c.z)
			break;
		at = (at + 1) & (slots - 1);
	}
	return at;
}

// Fills `welded` with one representative vertex number per vertex: the
// lowest-numbered earlier representative within the weld, or itself, which
// then joins its cell's chain. Cells and chains are this arena's and are given
// back before it returns.
static void weld(voe_base_arena *arena, const voe_3d_shape_geometry *geometry,
		 uint32_t *welded)
{
	const uint32_t count = geometry->vertex_count;
	const uint32_t slots = power_of_two_above(count * 2);
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	uint32_t *heads = voe_base_arena_push(arena, sizeof *heads * slots);
	uint32_t *next = voe_base_arena_push(arena, sizeof *next * count);

	VOE_BASE_ASSERT(welded != NULL, "welding into nothing");
	memset(heads, 0xff, sizeof *heads * slots);
	for (uint32_t i = 0; i < count; i++) {
		const voe_math_float3 p = geometry->vertices[i].position;
		const struct cell home = cell_of(p);

		welded[i] = i;
		for (int64_t n = 0; n < 27; n++) {
			const struct cell around = { home.x + n % 3 - 1,
						     home.y + n / 3 % 3 - 1,
						     home.z + n / 9 - 1 };
			uint32_t j = heads[find_cell(heads, slots, geometry,
						     around)];

			// A chain holds earlier vertices only, each once.
			for (; j != EMPTY; j = next[j])
				if (j < welded[i] &&
				    same_position(p,
						  geometry->vertices[j].position))
					welded[i] = j;
		}
		if (welded[i] == i) {
			const uint32_t at =
				find_cell(heads, slots, geometry, home);

			next[i] = heads[at];
			heads[at] = i;
		}
	}
	voe_base_arena_rewind(arena, mark);
	VOE_BASE_ASSERT(count == 0 || welded[0] == 0, "first vertex not its own");
}

// Walks `geometry`'s triangles and returns how many edges its surface has,
// filling `edges` with them unless it is NULL — which is how the count is had
// before there is anywhere to put them. Its working memory is this arena's and
// is given back before it returns.
static uint32_t walk_edges(voe_base_arena *arena,
			   const voe_3d_shape_geometry *geometry,
			   voe_3d_shape_edge *edges)
{
	const uint32_t triangles = geometry->index_count / 3;
	const uint32_t slots = power_of_two_above(triangles * 3);
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	uint32_t *welded =
		voe_base_arena_push(arena,
				    sizeof *welded * geometry->vertex_count);
	struct slot *table;
	uint32_t count = 0;

	weld(arena, geometry, welded);
	table = voe_base_arena_push(arena, sizeof *table * slots);
	memset(table, 0xff, sizeof *table * slots);
	for (uint32_t t = 0; t < triangles; t++) {
		const uint32_t corner[3] = {
			geometry->indices[t * 3 + 0],
			geometry->indices[t * 3 + 1],
			geometry->indices[t * 3 + 2],
		};
		const voe_math_float3 a =
			geometry->vertices[corner[0]].position;
		const voe_math_float3 b =
			geometry->vertices[corner[1]].position;
		const voe_math_float3 c =
			geometry->vertices[corner[2]].position;
		// Outward because every built-in shape is wound
		// counter-clockwise seen from outside (src/cube.h).
		const voe_math_float3 cross = voe_math_float3_cross(
			voe_math_float3_sub(b, a), voe_math_float3_sub(c, a));
		voe_math_float3 normal;

		// No area, no facing: the test normalize asserts on.
		if (!(voe_math_float3_length(cross) > 0.0f))
			continue;
		normal = voe_math_float3_normalize(cross);
		for (uint32_t e = 0; e < 3; e++) {
			const uint32_t p = welded[corner[e]];
			const uint32_t q = welded[corner[(e + 1) % 3]];
			const uint32_t low = p < q ? p : q;
			const uint32_t high = p < q ? q : p;
			uint32_t at = hash_pair(low, high) & (slots - 1);

			while (table[at].a != EMPTY &&
			       (table[at].a != low || table[at].b != high))
				at = (at + 1) & (slots - 1);
			if (table[at].a != EMPTY) {
				if (edges != NULL)
					edges[table[at].edge].right = normal;
				continue;
			}
			table[at] = (struct slot){ low, high, count };
			if (edges != NULL)
				edges[count] = (voe_3d_shape_edge){
					.a = geometry->vertices[low].position,
					.b = geometry->vertices[high].position,
					.left = normal,
					.right = normal,
				};
			count++;
		}
	}
	voe_base_arena_rewind(arena, mark);
	return count;
}

void voe_3d_shape_geometry_build(voe_base_arena *arena,
				 const voe_render_vertex *vertices,
				 uint32_t vertex_count, const uint32_t *indices,
				 uint32_t index_count,
				 voe_3d_shape_geometry *out)
{
	uint32_t count;
	voe_3d_shape_edge *edges;

	VOE_BASE_ASSERT(arena != NULL, "building edges with no arena");
	VOE_BASE_ASSERT(out != NULL, "building edges into nothing");
	VOE_BASE_ASSERT(index_count % 3 == 0, "edges of a partial triangle");
	VOE_BASE_ASSERT(vertex_count < UINT32_C(1) << 31,
			"more vertices than the weld's table can number");

	*out = (voe_3d_shape_geometry){
		.vertices = vertices,
		.vertex_count = vertex_count,
		.indices = indices,
		.index_count = index_count,
	};
	count = walk_edges(arena, out, NULL);
	// Only flat triangles leave no edges, and an arena takes no empty push.
	if (count == 0)
		return;
	edges = voe_base_arena_push(arena, sizeof *edges * count);
	walk_edges(arena, out, edges);
	out->edges = edges;
	out->edge_count = count;
}

void voe_3d_shape_geometries_create(voe_base_arena *arena,
				    voe_3d_shape_geometries *out)
{
	voe_render_vertex *capsule_vertices;
	uint32_t *capsule_indices;
	voe_render_vertex *cylinder_vertices;
	uint32_t *cylinder_indices;

	VOE_BASE_ASSERT(arena != NULL, "building shape geometry with no arena");
	VOE_BASE_ASSERT(out != NULL, "building shape geometry into nothing");

	*out = (voe_3d_shape_geometries){ 0 };

	// The cube is data already (src/cube.h); nothing is copied for it.
	voe_3d_shape_geometry_build(arena, voe_3d_cube_vertices,
				    VOE_3D_CUBE_VERTICES, voe_3d_cube_indices,
				    VOE_3D_CUBE_INDICES,
				    &out->kinds[VOE_3D_SHAPE_CUBE]);

	capsule_vertices = voe_base_arena_push(
		arena, sizeof *capsule_vertices * VOE_3D_CAPSULE_VERTICES);
	capsule_indices = voe_base_arena_push(
		arena, sizeof *capsule_indices * VOE_3D_CAPSULE_INDICES);
	voe_3d_capsule_build(capsule_vertices, capsule_indices);
	voe_3d_shape_geometry_build(arena, capsule_vertices,
				    VOE_3D_CAPSULE_VERTICES, capsule_indices,
				    VOE_3D_CAPSULE_INDICES,
				    &out->kinds[VOE_3D_SHAPE_CAPSULE]);

	cylinder_vertices = voe_base_arena_push(
		arena, sizeof *cylinder_vertices * VOE_3D_CYLINDER_VERTICES);
	cylinder_indices = voe_base_arena_push(
		arena, sizeof *cylinder_indices * VOE_3D_CYLINDER_INDICES);
	voe_3d_cylinder_build(cylinder_vertices, cylinder_indices);
	voe_3d_shape_geometry_build(arena, cylinder_vertices,
				    VOE_3D_CYLINDER_VERTICES, cylinder_indices,
				    VOE_3D_CYLINDER_INDICES,
				    &out->kinds[VOE_3D_SHAPE_CYLINDER]);
	VOE_BASE_ASSERT(KIND_COUNT == 4, "a kind this build does not build");
}

const voe_3d_shape_geometry *
voe_3d_shape_geometry_of(const voe_3d_shape_geometries *geometries,
			 uint32_t kind)
{
	VOE_BASE_ASSERT(geometries != NULL, "reading shape geometry from nothing");

	if (kind == 0 || kind >= KIND_COUNT ||
	    geometries->kinds[kind].vertices == NULL)
		return NULL;
	return &geometries->kinds[kind];
}
