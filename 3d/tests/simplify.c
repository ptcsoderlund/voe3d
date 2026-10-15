// THE CLAIMS, on meshes built here in code, none needing a graphics card:
//
// 1. A flat 32 × 32-quad grid simplified to an eighth reaches the target with
//    an error under 1e-5, keeps its four corners, and writes only indices below
//    the vertex count and no triangle with two corners at one position.
// 2. A UV sphere of 32 × 16 simplified to a half reaches the target with an
//    error above nought and under a tenth of the radius, and every triangle
//    still faces out: its normal · its centroid above nought.
// 3. That sphere with a max_error of 1e-6 stops well above the target, writing
//    at most what it was given.
// 4. The grid with its middle column of positions duplicated as a uv seam keeps
//    the column's two end positions used, and no triangle names vertices of
//    both copies of the column: the seam moves along its line, untorn.
// 5. A flat-shaded open cylinder of 16 sides and 8 rings, each side its own
//    strip, to 96 indices at angle 0 writes at most 96 at an error under 1e-5:
//    the rings fall along straight seam lines.
// 6. That cylinder to 48 indices, max_error its radius: at angle 0 it writes 96
//    or more; at 180, softened, fewer, at an error above nought and under it.
//
// Every call passes the angle; 0 is the exact normals cases 1 to 4 were made for.
#include "../src/simplify.h"

#include <base/arena.h>

#include <render/device.h>

#include <testing/test.h>

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define QUADS 32
#define SIDE (QUADS + 1)
#define SEGMENTS 32
#define RINGS 16
#define PI 3.14159265358979323846f
#define FACETS 16
#define LENGTHS 8

struct mesh {
	voe_render_vertex *vertices;
	uint32_t vertex_count;
	uint32_t *indices;
	uint32_t index_count;
};

// The cylinder's point on rim column `column` (wrapped) at ring `ring`: radius
// 1, a quarter metre between rings. One sum per column, so strips meet exactly.
static voe_math_float3 rim(uint32_t column, uint32_t ring)
{
	const float phi = 2.0f * PI * (float)(column % FACETS) / FACETS;

	return (voe_math_float3){ cosf(phi), 0.25f * (float)ring, sinf(phi) };
}

// Side s is vertices s × 18 + edge × 9 + ring, edge 0 at column s and 1 at
// s + 1, all wearing the normal through the side's middle; uv and colour none.
static struct mesh cylinder(voe_base_arena *arena)
{
	struct mesh m = { .vertex_count = FACETS * 2 * (LENGTHS + 1),
			  .index_count = FACETS * LENGTHS * 6 };
	uint32_t at = 0;

	m.vertices = voe_base_arena_push(arena, sizeof *m.vertices * m.vertex_count);
	m.indices = voe_base_arena_push(arena, sizeof *m.indices * m.index_count);
	for (uint32_t s = 0; s < FACETS; s++) {
		const float middle = 2.0f * PI * ((float)s + 0.5f) / FACETS;
		const uint32_t base = s * 2 * (LENGTHS + 1);

		for (uint32_t v = 0; v < 2 * (LENGTHS + 1); v++) {
			m.vertices[base + v].position =
				rim(s + v / (LENGTHS + 1), v % (LENGTHS + 1));
			m.vertices[base + v].normal =
				(voe_math_float3){ cosf(middle), 0.0f, sinf(middle) };
		}
		for (uint32_t r = 0; r < LENGTHS; r++) {
			const uint32_t a = base + r;
			const uint32_t b = base + r + 1;
			const uint32_t c = base + LENGTHS + 1 + r;
			const uint32_t d = base + LENGTHS + 2 + r;
			const uint32_t quad[6] = { a, b, c, c, b, d };

			for (uint32_t k = 0; k < 6; k++)
				m.indices[at++] = quad[k];
		}
	}
	return m;
}

// Grid vertex (i, j) at x = i/32, z = j/32, facing up. With `seam`, the quads
// right of column 16 use a copy of it whose u is one more.
static uint32_t grid_vertex(uint32_t i, uint32_t j, bool right, bool seam)
{
	return seam && right && i == QUADS / 2 ? SIDE * SIDE + j : i * SIDE + j;
}

static struct mesh grid(voe_base_arena *arena, bool seam)
{
	struct mesh m = { .vertex_count = SIDE * SIDE + (seam ? SIDE : 0),
			  .index_count = QUADS * QUADS * 6 };

	m.vertices = voe_base_arena_push(arena, sizeof *m.vertices * m.vertex_count);
	m.indices = voe_base_arena_push(arena, sizeof *m.indices * m.index_count);
	for (uint32_t v = 0; v < m.vertex_count; v++) {
		const uint32_t i = v < SIDE * SIDE ? v / SIDE : QUADS / 2;
		const uint32_t j = v < SIDE * SIDE ? v % SIDE : v - SIDE * SIDE;

		m.vertices[v].position = (voe_math_float3){ (float)i / QUADS, 0.0f,
							    (float)j / QUADS };
		m.vertices[v].normal = (voe_math_float3){ 0.0f, 1.0f, 0.0f };
		m.vertices[v].uv = (voe_math_float2){
			(float)i / QUADS + (v < SIDE * SIDE ? 0.0f : 1.0f),
			(float)j / QUADS
		};
	}
	for (uint32_t i = 0, at = 0; i < QUADS; i++) {
		for (uint32_t j = 0; j < QUADS; j++) {
			const uint32_t a = grid_vertex(i, j, true, seam);
			const uint32_t b = grid_vertex(i, j + 1, true, seam);
			const uint32_t c = grid_vertex(i + 1, j, false, seam);
			const uint32_t d = grid_vertex(i + 1, j + 1, false, seam);
			const uint32_t quad[6] = { a, b, c, c, b, d };

			for (uint32_t k = 0; k < 6; k++)
				m.indices[at++] = quad[k];
		}
	}
	return m;
}

// Ring r of RINGS - 1 between the poles, segment s; 0 the top pole, the last
// vertex the bottom one.
static uint32_t sphere_vertex(uint32_t r, uint32_t s)
{
	return 1 + (r - 1) * SEGMENTS + s % SEGMENTS;
}

static struct mesh sphere(voe_base_arena *arena)
{
	struct mesh m = { .vertex_count = 2 + (RINGS - 1) * SEGMENTS,
			  .index_count = SEGMENTS * (RINGS - 1) * 6 };
	const uint32_t bottom = m.vertex_count - 1;
	uint32_t at = 0;

	m.vertices = voe_base_arena_push(arena, sizeof *m.vertices * m.vertex_count);
	m.indices = voe_base_arena_push(arena, sizeof *m.indices * m.index_count);
	m.vertices[0].position = (voe_math_float3){ 0.0f, 1.0f, 0.0f };
	m.vertices[bottom].position = (voe_math_float3){ 0.0f, -1.0f, 0.0f };
	for (uint32_t r = 1; r < RINGS; r++) {
		for (uint32_t s = 0; s < SEGMENTS; s++) {
			const float theta = PI * (float)r / RINGS;
			const float phi = 2.0f * PI * (float)s / SEGMENTS;

			m.vertices[sphere_vertex(r, s)].position =
				(voe_math_float3){ sinf(theta) * cosf(phi),
						   cosf(theta),
						   sinf(theta) * sinf(phi) };
		}
	}
	for (uint32_t v = 0; v < m.vertex_count; v++)
		m.vertices[v].normal = m.vertices[v].position;
	for (uint32_t s = 0; s < SEGMENTS; s++) {
		const uint32_t top[3] = { 0, sphere_vertex(1, s + 1),
					  sphere_vertex(1, s) };
		const uint32_t low[3] = { sphere_vertex(RINGS - 1, s),
					  sphere_vertex(RINGS - 1, s + 1), bottom };

		for (uint32_t k = 0; k < 3; k++)
			m.indices[at++] = top[k];
		for (uint32_t k = 0; k < 3; k++)
			m.indices[at++] = low[k];
		for (uint32_t r = 1; r + 1 < RINGS; r++) {
			const uint32_t band[6] = {
				sphere_vertex(r, s),	 sphere_vertex(r, s + 1),
				sphere_vertex(r + 1, s), sphere_vertex(r, s + 1),
				sphere_vertex(r + 1, s + 1), sphere_vertex(r + 1, s)
			};

			for (uint32_t k = 0; k < 6; k++)
				m.indices[at++] = band[k];
		}
	}
	return m;
}

static bool same_point(voe_math_float3 a, voe_math_float3 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

// Whole triangles, every index below the vertex count, none with two corners
// at one position.
static void check_triangles(const struct mesh *m, const uint32_t *out,
			    uint32_t count)
{
	VOE_TEST_CHECK_INT(count % 3, 0);
	for (uint32_t i = 0; i + 2 < count; i += 3) {
		if (out[i] >= m->vertex_count || out[i + 1] >= m->vertex_count ||
		    out[i + 2] >= m->vertex_count) {
			VOE_TEST_CHECK(false);
			return;
		}
		const voe_math_float3 a = m->vertices[out[i]].position;
		const voe_math_float3 b = m->vertices[out[i + 1]].position;
		const voe_math_float3 c = m->vertices[out[i + 2]].position;

		if (same_point(a, b) || same_point(b, c) || same_point(c, a)) {
			VOE_TEST_CHECK(false);
			return;
		}
	}
}

static bool uses_point(const struct mesh *m, const uint32_t *out,
		       uint32_t count, voe_math_float3 p)
{
	for (uint32_t i = 0; i < count; i++)
		if (same_point(m->vertices[out[i]].position, p))
			return true;
	return false;
}

// Every triangle's normal · its centroid above nought: facing out of a sphere
// about the origin.
static bool faces_out(const struct mesh *m, const uint32_t *out, uint32_t count)
{
	for (uint32_t i = 0; i + 2 < count; i += 3) {
		const voe_math_float3 a = m->vertices[out[i]].position;
		const voe_math_float3 b = m->vertices[out[i + 1]].position;
		const voe_math_float3 c = m->vertices[out[i + 2]].position;
		const float u[3] = { b.x - a.x, b.y - a.y, b.z - a.z };
		const float v[3] = { c.x - a.x, c.y - a.y, c.z - a.z };
		const float n[3] = { u[1] * v[2] - u[2] * v[1],
				     u[2] * v[0] - u[0] * v[2],
				     u[0] * v[1] - u[1] * v[0] };

		if (n[0] * (a.x + b.x + c.x) + n[1] * (a.y + b.y + c.y) +
			    n[2] * (a.z + b.z + c.z) <=
		    0.0f)
			return false;
	}
	return true;
}

static void flat_grid(voe_base_arena *arena)
{
	const struct mesh m = grid(arena, false);
	uint32_t *out = voe_base_arena_push(arena, sizeof *out * m.index_count);
	const uint32_t target = m.index_count / 8;
	float error = -1.0f;
	const uint32_t count = voe_3d_simplify(arena, m.vertices, m.vertex_count,
					       m.indices, m.index_count, target,
					       FLT_MAX, 0.0f, out, &error);
	const uint32_t corners[4] = { 0, QUADS, QUADS * SIDE, SIDE * SIDE - 1 };

	VOE_TEST_CHECK(count <= target);
	VOE_TEST_CHECK(count > 0);
	VOE_TEST_CHECK(error >= 0.0f && error < 1e-5f);
	for (uint32_t k = 0; k < 4; k++)
		VOE_TEST_CHECK(uses_point(&m, out, count,
					  m.vertices[corners[k]].position));
	check_triangles(&m, out, count);
}

static void round_sphere(voe_base_arena *arena)
{
	const struct mesh m = sphere(arena);
	uint32_t *out = voe_base_arena_push(arena, sizeof *out * m.index_count);
	const uint32_t target = m.index_count / 2;
	float error = -1.0f;
	uint32_t count;

	VOE_TEST_CHECK(faces_out(&m, m.indices, m.index_count));
	count = voe_3d_simplify(arena, m.vertices, m.vertex_count, m.indices,
				m.index_count, target, FLT_MAX, 0.0f, out, &error);
	VOE_TEST_CHECK(count <= target);
	VOE_TEST_CHECK(count > 0);
	VOE_TEST_CHECK(error > 0.0f && error < 0.1f);
	VOE_TEST_CHECK(faces_out(&m, out, count));
	check_triangles(&m, out, count);

	// 3: a limit no collapse on this sphere is under.
	count = voe_3d_simplify(arena, m.vertices, m.vertex_count, m.indices,
				m.index_count, target, 1e-6f, 0.0f, out, &error);
	VOE_TEST_CHECK(count > target + (m.index_count - target) / 2);
	VOE_TEST_CHECK(count <= m.index_count);
	VOE_TEST_CHECK(error <= 1e-6f);
	check_triangles(&m, out, count);
}

static void seamed_grid(voe_base_arena *arena)
{
	const struct mesh m = grid(arena, true);
	uint32_t *out = voe_base_arena_push(arena, sizeof *out * m.index_count);
	float error = -1.0f;
	const uint32_t count = voe_3d_simplify(arena, m.vertices, m.vertex_count,
					       m.indices, m.index_count,
					       m.index_count / 8, FLT_MAX, 0.0f, out,
					       &error);

	bool torn = false;

	VOE_TEST_CHECK(count < m.index_count);
	VOE_TEST_CHECK(uses_point(&m, out, count, m.vertices[SIDE * SIDE].position));
	VOE_TEST_CHECK(uses_point(&m, out, count,
				  m.vertices[SIDE * SIDE + QUADS].position));
	for (uint32_t i = 0; i + 2 < count; i += 3) {
		bool original = false;
		bool copy = false;

		for (uint32_t k = 0; k < 3; k++) {
			original |= out[i + k] / SIDE == QUADS / 2 &&
				    out[i + k] < SIDE * SIDE;
			copy |= out[i + k] >= SIDE * SIDE;
		}
		torn |= original && copy;
	}
	VOE_TEST_CHECK(!torn);
	check_triangles(&m, out, count);
}

static void faceted_cylinder(voe_base_arena *arena)
{
	const struct mesh m = cylinder(arena);
	uint32_t *out = voe_base_arena_push(arena, sizeof *out * m.index_count);
	float error = -1.0f;
	uint32_t count = voe_3d_simplify(arena, m.vertices, m.vertex_count,
					 m.indices, m.index_count, 96, 1e-3f, 0.0f,
					 out, &error);
	uint32_t hard;

	VOE_TEST_CHECK(count <= 96);
	VOE_TEST_CHECK(error >= 0.0f && error < 1e-5f);
	check_triangles(&m, out, count);

	// 6: the radius is 1.
	hard = voe_3d_simplify(arena, m.vertices, m.vertex_count, m.indices,
			       m.index_count, 48, 1.0f, 0.0f, out, &error);
	VOE_TEST_CHECK(hard >= 96);
	check_triangles(&m, out, hard);
	count = voe_3d_simplify(arena, m.vertices, m.vertex_count, m.indices,
				m.index_count, 48, 1.0f, 180.0f, out, &error);
	VOE_TEST_CHECK(count < hard);
	VOE_TEST_CHECK(error > 0.0f && error < 1.0f);
	check_triangles(&m, out, count);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(1024 * 1024);

	flat_grid(arena);
	round_sphere(arena);
	seamed_grid(arena);
	faceted_cylinder(arena);
	voe_base_arena_destroy(arena);
	return voe_test_result();
}
