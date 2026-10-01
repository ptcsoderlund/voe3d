// The bounce schedule's one call: the entered cells, then the stale spheres,
// then the cycle, into the caller's arrays (see bounce_schedule.h for the
// order and what each list blends).
//
// A call marks what it has listed in a 4 KiB bit set on the stack, which is how
// no index is listed twice; nothing is allocated and nothing outlives the call
// but the schedule itself. The cycle walks every index once per 32³ steps
// because its stride is odd, and an index already listed this call is stepped
// over without counting against the budget.
//
// CONSTRAINTS. The entered scan visits all 32³ cells every call — a few tens of
// microseconds; per-axis slabs would lift it.
#include "bounce_schedule.h"

#include <assert.h>
#include <string.h>

#define SIDE ((uint32_t)VOE_RENDER_BOUNCE_PROBES)
#define ALL ((uint32_t)VOE_RENDER_BOUNCE_PROBES_ALL)
// Near 32³ / φ and odd, so coprime with 32³: consecutive steps land far apart.
#define CYCLE_STRIDE 20251u

static_assert((SIDE & (SIDE - 1)) == 0, "the toroidal mask needs a power of 2");
static_assert((CYCLE_STRIDE & 1u) == 1u, "the stride must be coprime with 32³");

// One call's output and what it has listed so far.
struct listing {
	uint32_t *probes;
	float *blends;
	uint32_t room;
	uint32_t count;
	uint32_t marks[VOE_RENDER_BOUNCE_PROBES_ALL / 32];
};

static uint32_t toroidal(const int32_t cell[3], uint32_t x, uint32_t y,
			 uint32_t z)
{
	const uint32_t probe = voe_render_bounce_wrap((int64_t)cell[0] + x) +
			       SIDE * voe_render_bounce_wrap((int64_t)cell[1] + y) +
			       SIDE * SIDE * voe_render_bounce_wrap((int64_t)cell[2] + z);

	assert(probe < ALL);
	return probe;
}

// Lists `probe` unless it is listed already or there is no room; true if it was.
static bool list(struct listing *out, uint32_t probe, float blend)
{
	assert(probe < ALL);
	const uint32_t bit = 1u << (probe & 31u);

	if (out->count >= out->room || (out->marks[probe / 32] & bit) != 0)
		return false;
	out->marks[probe / 32] |= bit;
	out->probes[out->count] = probe;
	out->blends[out->count] = blend;
	out->count++;
	assert(out->count <= out->room);
	return true;
}

// Whether world cell `w` lies outside the grid whose lowest cell is `last`.
static bool outside(int64_t w, int32_t last)
{
	return w < last || w >= (int64_t)last + SIDE;
}

static void list_entered(struct listing *out, const voe_render_bounce_schedule *s,
			 const int32_t cell[3])
{
	bool whole = !s->has_cell;

	for (int a = 0; a < 3 && !whole; a++) {
		const int64_t d = (int64_t)cell[a] - s->cell[a];

		whole = d >= SIDE || d <= -(int64_t)SIDE;
	}
	for (uint32_t z = 0; z < SIDE; z++)
		for (uint32_t y = 0; y < SIDE; y++)
			for (uint32_t x = 0; x < SIDE; x++)
				if (whole ||
				    outside((int64_t)cell[0] + x, s->cell[0]) ||
				    outside((int64_t)cell[1] + y, s->cell[1]) ||
				    outside((int64_t)cell[2] + z, s->cell[2]))
					list(out, toroidal(cell, x, y, z), 1.0f);
	assert(out->count <= out->room);
}

// The cells along one axis whose centre may lie within [lo, hi] metres of the
// corner: a superset, the distance test decides. False when none can.
static bool axis_span(float lo, float hi, uint32_t *first, uint32_t *last)
{
	const float a = lo / VOE_RENDER_BOUNCE_SPACING - 0.5f;
	const float b = hi / VOE_RENDER_BOUNCE_SPACING - 0.5f;

	if (!(a <= (float)(SIDE - 1)) || !(b >= 0.0f))
		return false;
	*first = a <= 0.0f ? 0 : (uint32_t)a;
	*last = b >= (float)(SIDE - 1) ? SIDE - 1 : (uint32_t)b + 1;
	assert(*first <= *last && *last < SIDE);
	return true;
}

static float centre_of(float corner, uint32_t i)
{
	return corner + ((float)i + 0.5f) * VOE_RENDER_BOUNCE_SPACING;
}

static void list_sphere(struct listing *out, const int32_t cell[3],
			voe_math_float3 corner, voe_math_float4 sphere,
			uint32_t *budget)
{
	const float r = sphere.w;
	uint32_t x0, x1, y0, y1, z0, z1;

	if (!(r >= 0.0f) ||
	    !axis_span(sphere.x - corner.x - r, sphere.x - corner.x + r, &x0, &x1) ||
	    !axis_span(sphere.y - corner.y - r, sphere.y - corner.y + r, &y0, &y1) ||
	    !axis_span(sphere.z - corner.z - r, sphere.z - corner.z + r, &z0, &z1))
		return;
	for (uint32_t z = z0; z <= z1 && *budget > 0; z++)
		for (uint32_t y = y0; y <= y1 && *budget > 0; y++)
			for (uint32_t x = x0; x <= x1 && *budget > 0; x++) {
				const float dx = centre_of(corner.x, x) - sphere.x;
				const float dy = centre_of(corner.y, y) - sphere.y;
				const float dz = centre_of(corner.z, z) - sphere.z;

				if (dx * dx + dy * dy + dz * dz <= r * r &&
				    list(out, toroidal(cell, x, y, z),
					 VOE_RENDER_BOUNCE_BLEND))
					(*budget)--;
			}
}

static void list_cycle(struct listing *out, voe_render_bounce_schedule *s,
		       uint32_t budget)
{
	for (uint32_t step = 0;
	     step < ALL && budget > 0 && out->count < out->room; step++) {
		const uint32_t probe = (s->cycle * CYCLE_STRIDE) & (ALL - 1);

		s->cycle = (s->cycle + 1) & (ALL - 1);
		if (list(out, probe, VOE_RENDER_BOUNCE_BLEND))
			budget--;
	}
	assert(s->cycle < ALL);
}

static bool light_changed(const voe_render_bounce_schedule *s,
			  const voe_render_light *sun)
{
	return s->direction.x != sun->direction.x ||
	       s->direction.y != sun->direction.y ||
	       s->direction.z != sun->direction.z ||
	       s->intensity != sun->intensity ||
	       s->colour.x != sun->colour.x || s->colour.y != sun->colour.y ||
	       s->colour.z != sun->colour.z;
}

uint32_t voe_render_bounce_schedule_next(voe_render_bounce_schedule *s,
					 const int32_t cell[3],
					 voe_math_float3 corner,
					 const voe_render_light *sun,
					 const voe_math_float4 *stale,
					 uint32_t stale_count, uint32_t *probes,
					 float *blends, uint32_t room)
{
	assert(s != NULL && cell != NULL && sun != NULL);
	assert(stale != NULL || stale_count == 0);
	assert((probes != NULL && blends != NULL) || room == 0);
	struct listing out = { .probes = probes, .blends = blends, .room = room };
	uint32_t budget = VOE_RENDER_BOUNCE_BUDGET;

	memset(out.marks, 0, sizeof(out.marks));
	list_entered(&out, s, cell);
	for (uint32_t i = 0; i < stale_count && budget > 0; i++)
		list_sphere(&out, cell, corner, stale[i], &budget);
	if (light_changed(s, sun))
		s->cycle = 0;
	list_cycle(&out, s, budget);

	memcpy(s->cell, cell, sizeof(s->cell));
	s->has_cell = true;
	s->direction = sun->direction;
	s->intensity = sun->intensity;
	s->colour = sun->colour;
	assert(out.count <= room);
	return out.count;
}
