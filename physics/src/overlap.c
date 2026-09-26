// The overlap query: the query shape as a segment plus a radius, in float about
// its own centre, tested against each collider's shape in turn.
//
// Against a sphere, the closest point on the segment; against a capsule, the
// closest points of two segments; against a box, the segment in the box's
// frame and the point along it of least signed distance to the box. That
// distance is convex along a segment, so a bounded ternary search finds it:
// SEARCH_STEPS narrows the segment by (2/3)^40, under 1e-7 of its length.
#include <base/assert.h>
#include <math/double3.h>
#include <math/quat.h>
#include <physics/collider_component.h>
#include <physics/overlap.h>

#include <math.h>

#define SEARCH_STEPS 40

// A segment a..b swept by a radius, about the query's centre.
struct swept {
	voe_math_float3 a;
	voe_math_float3 b;
	float radius;
};

// How far the query's nearest point is from the other shape, signed (below 0
// inside a box), and the unit way from the other shape towards the query.
struct nearest {
	float gap;
	voe_math_float3 normal;
};

static voe_math_float3 rotate(voe_math_quat q, voe_math_float3 v)
{
	const voe_math_float3 u = { q.x, q.y, q.z };
	const voe_math_float3 t =
		voe_math_float3_scale(voe_math_float3_cross(u, v), 2.0f);

	return voe_math_float3_add(
		voe_math_float3_add(v, voe_math_float3_scale(t, q.w)),
		voe_math_float3_cross(u, t));
}

// A sphere or capsule as a segment and radius, its centre at `offset`.
static struct swept swept_of(voe_physics_shape shape, voe_math_float3 offset)
{
	float along = 0.0f;
	voe_math_float3 axis;

	VOE_BASE_ASSERT(shape.kind != VOE_PHYSICS_COLLIDER_BOX,
			"a box is no swept shape");
	if (shape.kind == VOE_PHYSICS_COLLIDER_CAPSULE)
		along = shape.half.y - shape.half.x;
	axis = voe_math_float3_scale(
		rotate(shape.rotation, (voe_math_float3){ 0.0f, 1.0f, 0.0f }),
		along);
	return (struct swept){ .a = voe_math_float3_sub(offset, axis),
			       .b = voe_math_float3_add(offset, axis),
			       .radius = shape.half.x };
}

static float clamp01(float t)
{
	return fminf(fmaxf(t, 0.0f), 1.0f);
}

// The gap between two segments, and the way from q's closest point to p's
// (Ericson, Real-Time Collision Detection 5.1.9). Coincident points push up.
static struct nearest segments(struct swept p, struct swept q)
{
	const voe_math_float3 d1 = voe_math_float3_sub(p.b, p.a);
	const voe_math_float3 d2 = voe_math_float3_sub(q.b, q.a);
	const voe_math_float3 r = voe_math_float3_sub(p.a, q.a);
	const float a = voe_math_float3_dot(d1, d1);
	const float e = voe_math_float3_dot(d2, d2);
	const float f = voe_math_float3_dot(d2, r);
	const float c = voe_math_float3_dot(d1, r);
	const float b = voe_math_float3_dot(d1, d2);
	const float denom = a * e - b * b;
	float s = 0.0f;
	float t = 0.0f;
	voe_math_float3 between;
	float length;

	if (a > 1e-12f)
		s = denom > 1e-12f ? clamp01((b * f - c * e) / denom) : 0.0f;
	if (e > 1e-12f) {
		t = (b * s + f) / e;
		if (t < 0.0f || t > 1.0f) {
			t = clamp01(t);
			s = a > 1e-12f ? clamp01((t * b - c) / a) : 0.0f;
		}
	}
	between = voe_math_float3_sub(
		voe_math_float3_add(p.a, voe_math_float3_scale(d1, s)),
		voe_math_float3_add(q.a, voe_math_float3_scale(d2, t)));
	length = voe_math_float3_length(between);
	if (length < 1e-12f)
		return (struct nearest){ 0.0f, { 0.0f, 1.0f, 0.0f } };
	return (struct nearest){ length,
				 voe_math_float3_scale(between, 1.0f / length) };
}

// The signed distance from `p` to a box of half extents `half` about the
// origin, and the way out: to the nearest face when inside.
static struct nearest box_distance(voe_math_float3 p, voe_math_float3 half)
{
	const voe_math_float3 q = { fabsf(p.x) - half.x, fabsf(p.y) - half.y,
				    fabsf(p.z) - half.z };
	const voe_math_float3 out = voe_math_float3_sub(
		p, voe_math_float3_clamp(p, voe_math_float3_neg(half), half));
	const float length = voe_math_float3_length(out);
	voe_math_float3 normal = { 0.0f, 0.0f, 0.0f };
	float inside = q.x;

	if (length > 0.0f)
		return (struct nearest){ length,
					 voe_math_float3_scale(out, 1.0f / length) };
	normal.x = copysignf(1.0f, p.x);
	if (q.y > inside) {
		inside = q.y;
		normal = (voe_math_float3){ 0.0f, copysignf(1.0f, p.y), 0.0f };
	}
	if (q.z > inside) {
		inside = q.z;
		normal = (voe_math_float3){ 0.0f, 0.0f, copysignf(1.0f, p.z) };
	}
	return (struct nearest){ inside, normal };
}

static float box_gap_at(struct swept local, voe_math_float3 half, float t)
{
	return box_distance(voe_math_float3_lerp(local.a, local.b, t), half).gap;
}

// The segment in the box's frame, the point along it nearest the box, and the
// way out turned back into the query's frame.
static struct nearest segment_box(struct swept query, voe_physics_shape box,
				  voe_math_float3 offset)
{
	const voe_math_quat back = { -box.rotation.x, -box.rotation.y,
				     -box.rotation.z, box.rotation.w };
	const struct swept local = {
		.a = rotate(back, voe_math_float3_sub(query.a, offset)),
		.b = rotate(back, voe_math_float3_sub(query.b, offset)),
	};
	float lo = 0.0f;
	float hi = 1.0f;
	struct nearest found;

	for (int step = 0; step < SEARCH_STEPS; step++) {
		const float one = lo + (hi - lo) / 3.0f;
		const float two = hi - (hi - lo) / 3.0f;

		if (box_gap_at(local, box.half, one) <
		    box_gap_at(local, box.half, two))
			hi = two;
		else
			lo = one;
	}
	found = box_distance(voe_math_float3_lerp(local.a, local.b,
						  0.5f * (lo + hi)),
			     box.half);
	found.normal = rotate(box.rotation, found.normal);
	return found;
}

// The query against one other shape: true and *out filled when they overlap.
static bool contact_with(struct swept query, voe_physics_shape query_shape,
			 voe_physics_shape other, voe_physics_contact *out)
{
	const voe_math_float3 offset = voe_math_double3_to_float3(
		voe_math_double3_sub(other.centre, query_shape.centre));
	struct nearest found;
	float depth;

	if (other.kind == VOE_PHYSICS_COLLIDER_BOX) {
		found = segment_box(query, other, offset);
		depth = query.radius - found.gap;
	} else {
		const struct swept them = swept_of(other, offset);

		found = segments(query, them);
		depth = query.radius + them.radius - found.gap;
	}
	if (!(depth > 0.0f))
		return false;
	out->normal = found.normal;
	out->depth = depth;
	return true;
}

uint32_t voe_physics_overlap(const voe_ecs_world *world,
			     voe_physics_shape query, voe_ecs_entity ignore,
			     voe_physics_contact *out, uint32_t capacity)
{
	uint32_t count;
	const voe_physics_collider *rows;
	const voe_ecs_entity *entities;
	struct swept swept;
	uint32_t written = 0;

	VOE_BASE_ASSERT(world != NULL && (out != NULL || capacity == 0),
			"an overlap in no world, or into nowhere");
	count = voe_physics_collider_count(world);
	rows = voe_physics_collider_rows(world);
	entities = voe_physics_collider_entities(world);
	swept = swept_of(query, (voe_math_float3){ 0.0f, 0.0f, 0.0f });

	for (uint32_t i = 0; i < count && written < capacity; i++) {
		voe_physics_shape other;
		voe_physics_contact contact = { .entity = entities[i],
						.trigger = rows[i].trigger };

		if (entities[i].index == ignore.index &&
		    entities[i].generation == ignore.generation)
			continue;
		if (!voe_physics_shape_of(world, entities[i], &other))
			continue;
		if (contact_with(swept, query, other, &contact))
			out[written++] = contact;
	}
	VOE_BASE_DEBUG_ASSERT(written <= capacity, "wrote past capacity");
	return written;
}
