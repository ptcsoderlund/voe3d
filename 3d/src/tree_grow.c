// A tree's skeleton grown from its values (3d/tree_grow.h): the trunk as one
// limb, then each limb in the order it was grown given its children, so the
// walk is breadth first and the caps cut twigs before branches.
//
// A limb is a fixed number of equal steps by order (8 trunk, 4 branch, 2
// twig), whole or not at all. Children leave a limb above `crown` × its
// length, evenly spaced and jittered, turned about it by the golden angle and
// a jitter. A child's base radius is 0.8 × √reach of its parent's there, and
// every limb tapers to a quarter of its base at its tip.
//
// The pick folds kind, x and z into one 64-bit word by odd multipliers and
// finishes it with murmur3's fmix64, so neighbouring copies differ.
//
// Constraints: one push of VOE_3D_TREE_SEGMENTS segments; a walk over them
// once, each limb's children up to the cap.
#include <3d/tree_grow.h>

#include <base/assert.h>
#include <math/float3.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define ORDERS 3
#define GOLDEN_ANGLE 2.39996323f
#define DEGREES 0.0174532925f
// The tip's radius as a share of the base's is 1 − TAPER.
#define TAPER 0.75f
// How far `droop` 1 turns a limb's heading toward −Y over its length, and how
// far `crooked` 1 jitters one step's.
#define DROOP_BEND 2.0f
#define CROOKED_JITTER 0.3f

static const uint32_t limb_steps[ORDERS] = { 8, 4, 2 };

typedef struct {
	const voe_assets_tree *values;
	voe_3d_tree_segment *segments;
	uint32_t count;
	uint64_t seed;
	bool full;
} growth;

// The next number in [0, 1) from a xorshift64 on `*seed`.
static float draw(uint64_t *seed)
{
	uint64_t x = *seed;

	VOE_BASE_ASSERT(x != 0, "a nonzero seed");
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	*seed = x;
	return (float)(x >> 40) * 0x1p-24f;
}

static float spread(uint64_t *seed)
{
	return draw(seed) * 2.0f - 1.0f;
}

// A nonzero seed from the variation alone, its bits mixed by splitmix64.
static uint64_t seed_of(uint32_t variation)
{
	uint64_t z = (uint64_t)variation + 0x9E3779B97F4A7C15ull;

	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	z ^= z >> 31;
	return z != 0 ? z : 1;
}

// Two unit vectors perpendicular to unit `d` and to each other.
static void around(voe_math_float3 d, voe_math_float3 *u, voe_math_float3 *w)
{
	const voe_math_float3 ref = fabsf(d.y) < 0.9f ?
					    (voe_math_float3){ 0.0f, 1.0f, 0.0f } :
					    (voe_math_float3){ 1.0f, 0.0f, 0.0f };

	*u = voe_math_float3_normalize(voe_math_float3_cross(d, ref));
	*w = voe_math_float3_cross(d, *u);
}

// Appends a limb of `order` from `start` along `heading`, `length` long and
// `radius` at its base, its first segment's parent `parent`. False, and
// nothing appended, when it would pass the cap.
static bool grow_limb(growth *g, voe_math_float3 start, voe_math_float3 heading,
		      float length, float radius, uint32_t parent,
		      uint32_t order)
{
	const uint32_t steps = limb_steps[order];
	const float step = length / (float)steps;
	const float bend = order > 0 ? g->values->droop * DROOP_BEND / steps : 0;
	const float jitter = g->values->crooked * CROOKED_JITTER;
	voe_math_float3 at = start, d = heading;

	if (g->count + steps > VOE_3D_TREE_SEGMENTS)
		return false;
	for (uint32_t k = 0; k < steps; k++) {
		const voe_math_float3 wander = { spread(&g->seed),
						 spread(&g->seed),
						 spread(&g->seed) };
		const voe_math_float3 end = voe_math_float3_add(
			at, voe_math_float3_scale(d, step));

		g->segments[g->count] = (voe_3d_tree_segment){
			.start = at,
			.end = end,
			.radius_start = radius * (1.0f - TAPER * k / steps),
			.radius_end = radius * (1.0f - TAPER * (k + 1) / steps),
			.parent = k == 0 ? parent : g->count - 1,
			.order = order,
		};
		g->count++;
		at = end;
		d.y -= bend;
		d = voe_math_float3_normalize(voe_math_float3_add(
			d, voe_math_float3_scale(wander, jitter)));
	}
	return true;
}

// The children of the limb whose first segment is `first`.
static void grow_children(growth *g, uint32_t first)
{
	const voe_assets_tree *v = g->values;
	const voe_3d_tree_segment base = g->segments[first];
	const uint32_t steps = limb_steps[base.order];
	const float length =
		steps * voe_math_float3_length(
				voe_math_float3_sub(base.end, base.start));
	const uint32_t count = (uint32_t)(v->branching * length *
					  (1.0f - v->crown) + 0.5f);
	const float angle = v->angle * DEGREES;

	for (uint32_t i = 0; i < count && !g->full; i++) {
		const float t = v->crown + (1.0f - v->crown) *
						   (i + 0.5f + 0.4f * spread(&g->seed)) /
						   count;
		const uint32_t j =
			(uint32_t)fminf(t * (float)steps, (float)(steps - 1));
		const float frac = t * (float)steps - (float)j;
		const voe_3d_tree_segment on = g->segments[first + j];
		const voe_math_float3 d = voe_math_float3_normalize(
			voe_math_float3_sub(on.end, on.start));
		const float turn = i * GOLDEN_ANGLE + 0.5f * spread(&g->seed);
		voe_math_float3 u, w, heading;

		around(d, &u, &w);
		heading = voe_math_float3_add(
			voe_math_float3_scale(d, cosf(angle)),
			voe_math_float3_scale(
				voe_math_float3_add(
					voe_math_float3_scale(u, cosf(turn)),
					voe_math_float3_scale(w, sinf(turn))),
				sinf(angle)));
		g->full = !grow_limb(
			g, voe_math_float3_lerp(on.start, on.end, frac), heading,
			v->reach * length * (1.0f - t),
			0.8f * sqrtf(v->reach) *
				(on.radius_start +
				 (on.radius_end - on.radius_start) * frac),
			first + j, base.order + 1);
	}
}

void voe_3d_tree_grow(voe_base_arena *arena, const voe_assets_tree *values,
		      uint32_t variation, voe_3d_tree_skeleton *out)
{
	growth g = {
		.values = values,
		.segments = voe_base_arena_push(
			arena, sizeof(voe_3d_tree_segment) *
				       VOE_3D_TREE_SEGMENTS),
		.seed = seed_of(variation),
	};

	VOE_BASE_ASSERT(values != NULL && out != NULL, "values and an out");
	grow_limb(&g, (voe_math_float3){ 0 }, (voe_math_float3){ 0, 1, 0 },
		  values->height, 0.5f * values->trunk, VOE_3D_TREE_NO_PARENT, 0);
	for (uint32_t i = 0; i < g.count && !g.full; i++) {
		const voe_3d_tree_segment s = g.segments[i];

		if (s.order + 1 < ORDERS &&
		    (s.parent == VOE_3D_TREE_NO_PARENT ||
		     g.segments[s.parent].order != s.order))
			grow_children(&g, i);
	}
	out->segments = g.segments;
	out->count = g.count;
}

uint32_t voe_3d_tree_pick(uint32_t kind, int32_t x_mm, int32_t z_mm,
			  uint32_t count)
{
	uint64_t h = (uint64_t)kind * 0x9E3779B97F4A7C15ull ^
		     (uint64_t)(uint32_t)x_mm * 0xC2B2AE3D27D4EB4Full ^
		     (uint64_t)(uint32_t)z_mm * 0x165667B19E3779F9ull;

	VOE_BASE_ASSERT(count > 0, "a pick among no variations");
	h = (h ^ (h >> 33)) * 0xFF51AFD7ED558CCDull;
	h = (h ^ (h >> 33)) * 0xC4CEB9FE1A85EC53ull;
	h ^= h >> 33;
	return (uint32_t)(h % count);
}
