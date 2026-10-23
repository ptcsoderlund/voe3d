// The grown foliage of 3d/foliage_grow.h: the area's copies of the remade
// kinds removed, then each kind's lattice cells over the area hashed, jittered,
// kept by the paint and placed.
//
// Used by the model store when a load, a stamp, an undo or a kind's setting
// asks for a remake. Positions are whole millimetres; a copy's metres are its
// millimetres / 1000 divided, not multiplied, so a point on a grid line in
// millimetres is on it in metres and its bilinear weight is exact.
//
// Constraints: the lattice cells walked are those over the area plus one each
// side, so a point rounded across a cell's edge is still found; a whole-grid
// remake at 50 copies per 100 m² on the largest grid walks about 3·10⁷ cells
// a kind, which a coarser lattice for the far part of a grid would lift.
#include <3d/foliage_grow.h>

#include <3d/foliage.h>

#include <base/assert.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#define TURN_UNITS 65536.0f

// The area in metres, half-open.
struct area {
	float x0, z0, x1, z1;
};

static float metres(int32_t millimetres)
{
	return (float)millimetres / 1000.0f;
}

static bool inside(struct area a, float x, float z)
{
	return x >= a.x0 && x < a.x1 && z >= a.z0 && z < a.z1;
}

// splitmix64's finaliser.
static uint64_t mix(uint64_t x)
{
	x += 0x9e3779b97f4a7c15u;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9u;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebu;
	return x ^ (x >> 31);
}

// The next number in [0, 1) from the cell's hash, which is advanced.
static float draw(uint64_t *hash)
{
	*hash = mix(*hash);
	return (float)(*hash >> 40) * 0x1p-24f;
}

// Layer `layer`'s bilinear weight at (x, z), 0..255, the point clamped onto
// the grid; paint NULL is all layer 0.
static float layer_weight(const voe_assets_landscape *landscape, uint32_t layer,
			  float x, float z)
{
	VOE_BASE_ASSERT(layer < VOE_ASSETS_LANDSCAPE_LAYERS, "a layer");
	if (!landscape->paint)
		return layer == 0 ? 255.0f : 0.0f;
	const uint32_t n = landscape->cells + 1;
	const float cell = landscape->size / (float)landscape->cells;
	const float half = 0.5f * landscape->size;
	const float u = fminf(fmaxf((x + half) / cell, 0.0f),
			      (float)landscape->cells);
	const float v = fminf(fmaxf((z + half) / cell, 0.0f),
			      (float)landscape->cells);
	const uint32_t c = (uint32_t)fminf(floorf(u), (float)(n - 2));
	const uint32_t r = (uint32_t)fminf(floorf(v), (float)(n - 2));
	const float s = u - (float)c, t = v - (float)r;
	const uint8_t *plane = &landscape->paint[(layer / 4) * n * n * 4];
	const uint32_t at = (r * n + c) * 4 + layer % 4;
	const float top = (1.0f - s) * (float)plane[at] +
			  s * (float)plane[at + 4];
	const float bottom = (1.0f - s) * (float)plane[at + n * 4] +
			     s * (float)plane[at + n * 4 + 4];

	VOE_BASE_ASSERT(r + 1 < n && c + 1 < n, "a cell inside the grid");
	return (1.0f - t) * top + t * bottom;
}

// d: the tenths of kind `kind` at (x, z), each layer's weight / 255 times its
// grows, summed.
static float tenths_at(const voe_assets_landscape *landscape, uint32_t kind,
		       float x, float z)
{
	float sum = 0.0f;

	for (uint32_t l = 0; l < landscape->layer_count; l++)
		if (landscape->grows[l][kind] > 0)
			sum += layer_weight(landscape, l, x, z) / 255.0f *
			       (float)landscape->grows[l][kind];
	return sum;
}

// D: the most tenths any layer grows of kind `kind`, 0 for none.
static uint16_t most_tenths(const voe_assets_landscape *landscape,
			    uint32_t kind)
{
	uint16_t most = 0;

	VOE_BASE_ASSERT(landscape->layer_count <= VOE_ASSETS_LANDSCAPE_LAYERS,
			"at most 8 layers");
	for (uint32_t l = 0; l < landscape->layer_count; l++)
		if (landscape->grows[l][kind] > most)
			most = landscape->grows[l][kind];
	VOE_BASE_ASSERT(most <= VOE_ASSETS_LANDSCAPE_GROW_MAX, "a grow in range");
	return most;
}

// The copy at a cell's point, its scale and turn drawn from `hash` as a
// scatter draws them from its kind's look.
static voe_assets_landscape_copy copy_at(const voe_assets_landscape *landscape,
					 uint32_t kind, float x, float z,
					 uint64_t *hash)
{
	const voe_assets_landscape_look look = landscape->looks[kind];
	const float size = draw(hash);
	const float turn = draw(hash);
	const float scale =
		(1.0f + (float)look.size_vary / 100.0f * (2.0f * size - 1.0f)) *
		1000.0f;
	const long units = lroundf((float)look.turn_vary * (2.0f * turn - 1.0f) /
				   360.0f * TURN_UNITS);

	VOE_BASE_ASSERT(look.size_vary <= VOE_ASSETS_LANDSCAPE_SIZE_VARY_MAX &&
				look.tilt <= VOE_ASSETS_LANDSCAPE_TILT_MAX,
			"a kind's spreads in range");
	return (voe_assets_landscape_copy){
		.x = (int32_t)lroundf(x * 1000.0f),
		.z = (int32_t)lroundf(z * 1000.0f),
		.turn = (uint16_t)(uint32_t)units,
		.scale = (uint16_t)lroundf(fminf(fmaxf(scale, 1.0f), 65535.0f)),
		.kind = (uint8_t)kind,
		.lean = (uint8_t)lroundf((float)look.tilt / 100.0f * 255.0f),
	};
}

// The lattice cells, on one axis, whose point may land in [lo, hi): those
// over it and one each side, within the `side` cells of the grid.
static void cell_span(float lo, float hi, float half, float spacing,
		      uint32_t side, uint32_t *first, uint32_t *end)
{
	const float a = floorf((lo + half) / spacing) - 1.0f;
	const float b = floorf((hi + half) / spacing) + 2.0f;

	*first = a <= 0.0f ? 0u : (uint32_t)fminf(a, (float)side);
	*end = b <= 0.0f ? 0u : (uint32_t)fminf(b, (float)side);
}

// Kind `kind`'s kept lattice points in the area appended to the cap.
static uint32_t grow_kind(const voe_assets_landscape *landscape, uint32_t kind,
			  struct area a, voe_assets_landscape_copy *grown,
			  voe_scene_transform *placed, uint32_t count)
{
	const uint16_t most = most_tenths(landscape, kind);

	if (most == 0)
		return count;
	const float spacing = sqrtf(1000.0f / (float)most);
	const float half = 0.5f * landscape->size;
	const uint32_t side = (uint32_t)ceilf(landscape->size / spacing);
	uint32_t x0, x1, z0, z1;

	cell_span(a.x0, a.x1, half, spacing, side, &x0, &x1);
	cell_span(a.z0, a.z1, half, spacing, side, &z0, &z1);
	for (uint32_t cz = z0; cz < z1 && count < VOE_3D_FOLIAGE_GROWN; cz++) {
		for (uint32_t cx = x0; cx < x1; cx++) {
			uint64_t hash = mix(((uint64_t)kind << 48) ^
					    ((uint64_t)cx << 24) ^ cz);
			const float px = -half + ((float)cx + draw(&hash)) * spacing;
			const float pz = -half + ((float)cz + draw(&hash)) * spacing;
			const float keep = draw(&hash);
			const voe_assets_landscape_copy copy =
				copy_at(landscape, kind, px, pz, &hash);
			const float x = metres(copy.x), z = metres(copy.z);

			if (!inside(a, x, z) || x < -half || x >= half ||
			    z < -half || z >= half ||
			    keep * (float)most >= tenths_at(landscape, kind, x, z))
				continue;
			if (count >= VOE_3D_FOLIAGE_GROWN)
				break;
			grown[count] = copy;
			placed[count] = voe_3d_foliage_place(landscape, copy);
			count++;
		}
	}
	return count;
}

uint32_t voe_3d_foliage_grow(const voe_assets_landscape *landscape,
			     voe_3d_landscape_rect rect, uint32_t kinds,
			     voe_assets_landscape_copy *grown,
			     voe_scene_transform *placed, uint32_t count)
{
	VOE_BASE_ASSERT(landscape && landscape->heights && grown && placed,
			"a landscape and room for its grown copies");
	VOE_BASE_ASSERT(count <= VOE_3D_FOLIAGE_GROWN &&
				landscape->kind_count <= VOE_ASSETS_LANDSCAPE_KINDS,
			"a count and kinds within their caps");
	if (rect.x0 >= rect.x1 || rect.z0 >= rect.z1)
		return count;
	const float half = 0.5f * landscape->size;
	const float cell = landscape->size / (float)landscape->cells;
	const struct area a = {
		.x0 = -half + ((float)rect.x0 - 1.0f) * cell,
		.z0 = -half + ((float)rect.z0 - 1.0f) * cell,
		.x1 = -half + (float)rect.x1 * cell,
		.z1 = -half + (float)rect.z1 * cell,
	};
	const uint32_t held = kinds & ((1u << landscape->kind_count) - 1u);

	// From the last down, so the copy moved in has been looked at already.
	for (uint32_t i = count; i > 0; i--) {
		const voe_assets_landscape_copy copy = grown[i - 1];

		if (!((held >> copy.kind) & 1u) ||
		    !inside(a, metres(copy.x), metres(copy.z)))
			continue;
		count--;
		grown[i - 1] = grown[count];
		placed[i - 1] = placed[count];
	}
	for (uint32_t k = 0; k < landscape->kind_count; k++)
		if ((held >> k) & 1u)
			count = grow_kind(landscape, k, a, grown, placed, count);
	return count;
}
