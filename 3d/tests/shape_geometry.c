// THE CLAIMS: that the store answers for the three kinds it knows and for
// nothing else, that it holds each kind's own triangles, that every kind is a
// closed surface whose edges carry unit-length normals, and that every triangle
// of every kind is wound the way the edge walk assumes.
//
// THE WINDING IS THE CLAIM WORTH THE MOST HERE. 3d/src/shape_geometry.c takes
// normalize(cross(b - a, c - a)) as a triangle's *outward* normal, which is only
// true while every built-in shape is wound counter-clockwise seen from outside.
// Nothing in that file could notice a shape wound the other way: the edges would
// still be found, still be paired, still be unit length — and every silhouette
// walked over them and every ray cast at them would be inside out. The check
// below compares each triangle's computed normal with the three normals its own
// vertices carry, which the builders set outward by hand, so a rebuilt shape
// wound the other way fails here and not in a picture.
//
// A CLOSED SURFACE IS edge_count == index_count / 2: three edges to a triangle
// and two triangles to an edge. It is the one number that proves the weld did
// its work — welded by index instead, a cube would have 36 edges rather than 18,
// and a capsule's seam and poles would come apart.
//
// THE CUBE'S EDGES ARE NOT ALL FOLDS, and this file says so rather than claiming
// more than is true: a cube drawn as twelve triangles has six edges that are a
// face's diagonal, where the two triangles are the same flat face and carry the
// same normal. Twelve of its eighteen edges are its real edges, where the two
// normals are a right angle apart. Both numbers are checked, because a weld that
// merged too much or too little moves them.
//
// IT NEEDS NO GRAPHICS CARD: nothing here opens a device, and that is the point
// of the geometry being kept away from the upload.
#include <3d/shape_component.h>
#include <3d/shape_geometry.h>

#include "../src/capsule.h"
#include "../src/cube.h"
#include "../src/cylinder.h"

#include <base/arena.h>

#include <math/float3.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define SCRATCH (256 * 1024)
#define TOLERANCE 1e-4f

static bool is_unit(voe_math_float3 v)
{
	return fabsf(voe_math_float3_length(v) - 1.0f) <= TOLERANCE;
}

// Every edge of one kind: two triangles' worth of unit-length normals, and how
// many of them are a fold rather than a flat pair.
static uint32_t check_edges(const voe_3d_shape_geometry *geometry)
{
	uint32_t folds = 0;

	VOE_TEST_CHECK_INT(geometry->edge_count, geometry->index_count / 2);
	for (uint32_t i = 0; i < geometry->edge_count; i++) {
		const voe_3d_shape_edge *edge = &geometry->edges[i];

		if (!is_unit(edge->left) || !is_unit(edge->right)) {
			VOE_TEST_CHECK(false);
			break;
		}
		if (voe_math_float3_dot(edge->left, edge->right) < 1.0f -
								  TOLERANCE)
			folds++;
	}
	return folds;
}

// Every triangle of one kind, wound counter-clockwise seen from outside: the
// normal its corners' positions give agrees with the normal each of those
// corners carries.
static void check_winding(const voe_3d_shape_geometry *geometry)
{
	for (uint32_t i = 0; i < geometry->index_count; i += 3) {
		const voe_render_vertex *a =
			&geometry->vertices[geometry->indices[i + 0]];
		const voe_render_vertex *b =
			&geometry->vertices[geometry->indices[i + 1]];
		const voe_render_vertex *c =
			&geometry->vertices[geometry->indices[i + 2]];
		const voe_math_float3 normal = voe_math_float3_normalize(
			voe_math_float3_cross(
				voe_math_float3_sub(b->position, a->position),
				voe_math_float3_sub(c->position, a->position)));

		if (voe_math_float3_dot(normal, a->normal) <= 0.0f ||
		    voe_math_float3_dot(normal, b->normal) <= 0.0f ||
		    voe_math_float3_dot(normal, c->normal) <= 0.0f) {
			VOE_TEST_CHECK(false);
			return;
		}
	}
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(SCRATCH);
	voe_3d_shape_geometries geometries;
	const voe_3d_shape_geometry *cube;
	const voe_3d_shape_geometry *capsule;
	const voe_3d_shape_geometry *cylinder;

	voe_3d_shape_geometries_create(arena, &geometries);

	// Nought is no kind and 99 is a kind this build does not know: the
	// answer voe_3d_shape_kind_names gives them, in the same shape.
	VOE_TEST_CHECK(voe_3d_shape_geometry_of(&geometries, 0) == NULL);
	VOE_TEST_CHECK(voe_3d_shape_geometry_of(&geometries, 99) == NULL);

	cube = voe_3d_shape_geometry_of(&geometries, VOE_3D_SHAPE_CUBE);
	capsule = voe_3d_shape_geometry_of(&geometries, VOE_3D_SHAPE_CAPSULE);
	cylinder = voe_3d_shape_geometry_of(&geometries, VOE_3D_SHAPE_CYLINDER);
	VOE_TEST_CHECK(cube == &geometries.kinds[VOE_3D_SHAPE_CUBE]);
	VOE_TEST_CHECK(capsule == &geometries.kinds[VOE_3D_SHAPE_CAPSULE]);
	VOE_TEST_CHECK(cylinder == &geometries.kinds[VOE_3D_SHAPE_CYLINDER]);

	// The cube is pointed straight at its constants; nothing is copied for
	// it. The other two are exactly their own two counts long.
	VOE_TEST_CHECK(cube->vertices == voe_3d_cube_vertices);
	VOE_TEST_CHECK(cube->indices == voe_3d_cube_indices);
	VOE_TEST_CHECK_INT(cube->vertex_count, VOE_3D_CUBE_VERTICES);
	VOE_TEST_CHECK_INT(cube->index_count, VOE_3D_CUBE_INDICES);
	VOE_TEST_CHECK_INT(capsule->vertex_count, VOE_3D_CAPSULE_VERTICES);
	VOE_TEST_CHECK_INT(capsule->index_count, VOE_3D_CAPSULE_INDICES);
	VOE_TEST_CHECK_INT(cylinder->vertex_count, VOE_3D_CYLINDER_VERTICES);
	VOE_TEST_CHECK_INT(cylinder->index_count, VOE_3D_CYLINDER_INDICES);

	// Twelve real edges and six face diagonals — see the head of this file.
	VOE_TEST_CHECK_INT(check_edges(cube), 12);
	check_edges(capsule);
	check_edges(cylinder);

	check_winding(cube);
	check_winding(capsule);
	check_winding(cylinder);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
