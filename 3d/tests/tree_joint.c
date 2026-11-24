// A trunk joint's rings (3d/src/tree_joint.h, ADR-0519) on a straight trunk
// stretch along +Y of radius 0.2, a branch of radius 0.08 leaving at 30°, 60°
// and 110°, on an 8-sided and a 3-sided trunk. At share 0 every vertex lies on
// the faceted surface with the trunk's radial normal and uv there; at every
// share none is inside it; at share 1 the ring is the branch's plain ring; the
// base ring is longer along the trunk than around; E − J makes θ/2 with the
// trunk; and a base ring across the trunk's u seam has no jump in u. A first
// segment that never leaves the trunk has no joint.
// The faceted surface is found here by meeting each polygon edge, not by the
// joint's own reading of a facet. Needs no graphics card.
#include "../src/tree_joint.h"

#include <math/float2.h>
#include <math/float3.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define TAU 6.28318531f
#define TRUNK_R 0.2f
#define BRANCH_R 0.08f
#define BRANCH_SIDES 8
#define RING (BRANCH_SIDES + 1)
// 0.1 mm.
#define NEAR 1e-4f
#define SHARES 5

static const voe_math_float3 at_0 = { 0.0f, 0.0f, -1.0f };
static const voe_math_float3 at_90 = { -1.0f, 0.0f, 0.0f };

// The trunk as tree_mesh.c frames a tube along +Y, its v starting at 0.5.
static voe_3d_tree_stretch stretch_of(uint32_t sides)
{
	return (voe_3d_tree_stretch){
		.lower = { { 0.0f, 0.0f, 0.0f }, TRUNK_R, 0.5f },
		.upper = { { 0.0f, 4.0f, 0.0f }, TRUNK_R, 4.5f },
		.at_0 = at_0,
		.at_90 = at_90,
		.sides = sides,
		.turns = fmaxf(1.0f, roundf(TAU * TRUNK_R)),
	};
}

static voe_3d_tree_segment trunk_of(void)
{
	return (voe_3d_tree_segment){ { 0.0f, 0.0f, 0.0f }, { 0.0f, 4.0f, 0.0f },
				      TRUNK_R, TRUNK_R, UINT32_MAX, 0 };
}

// A branch from the trunk's axis at 2 m, `degrees` from +Y toward the trunk
// frame's angle `heading`, `length` long.
static voe_3d_tree_segment branch_of(float degrees, float heading,
				     float length)
{
	const float a = degrees * TAU / 360.0f;
	const voe_math_float3 out = voe_math_float3_add(
		voe_math_float3_scale(at_0, cosf(heading)),
		voe_math_float3_scale(at_90, sinf(heading)));
	const voe_math_float3 d = voe_math_float3_add(
		(voe_math_float3){ 0.0f, cosf(a), 0.0f },
		voe_math_float3_scale(out, sinf(a)));
	const voe_math_float3 start = { 0.0f, 2.0f, 0.0f };

	return (voe_3d_tree_segment){
		start, voe_math_float3_add(start, voe_math_float3_scale(d, length)),
		BRANCH_R, 0.06f, 0, 1
	};
}

// The branch's unit ring offsets as tree_mesh.c frames its first ring.
static void offsets_of(voe_3d_tree_segment b, voe_math_float3 *o)
{
	const voe_math_float3 d =
		voe_math_float3_normalize(voe_math_float3_sub(b.end, b.start));
	const voe_math_float3 ref = fabsf(d.y) < 0.9f ?
					    (voe_math_float3){ 0.0f, 1.0f, 0.0f } :
					    (voe_math_float3){ 1.0f, 0.0f, 0.0f };
	const voe_math_float3 n =
		voe_math_float3_normalize(voe_math_float3_cross(d, ref));
	const voe_math_float3 w = voe_math_float3_cross(d, n);

	for (uint32_t k = 0; k < RING; k++) {
		const float a = TAU * (float)k / (float)BRANCH_SIDES;

		o[k] = voe_math_float3_add(voe_math_float3_scale(n, cosf(a)),
					   voe_math_float3_scale(w, sinf(a)));
	}
}

// What the faceted trunk is at `p`'s height and direction from the axis: the
// distance to its surface, its radial normal, the edge's interpolated u (in
// 0..turns) and v, and `p`'s own distance.
typedef struct {
	float surface;
	float distance;
	voe_math_float3 normal;
	voe_math_float2 uv;
} trunk_seen;

static trunk_seen seen_at(const voe_3d_tree_stretch *s, voe_math_float3 p)
{
	const voe_math_float2 flat = { voe_math_float3_dot(p, s->at_0),
				       voe_math_float3_dot(p, s->at_90) };
	const float distance = voe_math_float2_length(flat);
	const voe_math_float2 e = voe_math_float2_scale(flat, 1.0f / distance);
	trunk_seen seen = {
		.distance = distance,
		.normal = voe_math_float3_add(
			voe_math_float3_scale(s->at_0, e.x),
			voe_math_float3_scale(s->at_90, e.y)),
	};

	for (uint32_t k = 0; k < s->sides; k++) {
		const float a0 = TAU * (float)k / (float)s->sides;
		const float a1 = TAU * (float)(k + 1) / (float)s->sides;
		const voe_math_float2 v = { TRUNK_R * cosf(a0), TRUNK_R * sinf(a0) };
		const voe_math_float2 edge = {
			TRUNK_R * cosf(a1) - v.x, TRUNK_R * sinf(a1) - v.y
		};
		const float ed = e.x * edge.y - e.y * edge.x;
		float mu, lambda;

		if (fabsf(ed) < 1e-9f)
			continue;
		mu = (v.x * e.y - v.y * e.x) / ed;
		lambda = (v.x * edge.y - v.y * edge.x) / ed;
		if (mu < -1e-5f || mu > 1.0f + 1e-5f || lambda <= 0.0f)
			continue;
		seen.surface = lambda;
		seen.uv = (voe_math_float2){
			s->turns * ((float)k + mu) / (float)s->sides,
			s->lower.v + (s->upper.v - s->lower.v) * p.y / 4.0f
		};
	}
	return seen;
}

static bool whole_apart(float a, float b)
{
	return fabsf(a - b - roundf(a - b)) <= NEAR;
}

static float degrees_between(voe_math_float3 a, voe_math_float3 b)
{
	const float c = voe_math_float3_dot(voe_math_float3_normalize(a),
					    voe_math_float3_normalize(b));

	return acosf(fminf(fmaxf(c, -1.0f), 1.0f)) * 360.0f / TAU;
}

static void ring_at(const voe_3d_tree_joint *j, const voe_3d_tree_stretch *s,
		    const voe_math_float3 *o, float share, voe_math_float3 *p,
		    voe_math_float3 *n, voe_math_float2 *uv)
{
	voe_3d_tree_joint_ring(j, s, o, RING, BRANCH_R, 1.0f, 7.0f, share, p, n,
			       uv);
}

static void the_base_ring_lies_on_the_trunk(const voe_3d_tree_joint *j,
					    const voe_3d_tree_stretch *s,
					    const voe_math_float3 *o)
{
	voe_math_float3 p[RING], n[RING];
	voe_math_float2 uv[RING];
	float along[2] = { 1e9f, -1e9f }, around[2] = { 1e9f, -1e9f };

	ring_at(j, s, o, 0.0f, p, n, uv);
	for (uint32_t k = 0; k < RING; k++) {
		const trunk_seen seen = seen_at(s, p[k]);
		const float arc = TRUNK_R *
				  remainderf(atan2f(p[k].x, p[k].z) -
						     atan2f(j->centre.x,
							    j->centre.z),
					     TAU);

		VOE_TEST_CHECK(fabsf(seen.distance - seen.surface) <= NEAR);
		VOE_TEST_CHECK(degrees_between(n[k], seen.normal) <= 0.5f);
		VOE_TEST_CHECK(whole_apart(uv[k].x, seen.uv.x));
		VOE_TEST_CHECK(fabsf(uv[k].y - seen.uv.y) <= NEAR);
		along[0] = fminf(along[0], p[k].y);
		along[1] = fmaxf(along[1], p[k].y);
		around[0] = fminf(around[0], arc);
		around[1] = fmaxf(around[1], arc);
	}
	VOE_TEST_CHECK(along[1] - along[0] > around[1] - around[0]);
}

static void no_ring_is_inside_the_trunk(const voe_3d_tree_joint *j,
					const voe_3d_tree_stretch *s,
					const voe_math_float3 *o)
{
	static const float shares[SHARES] = { 0.0f, 1.0f / 3.0f, 0.5f,
					      2.0f / 3.0f, 1.0f };

	for (uint32_t i = 0; i < SHARES; i++) {
		voe_math_float3 p[RING], n[RING];
		voe_math_float2 uv[RING];

		ring_at(j, s, o, shares[i], p, n, uv);
		for (uint32_t k = 0; k < RING; k++) {
			const trunk_seen seen = seen_at(s, p[k]);

			VOE_TEST_CHECK(seen.distance >= seen.surface - NEAR);
		}
	}
}

static void the_last_ring_is_the_branch_s(const voe_3d_tree_joint *j,
					  const voe_3d_tree_stretch *s,
					  const voe_math_float3 *o)
{
	voe_math_float3 p[RING], n[RING];
	voe_math_float2 uv[RING];

	ring_at(j, s, o, 1.0f, p, n, uv);
	for (uint32_t k = 0; k < RING; k++) {
		const voe_math_float3 plain = voe_math_float3_add(
			j->end, voe_math_float3_scale(o[k], BRANCH_R));

		VOE_TEST_CHECK(voe_math_float3_length(voe_math_float3_sub(
				       p[k], plain)) <= 1e-5f);
		VOE_TEST_CHECK(voe_math_float3_length(voe_math_float3_sub(
				       n[k], o[k])) <= 1e-5f);
		VOE_TEST_CHECK_FLOAT(uv[k].x, (float)k / BRANCH_SIDES, 1e-6f);
		VOE_TEST_CHECK_FLOAT(uv[k].y, 7.0f, 1e-6f);
	}
}

static void a_joint_at(float degrees, uint32_t sides)
{
	const voe_3d_tree_stretch s = stretch_of(sides);
	const voe_3d_tree_segment b = branch_of(degrees, 1.0f, 1.5f);
	voe_math_float3 o[RING];
	voe_3d_tree_joint j;

	printf("branch at %g degrees on %u sides\n", (double)degrees, sides);
	offsets_of(b, o);
	VOE_TEST_CHECK(voe_3d_tree_joint_of(trunk_of(), b, &j));
	VOE_TEST_CHECK_FLOAT(degrees_between(voe_math_float3_sub(j.end, j.centre),
					     (voe_math_float3){ 0, 1, 0 }),
			     0.5f * degrees, 0.5f);
	the_base_ring_lies_on_the_trunk(&j, &s, o);
	no_ring_is_inside_the_trunk(&j, &s, o);
	the_last_ring_is_the_branch_s(&j, &s, o);
}

// J straight out along at_0, the trunk's u seam; the trunk's own u jumps
// there and the base ring's does not.
static void no_jump_across_the_seam(uint32_t sides)
{
	const voe_3d_tree_stretch s = stretch_of(sides);
	const voe_3d_tree_segment b = branch_of(60.0f, 0.0f, 1.5f);
	voe_math_float3 o[RING], p[RING], n[RING];
	voe_math_float2 uv[RING];
	voe_3d_tree_joint j;
	bool trunk_jumps = false;

	offsets_of(b, o);
	VOE_TEST_CHECK(voe_3d_tree_joint_of(trunk_of(), b, &j));
	ring_at(&j, &s, o, 0.0f, p, n, uv);
	for (uint32_t k = 0; k + 1 < RING; k++) {
		const float own = fabsf(seen_at(&s, p[k + 1]).uv.x -
					seen_at(&s, p[k]).uv.x);

		trunk_jumps |= own > 0.5f * s.turns;
		VOE_TEST_CHECK(fabsf(uv[k + 1].x - uv[k].x) <= 0.5f * s.turns);
	}
	VOE_TEST_CHECK(trunk_jumps);
}

static void a_branch_inside_has_no_joint(void)
{
	voe_3d_tree_joint j;

	VOE_TEST_CHECK(!voe_3d_tree_joint_of(trunk_of(),
					     branch_of(60.0f, 1.0f, 0.1f), &j));
}

int main(void)
{
	static const float angles[3] = { 30.0f, 60.0f, 110.0f };
	static const uint32_t sides[2] = { 8, 3 };

	for (uint32_t s = 0; s < 2; s++) {
		for (uint32_t a = 0; a < 3; a++)
			a_joint_at(angles[a], sides[s]);
		no_jump_across_the_seam(sides[s]);
	}
	a_branch_inside_has_no_joint();
	return voe_test_result();
}
