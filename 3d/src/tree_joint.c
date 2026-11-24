// A trunk joint's J, E, A and B from two skeleton segments, and its collar's
// rings on a level's trunk stretch (tree_joint.h, ADR-0519).
//
// J0 is found by bisection along the branch's first segment between its start
// on the trunk's axis and its end; a point is read on a stretch by its height
// between the rings and its angle in the lower ring's frame, then snapped onto
// the facet that angle falls on.
//
// Constraints: the bisection is LEAVE_STEPS halvings, about float precision
// over any segment; it finds one crossing, the first only while the trunk's
// radius changes slower along the branch than the branch leaves the axis.
#include "tree_joint.h"

#include <base/assert.h>

#include <math.h>
#include <stddef.h>

#define TAU 6.28318531f
#define LEAVE_STEPS 32

// A point read on a stretch: its height as a share from the lower ring to the
// upper, and its angle in [0, τ) from at_0 toward at_90.
typedef struct {
	float height;
	float angle;
} spot;

// The faceted surface at a spot: its distance from the axis, and the angle the
// facets' interpolated u reads there.
typedef struct {
	float distance;
	float angle;
} facet;

// The trunk's surface under one base vertex.
typedef struct {
	voe_math_float3 position;
	voe_math_float3 normal;
	voe_math_float2 uv;
} sample;

// The radius of `s`'s surface across from `p`: tree_mesh.c's rule, copied.
static float radius_at(voe_3d_tree_segment s, voe_math_float3 p)
{
	const voe_math_float3 d = voe_math_float3_sub(s.end, s.start);
	const float t = voe_math_float3_dot(voe_math_float3_sub(p, s.start), d) /
			voe_math_float3_dot(d, d);
	const float c = fminf(fmaxf(t, 0.0f), 1.0f);

	return s.radius_start + (s.radius_end - s.radius_start) * c;
}

// How far `p` lies outside `s`'s round surface, `t` its unit direction.
static float outside_of(voe_3d_tree_segment s, voe_math_float3 t,
			voe_math_float3 p)
{
	const voe_math_float3 q = voe_math_float3_sub(p, s.start);
	const voe_math_float3 off = voe_math_float3_sub(
		q, voe_math_float3_scale(t, voe_math_float3_dot(q, t)));

	return voe_math_float3_length(off) - radius_at(s, p);
}

// Where along `branch` from its start, `d` its unit direction and `length`
// long, it leaves `trunk`; negative when its end is still inside.
static float leaving(voe_3d_tree_segment trunk, voe_math_float3 t,
		     voe_3d_tree_segment branch, voe_math_float3 d, float length)
{
	float in = 0.0f, out = length;

	VOE_BASE_ASSERT(outside_of(trunk, t, branch.start) <= 0.0f,
			"a branch starting inside its trunk");
	if (outside_of(trunk, t, branch.end) <= 0.0f)
		return -1.0f;
	for (uint32_t i = 0; i < LEAVE_STEPS; i++) {
		const float mid = 0.5f * (in + out);
		const voe_math_float3 p = voe_math_float3_add(
			branch.start, voe_math_float3_scale(d, mid));

		if (outside_of(trunk, t, p) <= 0.0f)
			in = mid;
		else
			out = mid;
	}
	VOE_BASE_ASSERT(out > 0.0f && out <= length, "a leaving on the segment");
	return out;
}

bool voe_3d_tree_joint_of(voe_3d_tree_segment trunk, voe_3d_tree_segment branch,
			  voe_3d_tree_joint *out)
{
	const voe_math_float3 t = voe_math_float3_normalize(
		voe_math_float3_sub(trunk.end, trunk.start));
	const voe_math_float3 first = voe_math_float3_sub(branch.end, branch.start);
	const float length = voe_math_float3_length(first);
	const voe_math_float3 d = voe_math_float3_scale(first, 1.0f / length);
	const float sine = voe_math_float3_length(voe_math_float3_cross(t, d));
	const float r = branch.radius_start;
	voe_math_float3 j0, foot, radial, back;
	float at, axial, collar;

	VOE_BASE_ASSERT(out != NULL && length > 0.0f && r > 0.0f,
			"a branch segment, its radius and an out");
	at = leaving(trunk, t, branch, d, length);
	if (at < 0.0f)
		return false;
	j0 = voe_math_float3_add(branch.start, voe_math_float3_scale(d, at));
	axial = voe_math_float3_dot(voe_math_float3_sub(j0, trunk.start), t);
	foot = voe_math_float3_add(trunk.start, voe_math_float3_scale(t, axial));
	radial = voe_math_float3_normalize(voe_math_float3_sub(j0, foot));
	collar = fminf(fmaxf(1.25f * r / sine, 1.5f * r), 0.5f * (length - at));
	back = voe_math_float3_sub(foot, voe_math_float3_scale(t, collar));
	*out = (voe_3d_tree_joint){
		.centre = voe_math_float3_add(
			back,
			voe_math_float3_scale(radial, radius_at(trunk, back))),
		.end = voe_math_float3_add(j0, voe_math_float3_scale(d, collar)),
		.along = t,
		.around = voe_math_float3_cross(t, radial),
		.half_along = fminf(fmaxf(1.5f * r / sine, 2.25f * r), 4.0f * r),
		.half_around = 1.5f * r,
	};
	VOE_BASE_ASSERT(out->half_along > out->half_around, "a long ellipse");
	return true;
}

static voe_math_float3 axis_of(const voe_3d_tree_stretch *s)
{
	return voe_math_float3_sub(s->upper.centre, s->lower.centre);
}

// `p` read on `s`: across the ring planes, which at_0 and at_90 span.
static spot spot_of(const voe_3d_tree_stretch *s, voe_math_float3 p)
{
	const voe_math_float3 axis = axis_of(s);
	const voe_math_float3 across = voe_math_float3_cross(s->at_0, s->at_90);
	const voe_math_float3 q = voe_math_float3_sub(p, s->lower.centre);
	const float rise = voe_math_float3_dot(axis, across);
	float height, angle;
	voe_math_float3 flat;

	VOE_BASE_ASSERT(rise > 1e-6f, "an upper ring above the lower");
	height = voe_math_float3_dot(q, across) / rise;
	flat = voe_math_float3_sub(q, voe_math_float3_scale(axis, height));
	angle = atan2f(voe_math_float3_dot(flat, s->at_90),
		       voe_math_float3_dot(flat, s->at_0));
	return (spot){ height, angle < 0.0f ? angle + TAU : angle };
}

// The facet `at` falls on: the polygon edge between vertices `side` and
// `side` + 1, its midpoint at R cos(half) from the axis, `off` from its middle.
static facet facet_at(const voe_3d_tree_stretch *s, spot at)
{
	const float half = 0.5f * TAU / (float)s->sides;
	const float side = floorf(at.angle / (2.0f * half));
	const float off = at.angle - (side + 0.5f) * 2.0f * half;
	const float radius = s->lower.radius +
			     (s->upper.radius - s->lower.radius) * at.height;

	VOE_BASE_ASSERT(fabsf(off) <= half * 1.0001f, "an angle on its facet");
	return (facet){
		.distance = radius * cosf(half) / cosf(off),
		.angle = 2.0f * half *
			 (side + 0.5f + 0.5f * tanf(off) / tanf(half)),
	};
}

// The trunk under base vertex `o`: the ellipse's point in o's direction seen
// along J's normal, snapped radially onto the facet, u read from J's angle.
static sample trunk_under(const voe_3d_tree_joint *j,
			  const voe_3d_tree_stretch *s, voe_math_float3 o,
			  float centre_angle)
{
	const float a = j->half_along, b = j->half_around;
	const voe_math_float2 seen = voe_math_float2_normalize((voe_math_float2){
		voe_math_float3_dot(o, j->along),
		voe_math_float3_dot(o, j->around) });
	const float reach = a * b / sqrtf((b * seen.x) * (b * seen.x) +
					  (a * seen.y) * (a * seen.y));
	const voe_math_float3 on_plane = voe_math_float3_add(
		j->centre,
		voe_math_float3_scale(
			voe_math_float3_add(
				voe_math_float3_scale(j->along, seen.x),
				voe_math_float3_scale(j->around, seen.y)),
			reach));
	const spot at = spot_of(s, on_plane);
	const facet f = facet_at(s, at);
	const voe_math_float3 radial = voe_math_float3_add(
		voe_math_float3_scale(s->at_0, cosf(at.angle)),
		voe_math_float3_scale(s->at_90, sinf(at.angle)));
	const voe_math_float3 centre = voe_math_float3_add(
		s->lower.centre, voe_math_float3_scale(axis_of(s), at.height));

	return (sample){
		.position = voe_math_float3_add(
			centre, voe_math_float3_scale(radial, f.distance)),
		.normal = radial,
		.uv = { s->turns *
				(centre_angle +
				 remainderf(f.angle - centre_angle, TAU)) /
				TAU,
			s->lower.v + (s->upper.v - s->lower.v) * at.height },
	};
}

void voe_3d_tree_joint_ring(const voe_3d_tree_joint *joint,
			    const voe_3d_tree_stretch *stretch,
			    const voe_math_float3 *offsets, uint32_t count,
			    float radius, float own_turns, float own_v,
			    float share, voe_math_float3 *positions,
			    voe_math_float3 *normals, voe_math_float2 *uvs)
{
	const float w = 1.0f - share * share * (3.0f - 2.0f * share);
	float centre_angle;

	VOE_BASE_ASSERT(joint != NULL && stretch != NULL && offsets != NULL &&
				positions != NULL && normals != NULL &&
				uvs != NULL,
			"a joint, a stretch, offsets and three outs");
	VOE_BASE_ASSERT(count >= 4 && stretch->sides >= 3 &&
				stretch->turns >= 1.0f && radius > 0.0f &&
				share >= 0.0f && share <= 1.0f,
			"a ring of 3 sides or more at a share of 0 to 1");
	centre_angle = facet_at(stretch, spot_of(stretch, joint->centre)).angle;
	for (uint32_t k = 0; k < count; k++) {
		const voe_math_float3 o = offsets[k];
		const sample base = trunk_under(joint, stretch, o, centre_angle);
		const voe_math_float3 blended = voe_math_float3_add(
			voe_math_float3_scale(base.normal, w),
			voe_math_float3_scale(o, 1.0f - w));
		const voe_math_float2 own = {
			own_turns * (float)k / (float)(count - 1), own_v
		};

		VOE_BASE_ASSERT(voe_math_float3_length(blended) > 1e-4f,
				"a normal between trunk and branch");
		positions[k] = voe_math_float3_lerp(
			base.position,
			voe_math_float3_add(joint->end,
					    voe_math_float3_scale(o, radius)),
			share);
		normals[k] = voe_math_float3_normalize(blended);
		uvs[k] = voe_math_float2_lerp(base.uv, own, 1.0f - w);
	}
}
