// The foliage arithmetic of 3d/foliage.h: a copy placed on the bilinear
// ground, a scatter stamp's tries drawn up front and refused against the
// copies near it, erase and undo's removal by moving the last copy in, and
// the copies over changed heights placed again.
//
// Used by the model store (the stamps, the puts and the replace after a
// sculpt) and the editor (the brush). Copies are in millimetres; the arithmetic
// here is in metres, a copy's position read as its millimetres / 1000.
//
// Constraints: a scatter keeps its tries on the stack, VOE_3D_FOLIAGE_TRIES of
// them; it walks every copy once, box-tested, and checks each near one against
// every try, and each try against the ones kept before it. Removing by value
// scans the copies once per copy removed. Nothing here allocates.
#include <3d/foliage.h>

#include "brush_weight.h"

#include <base/assert.h>
#include <math/float3.h>
#include <math/quat.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LONGEST_SECONDS 0.1f
#define SPACING 0.84f
#define TWO_PI 6.283185307f
#define TURN_UNITS 65536.0f

// One drawn point of a stamp, kept or not.
struct foliage_try {
	voe_assets_landscape_copy copy;
	bool refused;
};

static float metres(int32_t millimetres)
{
	return (float)millimetres * 0.001f;
}

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

// The picked kinds the landscape has.
static uint32_t held_kinds(const voe_assets_landscape *landscape,
			   uint32_t picked)
{
	VOE_BASE_ASSERT(landscape->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"at most 16 kinds");
	const uint32_t mask = (1u << landscape->kind_count) - 1u;

	return picked & mask;
}

// The kind at `share` (0..1) along the set bits of `kinds`, which has one.
static uint8_t nth_kind(uint32_t kinds, float share)
{
	uint32_t count = 0;

	for (uint32_t k = 0; k < VOE_ASSETS_LANDSCAPE_KINDS; k++)
		count += (kinds >> k) & 1u;
	VOE_BASE_ASSERT(count > 0, "a picked kind");
	uint32_t index = (uint32_t)(share * (float)count);

	index = index < count ? index : count - 1;
	for (uint32_t k = 0; k < VOE_ASSETS_LANDSCAPE_KINDS; k++) {
		if (!((kinds >> k) & 1u))
			continue;
		if (index == 0)
			return (uint8_t)k;
		index--;
	}
	VOE_BASE_ASSERT(false, "the index is below the count");
	return 0;
}

static bool nearer_than(voe_assets_landscape_copy a, voe_assets_landscape_copy b,
			float spacing)
{
	const float dx = metres(a.x - b.x);
	const float dz = metres(a.z - b.z);

	return dx * dx + dz * dz < spacing * spacing;
}

voe_scene_transform voe_3d_foliage_place(const voe_assets_landscape *landscape,
					 voe_assets_landscape_copy copy)
{
	VOE_BASE_ASSERT(landscape && landscape->heights, "a landscape");
	VOE_BASE_ASSERT(copy.scale >= 1, "a scale of at least a thousandth");
	const float x = metres(copy.x);
	const float z = metres(copy.z);
	const float cell = landscape->size / (float)landscape->cells;
	const float slope_x = (voe_3d_landscape_height(landscape, x + cell, z) -
			       voe_3d_landscape_height(landscape, x - cell, z)) /
			      (2.0f * cell);
	const float slope_z = (voe_3d_landscape_height(landscape, x, z + cell) -
			       voe_3d_landscape_height(landscape, x, z - cell)) /
			      (2.0f * cell);
	const voe_math_float3 normal = voe_math_float3_normalize(
		(voe_math_float3){ -slope_x, 1.0f, -slope_z });
	// +Y × normal: the axis that takes up toward the normal.
	const voe_math_float3 axis = { normal.z, 0.0f, -normal.x };
	const float scale = (float)copy.scale * 0.001f;
	voe_math_quat rotation = voe_math_quat_from_axis_angle(
		(voe_math_float3){ 0.0f, 1.0f, 0.0f },
		(float)copy.turn / TURN_UNITS * TWO_PI);

	if (copy.lean > 0 && axis.x * axis.x + axis.z * axis.z > 1e-6f)
		rotation = voe_math_quat_mul(
			voe_math_quat_from_axis_angle(
				axis, acosf(fminf(normal.y, 1.0f)) *
					      (float)copy.lean / 255.0f),
			rotation);
	return (voe_scene_transform){
		.position = { x, voe_3d_landscape_height(landscape, x, z), z },
		.rotation = rotation,
		.scale = { scale, scale, scale },
	};
}

// One try: a point uniform in the disc and its copy, refused already when it
// is off the square or its weight does not beat a draw. Six draws always.
static struct foliage_try try_point(const voe_assets_landscape *landscape,
				    const voe_3d_foliage_brush *brush, float x,
				    float z, uint32_t kinds, uint64_t *seed)
{
	const float reach = brush->radius * sqrtf(draw(seed));
	const float angle = TWO_PI * draw(seed);
	const float beat = draw(seed);
	const float kind = draw(seed);
	const float size = draw(seed);
	const float turn = draw(seed);
	const float px = x + reach * cosf(angle);
	const float pz = z + reach * sinf(angle);
	const float half = 0.5f * landscape->size;
	const float scale = (1.0f + brush->size_vary * (2.0f * size - 1.0f)) *
			    1000.0f;
	const long units = lroundf(brush->turn_vary * (2.0f * turn - 1.0f) /
				   TWO_PI * TURN_UNITS);
	const float lean = fminf(fmaxf(brush->lean, 0.0f), 1.0f) * 255.0f;
	const float weight =
		voe_3d_brush_weight(brush->radius, brush->softness, reach);
	struct foliage_try out = {
		.copy = {
			.x = (int32_t)lroundf(px * 1000.0f),
			.z = (int32_t)lroundf(pz * 1000.0f),
			.turn = (uint16_t)(uint32_t)units,
			.scale = (uint16_t)lroundf(
				fminf(fmaxf(scale, 1.0f), 65535.0f)),
			.kind = nth_kind(kinds, kind),
			.lean = (uint8_t)lroundf(lean),
		},
	};

	out.refused = fabsf(px) > half || fabsf(pz) > half || weight <= beat;
	VOE_BASE_ASSERT(out.copy.scale >= 1, "a scale");
	return out;
}

// Each try within `spacing` of a copy whose position lies within `reach` of
// (x, z) on both axes refused: one walk over the copies.
static void refuse_near_copies(const voe_assets_landscape *landscape,
			       struct foliage_try *tries, uint32_t count,
			       float x, float z, float reach, float spacing)
{
	VOE_BASE_ASSERT(tries && count <= VOE_3D_FOLIAGE_TRIES, "the tries");
	VOE_BASE_ASSERT(reach > 0.0f, "a reach");
	for (uint32_t i = 0; i < landscape->copy_count; i++) {
		const voe_assets_landscape_copy near = landscape->copies[i];

		if (fabsf(metres(near.x) - x) > reach ||
		    fabsf(metres(near.z) - z) > reach)
			continue;
		for (uint32_t t = 0; t < count; t++)
			if (!tries[t].refused &&
			    nearer_than(tries[t].copy, near, spacing))
				tries[t].refused = true;
	}
}

static void append(voe_assets_landscape *landscape, voe_scene_transform *placed,
		   voe_assets_landscape_copy copy)
{
	VOE_BASE_ASSERT(landscape->copy_count < VOE_ASSETS_LANDSCAPE_COPIES,
			"room for a copy");
	VOE_BASE_ASSERT(copy.kind < landscape->kind_count, "a held kind");
	landscape->copies[landscape->copy_count] = copy;
	if (placed)
		placed[landscape->copy_count] = voe_3d_foliage_place(landscape, copy);
	landscape->copy_count++;
}

uint32_t voe_3d_foliage_scatter(voe_assets_landscape *landscape,
				voe_scene_transform *placed,
				const voe_3d_foliage_brush *brush, float x,
				float z, float seconds, uint64_t *seed,
				voe_assets_landscape_copy *added)
{
	VOE_BASE_ASSERT(landscape && landscape->heights && landscape->copies,
			"a landscape with room for its copies");
	VOE_BASE_ASSERT(brush && seed && *seed != 0 && added,
			"a brush, a nonzero seed and room for what is added");
	VOE_BASE_ASSERT(brush->radius > 0.0f && brush->density > 0.0f &&
				seconds >= 0.0f,
			"a radius, a density and time not running backwards");
	const uint32_t kinds = held_kinds(landscape, brush->picked);
	const float s = fminf(seconds, LONGEST_SECONDS);
	const float wanted = ceilf(brush->density / 100.0f * 3.14159265f *
				   brush->radius * brush->radius *
				   fminf(1.0f, 4.0f * s));
	const uint32_t count =
		(uint32_t)fminf(wanted, (float)VOE_3D_FOLIAGE_TRIES);
	const float spacing = SPACING * sqrtf(100.0f / brush->density);
	struct foliage_try tries[VOE_3D_FOLIAGE_TRIES];
	uint32_t kept = 0;

	if (kinds == 0)
		return 0;
	for (uint32_t t = 0; t < count; t++)
		tries[t] = try_point(landscape, brush, x, z, kinds, seed);
	refuse_near_copies(landscape, tries, count, x, z,
			   brush->radius + spacing, spacing);
	for (uint32_t t = 0; t < count; t++) {
		bool crowded = tries[t].refused;

		if (landscape->copy_count >= VOE_ASSETS_LANDSCAPE_COPIES)
			break;
		for (uint32_t k = 0; k < kept && !crowded; k++)
			crowded = nearer_than(tries[t].copy, added[k], spacing);
		if (crowded)
			continue;
		append(landscape, placed, tries[t].copy);
		added[kept++] = tries[t].copy;
	}
	VOE_BASE_ASSERT(kept <= VOE_3D_FOLIAGE_TRIES, "added fits its room");
	return kept;
}

// Copy `i` removed by moving the last into its place, `placed` alike.
static void remove_at(voe_assets_landscape *landscape,
		      voe_scene_transform *placed, uint32_t i)
{
	VOE_BASE_ASSERT(i < landscape->copy_count, "a copy");
	const uint32_t last = landscape->copy_count - 1;

	landscape->copies[i] = landscape->copies[last];
	if (placed)
		placed[i] = placed[last];
	landscape->copy_count = last;
	VOE_BASE_ASSERT(landscape->copy_count < VOE_ASSETS_LANDSCAPE_COPIES,
			"one fewer");
}

// Walked from the last down, so the copy moved in has been looked at already.
uint32_t voe_3d_foliage_erase(voe_assets_landscape *landscape,
			      voe_scene_transform *placed,
			      const voe_3d_foliage_brush *brush, float x, float z,
			      voe_assets_landscape_copy *removed, uint32_t room)
{
	VOE_BASE_ASSERT(landscape && landscape->copies && brush,
			"a landscape with its copies and a brush");
	VOE_BASE_ASSERT(removed || room == 0, "room for what is removed");
	const uint32_t kinds = held_kinds(landscape, brush->picked);
	const float radius_squared = brush->radius * brush->radius;
	uint32_t taken = 0;

	for (uint32_t i = landscape->copy_count; i > 0 && taken < room; i--) {
		const voe_assets_landscape_copy copy = landscape->copies[i - 1];
		const float dx = metres(copy.x) - x;
		const float dz = metres(copy.z) - z;

		if (!((kinds >> copy.kind) & 1u) ||
		    dx * dx + dz * dz > radius_squared)
			continue;
		removed[taken++] = copy;
		remove_at(landscape, placed, i - 1);
	}
	return taken;
}

static bool same_copy(voe_assets_landscape_copy a, voe_assets_landscape_copy b)
{
	return a.x == b.x && a.z == b.z && a.turn == b.turn &&
	       a.scale == b.scale && a.kind == b.kind && a.lean == b.lean;
}

void voe_3d_foliage_put(voe_assets_landscape *landscape,
			voe_scene_transform *placed,
			const voe_assets_landscape_copy *copies, uint32_t count,
			bool add)
{
	VOE_BASE_ASSERT(landscape && landscape->heights && landscape->copies,
			"a landscape with room for its copies");
	VOE_BASE_ASSERT(copies || count == 0, "the copies put");
	for (uint32_t k = 0; k < count; k++) {
		if (add) {
			if (landscape->copy_count >= VOE_ASSETS_LANDSCAPE_COPIES)
				return;
			append(landscape, placed, copies[k]);
			continue;
		}
		for (uint32_t i = 0; i < landscape->copy_count; i++) {
			if (same_copy(landscape->copies[i], copies[k])) {
				remove_at(landscape, placed, i);
				break;
			}
		}
	}
}

void voe_3d_foliage_replace(const voe_assets_landscape *landscape,
			    voe_scene_transform *placed,
			    voe_3d_landscape_rect heights)
{
	VOE_BASE_ASSERT(landscape && landscape->heights, "a landscape");
	VOE_BASE_ASSERT(placed || landscape->copy_count == 0,
			"the placed transforms");
	if (heights.x0 >= heights.x1 || heights.z0 >= heights.z1)
		return;
	const float half = 0.5f * landscape->size;
	const float cell = landscape->size / (float)landscape->cells;
	const float x0 = -half + ((float)heights.x0 - 1.0f) * cell;
	const float x1 = -half + (float)heights.x1 * cell;
	const float z0 = -half + ((float)heights.z0 - 1.0f) * cell;
	const float z1 = -half + (float)heights.z1 * cell;

	for (uint32_t i = 0; i < landscape->copy_count; i++) {
		const voe_assets_landscape_copy copy = landscape->copies[i];
		const float x = metres(copy.x);
		const float z = metres(copy.z);

		if (x >= x0 && x <= x1 && z >= z0 && z <= z1)
			placed[i] = voe_3d_foliage_place(landscape, copy);
	}
}
