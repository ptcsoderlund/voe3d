// The captured bounce grid's bookkeeping: place, take, relight-needed and
// relit (see bounce_probes.h for what each one means).
//
// Every scan walks the grid in local cells, 0..size − 1 from the lowest cell,
// and turns each into its toroidal index through voe_render_bounce_probe_index
// on the world cell. The three marks are bit sets in the state; nothing is
// allocated and nothing outlives a call but the state itself.
//
// CONSTRAINTS. Whole-grid scans, as the header names: one per place, one per
// stale sphere, one per probe taken.
#include "bounce_probes.h"

#include <assert.h>
#include <math.h>
#include <string.h>

#define XZ ((uint32_t)VOE_RENDER_BOUNCE_PROBES_XZ)
#define H ((uint32_t)VOE_RENDER_BOUNCE_PROBES_Y)
#define TOTAL ((uint32_t)VOE_RENDER_BOUNCE_PROBES_TOTAL)

static_assert(TOTAL % 32 == 0, "the marks are whole words");

static uint32_t index_of(const int32_t cell[3], uint32_t x, uint32_t y,
			 uint32_t z)
{
	const uint32_t probe = voe_render_bounce_probe_index(
		(int64_t)cell[0] + x, (int64_t)cell[1] + y, (int64_t)cell[2] + z);

	assert(probe < TOTAL);
	return probe;
}

static void set_bit(uint32_t *bits, uint32_t probe)
{
	assert(probe < TOTAL);
	bits[probe / 32] |= 1u << (probe & 31u);
}

static void clear_bit(uint32_t *bits, uint32_t probe)
{
	assert(probe < TOTAL);
	bits[probe / 32] &= ~(1u << (probe & 31u));
}

static bool bit(const uint32_t *bits, uint32_t probe)
{
	assert(probe < TOTAL);
	return (bits[probe / 32] & (1u << (probe & 31u))) != 0;
}

// Whether world cell `w` lies outside the span of `size` from `last`.
static bool outside(int64_t w, int32_t last, uint32_t size)
{
	return w < last || w >= (int64_t)last + size;
}

static voe_math_float3 centre_of(voe_math_float3 corner, float s, uint32_t x,
				 uint32_t y, uint32_t z)
{
	assert(x < XZ && y < H && z < XZ);
	assert(s > 0.0f);
	return (voe_math_float3){ corner.x + ((float)x + 0.5f) * s,
				  corner.y + ((float)y + 0.5f) * s,
				  corner.z + ((float)z + 0.5f) * s };
}

static void enter(voe_render_bounce_probes *p, const int32_t cell[3],
		  float spacing)
{
	bool whole = !p->placed || spacing != p->spacing;
	const uint32_t size[3] = { XZ, H, XZ };

	for (int a = 0; a < 3 && !whole; a++) {
		const int64_t d = (int64_t)cell[a] - p->cell[a];

		whole = d >= size[a] || d <= -(int64_t)size[a];
	}
	for (uint32_t z = 0; z < XZ; z++)
		for (uint32_t y = 0; y < H; y++)
			for (uint32_t x = 0; x < XZ; x++) {
				if (!whole &&
				    !outside((int64_t)cell[0] + x, p->cell[0], XZ) &&
				    !outside((int64_t)cell[1] + y, p->cell[1], H) &&
				    !outside((int64_t)cell[2] + z, p->cell[2], XZ))
					continue;
				const uint32_t probe = index_of(cell, x, y, z);

				clear_bit(p->holds, probe);
				set_bit(p->queued, probe);
				set_bit(p->changed, probe);
			}
	assert(whole || p->placed);
}

static void queue_sphere(voe_render_bounce_probes *p, const int32_t cell[3],
			 voe_math_float3 corner, float spacing,
			 voe_math_float4 sphere)
{
	assert(p != NULL && cell != NULL);
	if (!(sphere.w >= 0.0f) || !isfinite(sphere.w))
		return;
	for (uint32_t z = 0; z < XZ; z++)
		for (uint32_t y = 0; y < H; y++)
			for (uint32_t x = 0; x < XZ; x++) {
				const voe_math_float3 c =
					centre_of(corner, spacing, x, y, z);
				const float dx = c.x - sphere.x;
				const float dy = c.y - sphere.y;
				const float dz = c.z - sphere.z;

				if (dx * dx + dy * dy + dz * dz <= sphere.w * sphere.w)
					set_bit(p->queued, index_of(cell, x, y, z));
			}
}

static void pick_lights(const voe_render_light *sun, uint32_t sun_bounces,
			float sun_strength,
			const voe_render_point_lights *point_lights,
			voe_render_bounce_lights *lights)
{
	memset(lights, 0, sizeof(*lights));
	lights->sun = *sun;
	lights->sun_bounces = sun_bounces;
	lights->sun_strength = sun_strength;
	for (uint32_t i = 0; i < point_lights->count &&
			     lights->lamp_count < VOE_RENDER_BOUNCE_LAMPS;
	     i++)
		if (point_lights->lights[i].bounces >= 1)
			lights->lamps[lights->lamp_count++] =
				point_lights->lights[i];
	assert(lights->lamp_count <= VOE_RENDER_BOUNCE_LAMPS);
}

void voe_render_bounce_probes_place(voe_render_bounce_probes *p,
				    const int32_t cell[3], voe_math_float3 corner,
				    float spacing, const voe_math_float4 *stale,
				    uint32_t stale_count,
				    const voe_render_light *sun,
				    uint32_t sun_bounces, float sun_strength,
				    const voe_render_point_lights *point_lights,
				    voe_render_bounce_lights *lights)
{
	assert(p != NULL && cell != NULL && sun != NULL && lights != NULL);
	assert(spacing > 0.0f && isfinite(spacing));
	assert(stale != NULL || stale_count == 0);
	assert(point_lights != NULL &&
	       (point_lights->lights != NULL || point_lights->count == 0));

	enter(p, cell, spacing);
	for (uint32_t i = 0; i < stale_count; i++)
		queue_sphere(p, cell, corner, spacing, stale[i]);
	memcpy(p->cell, cell, sizeof(p->cell));
	p->corner = corner;
	p->spacing = spacing;
	p->placed = true;
	pick_lights(sun, sun_bounces, sun_strength, point_lights, lights);
}

// The queued probe nearest the eye, ties to the lower index; TOTAL for none.
static uint32_t nearest_queued(const voe_render_bounce_probes *p)
{
	uint32_t best = TOTAL;
	float best_d = INFINITY;

	for (uint32_t z = 0; z < XZ; z++)
		for (uint32_t y = 0; y < H; y++)
			for (uint32_t x = 0; x < XZ; x++) {
				const uint32_t probe = index_of(p->cell, x, y, z);

				if (!bit(p->queued, probe))
					continue;
				const voe_math_float3 c =
					centre_of(p->corner, p->spacing, x, y, z);
				const float d = c.x * c.x + c.y * c.y + c.z * c.z;

				if (d < best_d || (d == best_d && probe < best)) {
					best = probe;
					best_d = d;
				}
			}
	assert(best == TOTAL || bit(p->queued, best));
	return best;
}

uint32_t voe_render_bounce_probes_take(voe_render_bounce_probes *p,
				       uint32_t *probes, uint32_t room)
{
	assert(p != NULL && (probes != NULL || room == 0));
	uint32_t n = 0;

	while (n < room && p->placed) {
		const uint32_t probe = nearest_queued(p);

		if (probe == TOTAL)
			break;
		clear_bit(p->queued, probe);
		set_bit(p->holds, probe);
		set_bit(p->changed, probe);
		probes[n++] = probe;
	}
	assert(n <= room);
	return n;
}

// Whether `a` about corner `ca` lights the bounce as `b` about `cb` did. The
// position is about the corner, not the eye, and the shadow slot only by
// whether there is one: an eye that moves shifts both and reorders slots.
static bool lamp_same(const voe_render_point_light *a, voe_math_float3 ca,
		      const voe_render_point_light *b, voe_math_float3 cb)
{
	const float mm = 0.001f;

	assert(a != NULL && b != NULL);
	return fabsf((a->position.x - ca.x) - (b->position.x - cb.x)) <= mm &&
	       fabsf((a->position.y - ca.y) - (b->position.y - cb.y)) <= mm &&
	       fabsf((a->position.z - ca.z) - (b->position.z - cb.z)) <= mm &&
	       a->range == b->range && a->colour.x == b->colour.x &&
	       a->colour.y == b->colour.y && a->colour.z == b->colour.z &&
	       a->falloff == b->falloff && (a->shadow != 0) == (b->shadow != 0) &&
	       a->shadow_strength == b->shadow_strength &&
	       a->bounces == b->bounces &&
	       a->bounce_strength == b->bounce_strength;
}

bool voe_render_bounce_probes_relight_needed(const voe_render_bounce_probes *p,
					     const voe_render_bounce_lights *lights)
{
	assert(p != NULL && lights != NULL);
	assert(lights->lamp_count <= VOE_RENDER_BOUNCE_LAMPS);
	const voe_render_bounce_lights *r = &p->relit;

	for (uint32_t i = 0; i < TOTAL / 32; i++)
		if (p->changed[i] != 0)
			return true;
	if (memcmp(&r->sun, &lights->sun, sizeof(r->sun)) != 0 ||
	    r->sun_bounces != lights->sun_bounces ||
	    r->sun_strength != lights->sun_strength ||
	    r->lamp_count != lights->lamp_count)
		return true;
	for (uint32_t i = 0; i < lights->lamp_count; i++)
		if (!lamp_same(&lights->lamps[i], p->corner, &r->lamps[i],
			       p->relit_corner))
			return true;
	return false;
}

void voe_render_bounce_probes_relit(voe_render_bounce_probes *p,
				    const voe_render_bounce_lights *lights)
{
	assert(p != NULL && lights != NULL);
	memset(p->changed, 0, sizeof(p->changed));
	p->relit = *lights;
	p->relit_corner = p->corner;
	assert(!voe_render_bounce_probes_relight_needed(p, lights));
}
