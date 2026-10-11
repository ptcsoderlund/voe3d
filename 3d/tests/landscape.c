// The landscape arithmetic of 3d/landscape.h: the bilinear height between four
// heights, a ray meeting a raised cell and missing beside the grid, raise,
// smooth and flatten by 0379 point 3's rates, and paint raising a layer, the
// sum kept and a light stroke stopping at its strength. Needs no graphics card.
#include <3d/landscape.h>
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ARENA (1024 * 1024)

static float *height_at(voe_assets_landscape *land, uint32_t c, uint32_t r)
{
	return &land->heights[(size_t)r * (land->cells + 1) + c];
}

// 16 m with 4 cells puts a height every 4 m; (-3, -2) is a quarter along x
// and half along z between heights 1, 2, 3 and 4. Off the grid it clamps.
static void height_is_bilinear_between_four(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 4, arena);

	*height_at(&land, 1, 1) = 1.0f;
	*height_at(&land, 2, 1) = 2.0f;
	*height_at(&land, 1, 2) = 3.0f;
	*height_at(&land, 2, 2) = 4.0f;
	VOE_TEST_CHECK_FLOAT(voe_3d_landscape_height(&land, -3.0f, -2.0f),
			     2.25f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(voe_3d_landscape_height(&land, -4.0f, -4.0f),
			     1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(voe_3d_landscape_height(&land, 100.0f, 0.0f),
			     0.0f, 1e-5f);
}

// A 4 m peak in the middle: straight down from 10 m meets it 6 m on, and a
// level ray at 1 m from the side meets its slope where it is 1 m, 3 m short
// of the middle.
static void ray_meets_a_raised_cell(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 4, arena);
	float distance = 0.0f;

	*height_at(&land, 2, 2) = 4.0f;
	VOE_TEST_CHECK(voe_3d_landscape_ray(&land, (voe_math_float3){ 0, 10, 0 },
					    (voe_math_float3){ 0, -1, 0 },
					    &distance));
	VOE_TEST_CHECK_FLOAT(distance, 6.0f, 1e-3f);
	VOE_TEST_CHECK(voe_3d_landscape_ray(&land,
					    (voe_math_float3){ -20, 1, 0 },
					    (voe_math_float3){ 1, 0, 0 },
					    &distance));
	VOE_TEST_CHECK_FLOAT(distance, 17.0f, 1e-3f);
}

// Down beside the grid, and level above its highest height, meet nothing.
static void ray_beside_the_grid_misses(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 4, arena);
	float distance = -1.0f;

	*height_at(&land, 2, 2) = 4.0f;
	VOE_TEST_CHECK(!voe_3d_landscape_ray(&land,
					     (voe_math_float3){ 20, 10, 0 },
					     (voe_math_float3){ 0, -1, 0 },
					     &distance));
	VOE_TEST_CHECK(!voe_3d_landscape_ray(&land,
					     (voe_math_float3){ -20, 5, 0 },
					     (voe_math_float3){ 1, 0, 0 },
					     &distance));
	VOE_TEST_CHECK_FLOAT(distance, -1.0f, 0.0f);
}

// Radius 16, softness 0.5, strength 1, one second clamped to 0.1: the middle
// rises 1·16·0.5·0.1 = 0.8 m, 12 m out less, the rim at 16 m not at all.
static void raise_lifts_the_middle_most_and_the_rim_not_at_all(
	voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(64.0f, 16, arena);
	const voe_3d_brush brush = { .kind = VOE_3D_BRUSH_RAISE,
				     .radius = 16.0f,
				     .strength = 1.0f,
				     .softness = 0.5f };
	const voe_3d_landscape_rect rect =
		voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 1.0f, arena);

	VOE_TEST_CHECK_FLOAT(*height_at(&land, 8, 8), 0.8f, 1e-5f);
	VOE_TEST_CHECK(*height_at(&land, 11, 8) > 0.0f);
	VOE_TEST_CHECK(*height_at(&land, 11, 8) < 0.8f);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 12, 8), 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 8, 4), 0.0f, 0.0f);
	VOE_TEST_CHECK(rect.x0 >= 4 && rect.x0 <= 8 && rect.x1 > 8 &&
		       rect.x1 <= 13);
	VOE_TEST_CHECK(rect.z0 >= 4 && rect.z0 <= 8 && rect.z1 > 8 &&
		       rect.z1 <= 13);
}

// A 10 m spike under a full smooth goes to its neighbours' mean, 0; a
// neighbour rises toward its own mean, the spike counted in it.
static void smooth_lowers_a_spike(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(64.0f, 16, arena);
	const voe_3d_brush brush = { .kind = VOE_3D_BRUSH_SMOOTH,
				     .radius = 8.0f,
				     .strength = 1.0f,
				     .softness = 0.0f };

	*height_at(&land, 8, 8) = 10.0f;
	(void)voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 0.1f, arena);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 8, 8), 0.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 9, 8), 1.25f, 1e-5f);
}

// Strength 0.5 for 0.1 s moves the middle half way to a 2 m target.
static void flatten_moves_toward_the_target(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(64.0f, 16, arena);
	const voe_3d_brush brush = { .kind = VOE_3D_BRUSH_FLATTEN,
				     .radius = 8.0f,
				     .strength = 0.5f,
				     .softness = 0.5f,
				     .target = 2.0f };

	(void)voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 0.1f, arena);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 8, 8), 1.0f, 1e-5f);
	VOE_TEST_CHECK_FLOAT(*height_at(&land, 10, 8), 0.0f, 0.0f);
}

// 16 m with 8 cells, a point every 2 m, the centre at (4, 4); three layers
// and paint all layer 0.
static voe_assets_landscape painted_flat(voe_base_arena *arena)
{
	voe_assets_landscape land = voe_assets_landscape_flat(16.0f, 8, arena);
	const size_t points = 9 * 9;

	land.layer_count = 3;
	land.paint = voe_base_arena_push(arena, points * 8);
	for (size_t i = 0; i < points * 8; i++)
		land.paint[i] = (i < points * 4 && i % 4 == 0) ? 255 : 0;
	return land;
}

static uint8_t layer_at(const voe_assets_landscape *land, uint32_t c,
			uint32_t r, uint32_t layer)
{
	const size_t point = (size_t)r * 9 + c;

	return land->paint[(layer / 4) * 81 * 4 + point * 4 + layer % 4];
}

static bool every_point_sums_to_255(const voe_assets_landscape *land)
{
	for (uint32_t r = 0; r < 9; r++) {
		for (uint32_t c = 0; c < 9; c++) {
			uint32_t sum = 0;

			for (uint32_t k = 0; k < 8; k++)
				sum += layer_at(land, c, r, k);
			if (sum != 255)
				return false;
		}
	}
	return true;
}

// Strength 1 for 0.1 s moves layer 2 at the centre 8·0.1 of the way to 255,
// 204, and layer 0 down to the 51 left; 6 m out, past the 4 m radius, nothing.
static void paint_raises_a_layer_and_keeps_the_sum(voe_base_arena *arena)
{
	voe_assets_landscape land = painted_flat(arena);
	const voe_3d_brush brush = { .kind = VOE_3D_BRUSH_PAINT,
				     .radius = 4.0f,
				     .strength = 1.0f,
				     .softness = 0.5f,
				     .layer = 2 };
	const voe_3d_landscape_rect rect =
		voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 0.1f, arena);

	VOE_TEST_CHECK(layer_at(&land, 4, 4, 2) == 204);
	VOE_TEST_CHECK(layer_at(&land, 4, 4, 0) == 51);
	VOE_TEST_CHECK(every_point_sums_to_255(&land));
	VOE_TEST_CHECK(layer_at(&land, 4, 7, 0) == 255);
	VOE_TEST_CHECK(layer_at(&land, 4, 7, 2) == 0);
	VOE_TEST_CHECK(rect.x0 <= 4 && rect.x1 > 4 && rect.z0 <= 4 &&
		       rect.z1 > 4 && rect.x1 <= 7 && rect.z1 <= 7);
}

// Held at strength 0.3, layer 2 stops at 0.3·255 and never passes it; layer
// 0 painted over it then takes layer 2 down.
static void a_light_stroke_stops_at_its_strength(voe_base_arena *arena)
{
	voe_assets_landscape land = painted_flat(arena);
	voe_3d_brush brush = { .kind = VOE_3D_BRUSH_PAINT,
			       .radius = 4.0f,
			       .strength = 0.3f,
			       .softness = 0.5f,
			       .layer = 2 };
	bool never_above = true;

	for (int i = 0; i < 50; i++) {
		(void)voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 0.1f,
					     arena);
		if (layer_at(&land, 4, 4, 2) > 0.3f * 255.0f)
			never_above = false;
	}
	VOE_TEST_CHECK(never_above);
	VOE_TEST_CHECK_FLOAT((float)layer_at(&land, 4, 4, 2), 0.3f * 255.0f,
			     2.0f);
	VOE_TEST_CHECK(every_point_sums_to_255(&land));
	const uint8_t before = layer_at(&land, 4, 4, 2);

	brush.layer = 0;
	brush.strength = 1.0f;
	(void)voe_3d_landscape_brush(&land, &brush, 0.0f, 0.0f, 0.1f, arena);
	VOE_TEST_CHECK(layer_at(&land, 4, 4, 2) < before);
	VOE_TEST_CHECK(every_point_sums_to_255(&land));
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);

	height_is_bilinear_between_four(arena);
	ray_meets_a_raised_cell(arena);
	ray_beside_the_grid_misses(arena);
	raise_lifts_the_middle_most_and_the_rim_not_at_all(arena);
	smooth_lowers_a_spike(arena);
	flatten_moves_toward_the_target(arena);
	paint_raises_a_layer_and_keeps_the_sum(arena);
	a_light_stroke_stops_at_its_strength(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
