// The simplifier of simplify.h, written out: weld, mark, plane sums, then a heap
// of collapses popped cheapest first until the target or the limit.
//
// THE WELD is an open-addressed table of positions at twice the vertex count, so
// it is at most half full and a probe never walks far. Each position keeps the
// first vertex that reached it, whose point it stands at. After it, each vertex
// is given its side: the first vertex at its position it may share one with.
//
// A POSITION'S TRIANGLES ARE A LIST OF CORNERS, one `next` per corner. A collapse
// re-points the moved position's corners at a vertex of the kept one and splices
// its list onto the kept one's; a dead triangle's corners stay in the lists and
// are skipped. A position of one side takes the vertex the collapsed edge's
// triangle names at the kept position, so uv and colour stay those of a real
// vertex. A seam line's corners go by side: each takes the vertex the edge's
// triangle of its own side names there.
//
// THE HEAP holds one entry per triangle edge pushed, the cheaper allowed way
// round, stamped with both positions' stamps. A collapse bumps both stamps and
// pushes the edges round the kept position again, so an entry whose stamps no
// longer match is stale and skipped when popped. Fresh entries are never more
// than three per live triangle, so a heap of three per triangle, compacted of
// stale entries when full, always has room.
//
// CONSTRAINTS: a border or seam edge count, a seam line's sides and the flip
// check walk a position's whole list, dead corners included, and a vertex's
// side is found by walking its position's sides, so a position of very high
// valence or many sides is slow; a list or an index per position would lift it.
// A position's kind is fixed when built; a move is checked again when popped.
// Costs and planes are in double.
#include "simplify.h"

#include <base/arena.h>
#include <base/assert.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// No position, no corner: the end of a list and an empty weld slot.
#define NONE UINT32_MAX

// What a position may do: move freely, move only along a border edge, move
// only along a seam edge by side, or never.
enum kind { FREE, BORDER, SEAM_LINE, FIXED };

// The PI the angle is turned into radians with.
#define PI 3.14159265358979323846

// A sum of weighted planes n·p + d = 0: the squared distance at p is
// pᵀAp + 2b·p + c, and weight is what the sum's planes weigh together.
struct quadric {
	double xx, xy, xz, yy, yz, zz;
	double x, y, z;
	double c;
	double weight;
};

// Moving `from` onto `to`, at a cost, while both stamps are these.
struct collapse {
	double cost;
	uint32_t from;
	uint32_t to;
	uint32_t from_stamp;
	uint32_t to_stamp;
};

// A seam line's move by side: the two live triangles on the edge, each one's
// side at the moved position and vertex at the kept one.
struct across {
	uint32_t side[2];
	uint32_t vertex[2];
};

// The welded mesh as it is simplified. Per vertex: position_of, side_of. Per
// position: vertex_at, kind, sides and seams (each counted up to three), stamp,
// first, quadrics. Per corner: corner (its vertex) and next. Per triangle:
// dead. `cosine` is the least dot of two unit normals in one side, `exact`
// when they must be equal.
struct mesh {
	const voe_render_vertex *vertices;
	uint32_t *position_of;
	uint32_t *side_of;
	uint32_t *vertex_at;
	uint8_t *kind;
	uint8_t *sides;
	uint8_t *seams;
	double cosine;
	bool exact;
	uint32_t *stamp;
	uint32_t *first;
	struct quadric *quadrics;
	uint32_t *corner;
	uint32_t *next;
	bool *dead;
	uint32_t triangle_count;
	uint32_t live;
	struct collapse *heap;
	uint32_t heap_count;
	uint32_t heap_capacity;
};

static uint32_t position_at(const struct mesh *m, uint32_t corner)
{
	return m->position_of[m->corner[corner]];
}

static void point(const struct mesh *m, uint32_t position, double p[3])
{
	const voe_math_float3 at = m->vertices[m->vertex_at[position]].position;

	p[0] = at.x;
	p[1] = at.y;
	p[2] = at.z;
}

// cross(b - a, c - a): the triangle's normal, twice its area long.
static void normal_of(const double a[3], const double b[3], const double c[3],
		      double n[3])
{
	const double u[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
	const double v[3] = { c[0] - a[0], c[1] - a[1], c[2] - a[2] };

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}

static double dot(const double a[3], const double b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// The corner of triangle `t` at `position`, or NONE.
static uint32_t corner_of(const struct mesh *m, uint32_t t, uint32_t position)
{
	for (uint32_t k = 0; k < 3; k++)
		if (position_at(m, t * 3 + k) == position)
			return t * 3 + k;
	return NONE;
}

// How many live triangles have both positions as corners.
static uint32_t shared(const struct mesh *m, uint32_t a, uint32_t b)
{
	uint32_t count = 0;

	for (uint32_t c = m->first[a]; c != NONE; c = m->next[c])
		if (!m->dead[c / 3] && corner_of(m, c / 3, b) != NONE)
			count++;
	return count;
}

// Adds the plane through `on` with unit normal `n`, at `weight`.
static void add_plane(struct quadric *q, const double n[3], const double on[3],
		      double weight)
{
	const double d = -dot(n, on);

	q->xx += weight * n[0] * n[0];
	q->xy += weight * n[0] * n[1];
	q->xz += weight * n[0] * n[2];
	q->yy += weight * n[1] * n[1];
	q->yz += weight * n[1] * n[2];
	q->zz += weight * n[2] * n[2];
	q->x += weight * n[0] * d;
	q->y += weight * n[1] * d;
	q->z += weight * n[2] * d;
	q->c += weight * d * d;
	q->weight += weight;
}

static void absorb(struct quadric *into, const struct quadric *q)
{
	into->xx += q->xx;
	into->xy += q->xy;
	into->xz += q->xz;
	into->yy += q->yy;
	into->yz += q->yz;
	into->zz += q->zz;
	into->x += q->x;
	into->y += q->y;
	into->z += q->z;
	into->c += q->c;
	into->weight += q->weight;
}

// The weighted sum of squared distances from p to the quadric's planes.
static double distances(const struct quadric *q, const double p[3])
{
	return q->xx * p[0] * p[0] + q->yy * p[1] * p[1] + q->zz * p[2] * p[2] +
	       2.0 * (q->xy * p[0] * p[1] + q->xz * p[0] * p[2] +
		      q->yz * p[1] * p[2]) +
	       2.0 * (q->x * p[0] + q->y * p[1] + q->z * p[2]) + q->c;
}

// The weighted mean squared distance of both positions' planes at `to`.
static double cost(const struct mesh *m, uint32_t from, uint32_t to)
{
	const struct quadric *a = &m->quadrics[from];
	const struct quadric *b = &m->quadrics[to];
	const double weight = a->weight + b->weight;
	double p[3];

	if (weight <= 0.0)
		return 0.0;
	point(m, to, p);
	return fmax(0.0, (distances(a, p) + distances(b, p)) / weight);
}

// Fills `out` for moving `from` along the edge onto `to`; false when the edge
// is not a seam edge or a corner that stays has a side neither triangle names.
static bool seam_across(const struct mesh *m, uint32_t from, uint32_t to,
			struct across *out)
{
	uint32_t found = 0;

	for (uint32_t c = m->first[from]; c != NONE; c = m->next[c]) {
		const uint32_t at = m->dead[c / 3] ? NONE : corner_of(m, c / 3, to);

		if (at == NONE)
			continue;
		if (found == 2)
			return false;
		out->side[found] = m->side_of[m->corner[c]];
		out->vertex[found++] = m->corner[at];
	}
	if (found != 2 || (out->side[0] == out->side[1] &&
			   m->side_of[out->vertex[0]] == m->side_of[out->vertex[1]]))
		return false;
	for (uint32_t c = m->first[from]; c != NONE; c = m->next[c]) {
		const uint32_t side = m->side_of[m->corner[c]];

		if (m->dead[c / 3] || corner_of(m, c / 3, to) != NONE)
			continue;
		if (side != out->side[0] && side != out->side[1])
			return false;
	}
	return true;
}

// Whether `from` may move onto `to` as the kinds say.
static bool may_move(const struct mesh *m, uint32_t from, uint32_t to)
{
	struct across across;

	if (m->kind[from] == FIXED)
		return false;
	if (m->kind[from] == SEAM_LINE)
		return seam_across(m, from, to, &across);
	return m->kind[from] == FREE || shared(m, from, to) != 2;
}

// Whether moving `from` onto `to` keeps every triangle that remains within 90°
// of where it faced; one flattened to nothing counts as turned.
static bool keeps_facing(const struct mesh *m, uint32_t from, uint32_t to)
{
	double target[3];

	point(m, to, target);
	for (uint32_t c = m->first[from]; c != NONE; c = m->next[c]) {
		const uint32_t t = c / 3;
		const uint32_t k = c % 3;
		double a[3], b[3], d[3], before[3], after[3];

		if (m->dead[t] || corner_of(m, t, to) != NONE)
			continue;
		point(m, position_at(m, c), a);
		point(m, position_at(m, t * 3 + (k + 1) % 3), b);
		point(m, position_at(m, t * 3 + (k + 2) % 3), d);
		normal_of(a, b, d, before);
		normal_of(target, b, d, after);
		if (dot(before, after) <= 0.0)
			return false;
	}
	return true;
}

static void sift_up(struct collapse *heap, uint32_t i)
{
	while (i > 0 && heap[i].cost < heap[(i - 1) / 2].cost) {
		const struct collapse swap = heap[i];

		heap[i] = heap[(i - 1) / 2];
		heap[(i - 1) / 2] = swap;
		i = (i - 1) / 2;
	}
}

static void sift_down(struct collapse *heap, uint32_t count, uint32_t i)
{
	for (;;) {
		uint32_t least = i;
		const uint32_t left = i * 2 + 1;
		struct collapse swap;

		if (left < count && heap[left].cost < heap[least].cost)
			least = left;
		if (left + 1 < count && heap[left + 1].cost < heap[least].cost)
			least = left + 1;
		if (least == i)
			return;
		swap = heap[i];
		heap[i] = heap[least];
		heap[least] = swap;
		i = least;
	}
}

static bool fresh(const struct mesh *m, const struct collapse *c)
{
	return m->stamp[c->from] == c->from_stamp &&
	       m->stamp[c->to] == c->to_stamp;
}

// Drops every stale entry and heaps what is left.
static void compact(struct mesh *m)
{
	uint32_t kept = 0;

	for (uint32_t i = 0; i < m->heap_count; i++)
		if (fresh(m, &m->heap[i]))
			m->heap[kept++] = m->heap[i];
	m->heap_count = kept;
	for (uint32_t i = kept / 2; i-- > 0;)
		sift_down(m->heap, kept, i);
}

static struct collapse collapse_of(const struct mesh *m, uint32_t from,
				   uint32_t to)
{
	return (struct collapse){ .cost = cost(m, from, to),
				  .from = from,
				  .to = to,
				  .from_stamp = m->stamp[from],
				  .to_stamp = m->stamp[to] };
}

// Pushes the edge between two positions the cheaper way round it may go.
static void push_edge(struct mesh *m, uint32_t a, uint32_t b)
{
	const bool ab = may_move(m, a, b);
	const bool ba = may_move(m, b, a);
	struct collapse c;

	if (!ab && !ba)
		return;
	c = ab ? collapse_of(m, a, b) : collapse_of(m, b, a);
	if (ab && ba) {
		const struct collapse other = collapse_of(m, b, a);

		if (other.cost < c.cost)
			c = other;
	}
	if (m->heap_count == m->heap_capacity)
		compact(m);
	VOE_BASE_ASSERT(m->heap_count < m->heap_capacity,
			"fresh entries are at most three per live triangle");
	m->heap[m->heap_count] = c;
	sift_up(m->heap, m->heap_count++);
}

// The cheapest fresh entry, taken off the heap; false when there is none.
static bool pop(struct mesh *m, struct collapse *out)
{
	while (m->heap_count > 0) {
		*out = m->heap[0];
		m->heap[0] = m->heap[--m->heap_count];
		sift_down(m->heap, m->heap_count, 0);
		if (fresh(m, out))
			return true;
	}
	return false;
}

// Pushes the two edges at `position` of every live triangle round it.
static void push_around(struct mesh *m, uint32_t position)
{
	for (uint32_t c = m->first[position]; c != NONE; c = m->next[c]) {
		const uint32_t t = c / 3;
		const uint32_t k = c % 3;

		if (m->dead[t])
			continue;
		push_edge(m, position, position_at(m, t * 3 + (k + 1) % 3));
		push_edge(m, position, position_at(m, t * 3 + (k + 2) % 3));
	}
}

// The vertex a live triangle naming both positions has at `to`.
static uint32_t vertex_across(const struct mesh *m, uint32_t from, uint32_t to)
{
	for (uint32_t c = m->first[from]; c != NONE; c = m->next[c]) {
		const uint32_t at = m->dead[c / 3] ? NONE : corner_of(m, c / 3, to);

		if (at != NONE)
			return m->corner[at];
	}
	VOE_BASE_ASSERT(false, "a fresh entry's edge has a live triangle");
	return NONE;
}

// Moves `from` onto `to`: the triangles naming both die, the rest name
// `to`'s vertex, by side on a seam line, the lists and quadrics join, and
// `to`'s edges are pushed anew.
static void collapse(struct mesh *m, uint32_t from, uint32_t to)
{
	const bool by_side = m->kind[from] == SEAM_LINE;
	struct across across = { { NONE, NONE }, { NONE, NONE } };
	uint32_t vertex = NONE;
	uint32_t last = NONE;

	if (by_side) {
		const bool moves = seam_across(m, from, to, &across);

		VOE_BASE_ASSERT(moves, "a seam line's move is checked when popped");
	} else {
		vertex = vertex_across(m, from, to);
	}
	for (uint32_t c = m->first[from]; c != NONE; c = m->next[c]) {
		last = c;
		if (m->dead[c / 3])
			continue;
		if (corner_of(m, c / 3, to) != NONE) {
			m->dead[c / 3] = true;
			m->live--;
			continue;
		}
		if (by_side)
			vertex = m->side_of[m->corner[c]] == across.side[0] ?
					 across.vertex[0] :
					 across.vertex[1];
		m->corner[c] = vertex;
	}
	absorb(&m->quadrics[to], &m->quadrics[from]);
	if (last != NONE) {
		m->next[last] = m->first[to];
		m->first[to] = m->first[from];
	}
	m->first[from] = NONE;
	m->stamp[from]++;
	m->stamp[to]++;
	push_around(m, to);
}

static uint32_t hash_point(voe_math_float3 p)
{
	// Adding nought turns -0 into +0, which == already counts as the same.
	const float xyz[3] = { p.x + 0.0f, p.y + 0.0f, p.z + 0.0f };
	uint32_t bits[3];

	memcpy(bits, xyz, sizeof bits);
	return bits[0] * 73856093u ^ bits[1] * 19349663u ^ bits[2] * 83492791u;
}

static bool same_point(voe_math_float3 a, voe_math_float3 b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

// Fills position_of and vertex_at; a position is new when its point is.
static void weld(struct mesh *m, voe_base_arena *scratch, uint32_t vertex_count)
{
	uint32_t slots = 2;
	uint32_t *table;
	uint32_t positions = 0;

	while (slots < vertex_count * 2u)
		slots *= 2;
	table = voe_base_arena_push(scratch, sizeof *table * slots);
	memset(table, 0xff, sizeof *table * slots);
	for (uint32_t v = 0; v < vertex_count; v++) {
		const voe_math_float3 p = m->vertices[v].position;
		uint32_t at = hash_point(p) & (slots - 1);

		while (table[at] != NONE &&
		       !same_point(m->vertices[m->vertex_at[table[at]]].position,
				   p))
			at = (at + 1) & (slots - 1);
		if (table[at] == NONE) {
			table[at] = positions;
			m->vertex_at[positions++] = v;
		}
		m->position_of[v] = table[at];
	}
}

// Whether vertex `v` joins the side whose first vertex is `first`: equal uv and
// colour complement, normals equal or within the angle.
static bool joins_side(const struct mesh *m, uint32_t first, uint32_t v)
{
	const voe_render_vertex *a = &m->vertices[first];
	const voe_render_vertex *b = &m->vertices[v];
	const double n[3] = { a->normal.x, a->normal.y, a->normal.z };
	const double o[3] = { b->normal.x, b->normal.y, b->normal.z };

	if (memcmp(&a->uv, &b->uv, sizeof a->uv) != 0 ||
	    memcmp(a->colour_complement, b->colour_complement,
		   sizeof a->colour_complement) != 0)
		return false;
	if (m->exact)
		return memcmp(&a->normal, &b->normal, sizeof a->normal) == 0;
	return dot(n, o) >= m->cosine * sqrt(dot(n, n) * dot(o, o));
}

// Gives each vertex its side, the first vertex at its position it joins, and
// counts each position's sides; `first` and `next` list a position's sides.
static void mark_sides(struct mesh *m, voe_base_arena *scratch,
		       uint32_t vertex_count)
{
	uint32_t *first = voe_base_arena_push(scratch, sizeof *first * vertex_count);
	uint32_t *next = voe_base_arena_push(scratch, sizeof *next * vertex_count);

	memset(first, 0xff, sizeof *first * vertex_count);
	for (uint32_t v = 0; v < vertex_count; v++) {
		const uint32_t p = m->position_of[v];
		uint32_t side = first[p];
		uint32_t last = NONE;

		while (side != NONE && !joins_side(m, side, v)) {
			last = side;
			side = next[side];
		}
		if (side == NONE) {
			side = v;
			next[v] = NONE;
			if (last == NONE)
				first[p] = v;
			else
				next[last] = v;
			m->sides[p] += m->sides[p] < 3;
		}
		m->side_of[v] = side;
	}
}

// Whether the edge a–b of live triangle `t`, shared by two, is a seam edge seen
// from the earlier of its triangles: the other names a different side at either end.
static bool seam_edge_first(const struct mesh *m, uint32_t t, uint32_t a,
			    uint32_t b)
{
	for (uint32_t c = m->first[a]; c != NONE; c = m->next[c]) {
		const uint32_t u = c / 3;
		const uint32_t at = m->dead[u] || u == t ? NONE : corner_of(m, u, b);

		if (at == NONE)
			continue;
		return u > t &&
		       (m->side_of[m->corner[c]] !=
				m->side_of[m->corner[corner_of(m, t, a)]] ||
			m->side_of[m->corner[at]] !=
				m->side_of[m->corner[corner_of(m, t, b)]]);
	}
	return false;
}

// One side stays free or border as add_planes left it; two sides on exactly two
// seam edges and no border are a seam line; anything else is fixed.
static void mark_kinds(struct mesh *m, uint32_t vertex_count)
{
	for (uint32_t p = 0; p < vertex_count; p++) {
		if (m->sides[p] <= 1)
			continue;
		m->kind[p] = m->sides[p] == 2 && m->seams[p] == 2 &&
					     m->kind[p] == FREE ?
				     SEAM_LINE :
				     FIXED;
	}
}

// Copies the corners, drops a triangle with two at one position and lists the
// rest at their positions.
static void link_corners(struct mesh *m, const uint32_t *indices)
{
	for (uint32_t t = 0; t < m->triangle_count; t++) {
		for (uint32_t k = 0; k < 3; k++)
			m->corner[t * 3 + k] = indices[t * 3 + k];
		m->dead[t] = position_at(m, t * 3) == position_at(m, t * 3 + 1) ||
			     position_at(m, t * 3 + 1) == position_at(m, t * 3 + 2) ||
			     position_at(m, t * 3 + 2) == position_at(m, t * 3);
		if (m->dead[t])
			continue;
		m->live++;
		for (uint32_t k = 0; k < 3; k++) {
			const uint32_t p = position_at(m, t * 3 + k);

			m->next[t * 3 + k] = m->first[p];
			m->first[p] = t * 3 + k;
		}
	}
}

// Each live triangle's plane, weighted by its area, into its three positions;
// along each border edge the upright plane, weighted by the length squared, into
// both ends, which become borders; each seam edge counted once at both ends.
static void add_planes(struct mesh *m)
{
	for (uint32_t t = 0; t < m->triangle_count; t++) {
		double p[3][3], n[3], length;

		if (m->dead[t])
			continue;
		for (uint32_t k = 0; k < 3; k++)
			point(m, position_at(m, t * 3 + k), p[k]);
		normal_of(p[0], p[1], p[2], n);
		length = sqrt(dot(n, n));
		if (length <= 0.0)
			continue;
		for (uint32_t k = 0; k < 3; k++) {
			const double unit[3] = { n[0] / length, n[1] / length,
						 n[2] / length };

			add_plane(&m->quadrics[position_at(m, t * 3 + k)], unit,
				  p[0], length / 2.0);
		}
		for (uint32_t k = 0; k < 3; k++) {
			const uint32_t a = position_at(m, t * 3 + k);
			const uint32_t b = position_at(m, t * 3 + (k + 1) % 3);
			const double edge[3] = { p[(k + 1) % 3][0] - p[k][0],
						 p[(k + 1) % 3][1] - p[k][1],
						 p[(k + 1) % 3][2] - p[k][2] };
			const double along[3] = { p[k][0] + n[0], p[k][1] + n[1],
						  p[k][2] + n[2] };
			double upright[3], size;

			if (shared(m, a, b) == 2) {
				if (seam_edge_first(m, t, a, b)) {
					m->seams[a] += m->seams[a] < 3;
					m->seams[b] += m->seams[b] < 3;
				}
				continue;
			}
			normal_of(p[k], p[(k + 1) % 3], along, upright);
			size = sqrt(dot(upright, upright));
			for (uint32_t i = 0; i < 3; i++)
				upright[i] /= size;
			add_plane(&m->quadrics[a], upright, p[k], dot(edge, edge));
			add_plane(&m->quadrics[b], upright, p[k], dot(edge, edge));
			m->kind[a] = BORDER;
			m->kind[b] = BORDER;
		}
	}
}

// The angle 180 gives a cosine below any dot of two normals over their lengths,
// so rounding never parts two opposite ones.
static void build(struct mesh *m, voe_base_arena *scratch,
		  const voe_render_vertex *vertices, uint32_t vertex_count,
		  const uint32_t *indices, uint32_t index_count,
		  float edge_degrees)
{
	*m = (struct mesh){ .vertices = vertices,
			    .triangle_count = index_count / 3,
			    .exact = edge_degrees <= 0.0f,
			    .cosine = edge_degrees >= 180.0f ?
					      -2.0 :
					      cos(edge_degrees * PI / 180.0) };
	m->position_of = voe_base_arena_push(scratch, sizeof(uint32_t) * vertex_count);
	m->side_of = voe_base_arena_push(scratch, sizeof(uint32_t) * vertex_count);
	m->vertex_at = voe_base_arena_push(scratch, sizeof(uint32_t) * vertex_count);
	m->kind = voe_base_arena_push(scratch, vertex_count);
	m->sides = voe_base_arena_push(scratch, vertex_count);
	m->seams = voe_base_arena_push(scratch, vertex_count);
	m->stamp = voe_base_arena_push(scratch, sizeof(uint32_t) * vertex_count);
	m->first = voe_base_arena_push(scratch, sizeof(uint32_t) * vertex_count);
	m->quadrics = voe_base_arena_push(scratch,
					  sizeof(struct quadric) * vertex_count);
	m->corner = voe_base_arena_push(scratch, sizeof(uint32_t) * index_count);
	m->next = voe_base_arena_push(scratch, sizeof(uint32_t) * index_count);
	m->dead = voe_base_arena_push(scratch, m->triangle_count);
	m->heap_capacity = index_count + 1;
	m->heap = voe_base_arena_push(scratch,
				      sizeof(struct collapse) * m->heap_capacity);
	memset(m->first, 0xff, sizeof(uint32_t) * vertex_count);
	weld(m, scratch, vertex_count);
	mark_sides(m, scratch, vertex_count);
	link_corners(m, indices);
	add_planes(m);
	mark_kinds(m, vertex_count);
	for (uint32_t t = 0; t < m->triangle_count; t++)
		for (uint32_t k = 0; k < 3 && !m->dead[t]; k++)
			push_edge(m, position_at(m, t * 3 + k),
				  position_at(m, t * 3 + (k + 1) % 3));
}

uint32_t voe_3d_simplify(voe_base_arena *scratch,
			 const voe_render_vertex *vertices,
			 uint32_t vertex_count, const uint32_t *indices,
			 uint32_t index_count, uint32_t target, float max_error,
			 float edge_degrees, uint32_t *out, float *error)
{
	const struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	struct mesh m;
	struct collapse next;
	double largest = 0.0;
	uint32_t written = 0;

	VOE_BASE_ASSERT(index_count % 3 == 0, "indices are whole triangles");
	VOE_BASE_ASSERT(vertex_count < UINT32_MAX / 2, "the weld doubles it");
	VOE_BASE_ASSERT(edge_degrees >= 0.0f && edge_degrees <= 180.0f,
			"an angle between normals is 0 to 180");
	for (uint32_t i = 0; i < index_count; i++)
		VOE_BASE_ASSERT(indices[i] < vertex_count,
				"an index names one of the vertices");
	build(&m, scratch, vertices, vertex_count, indices, index_count,
	      edge_degrees);
	while (m.live * 3 > target && pop(&m, &next)) {
		if (sqrt(next.cost) > (double)max_error)
			break;
		if (!may_move(&m, next.from, next.to) ||
		    !keeps_facing(&m, next.from, next.to))
			continue;
		largest = fmax(largest, next.cost);
		collapse(&m, next.from, next.to);
	}
	for (uint32_t t = 0; t < m.triangle_count; t++)
		for (uint32_t k = 0; k < 3 && !m.dead[t]; k++)
			out[written++] = m.corner[t * 3 + k];
	*error = (float)sqrt(largest);
	voe_base_arena_rewind(scratch, mark);
	return written;
}
