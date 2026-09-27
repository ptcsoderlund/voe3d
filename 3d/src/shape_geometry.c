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
// alternative is one walk into an array sized at the upper bound, which leaves a
// third of a megabyte of slack an arena cannot give back. Both walks are a
// startup one-shot over a few thousand triangles.
//
// THE WELD COMPARES EVERY VERTEX WITH EVERY REPRESENTATIVE BEFORE IT, which is
// quadratic: 594 vertices for the capsule, the largest shape here, is a third of
// a million comparisons once at startup. A shape with tens of thousands of
// vertices — a model read from a file — wants a grid or a sorted key instead,
// and that is the card that brings the mesh table in.
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

// An empty slot. No welded vertex number can be this: they are indices into an
// array of at most a few hundred vertices.
#define EMPTY UINT32_MAX

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

// Fills `welded` with one representative vertex number per vertex: the first
// vertex in the array at the same position, which is itself for a vertex that is
// the first at its own position.
static void weld(const voe_3d_shape_geometry *geometry, uint32_t *welded)
{
	for (uint32_t i = 0; i < geometry->vertex_count; i++) {
		welded[i] = i;
		for (uint32_t j = 0; j < i; j++) {
			if (welded[j] != j)
				continue;
			if (same_position(geometry->vertices[i].position,
					  geometry->vertices[j].position)) {
				welded[i] = j;
				break;
			}
		}
	}
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
	struct slot *table = voe_base_arena_push(arena, sizeof *table * slots);
	uint32_t count = 0;

	weld(geometry, welded);
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
		const voe_math_float3 normal = voe_math_float3_normalize(
			voe_math_float3_cross(voe_math_float3_sub(b, a),
					      voe_math_float3_sub(c, a)));

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

	*out = (voe_3d_shape_geometry){
		.vertices = vertices,
		.vertex_count = vertex_count,
		.indices = indices,
		.index_count = index_count,
	};
	count = walk_edges(arena, out, NULL);
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
