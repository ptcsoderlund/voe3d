// The landscape arithmetic of 3d/landscape.h: the bilinear height between four
// heights, a ray meeting a raised cell and missing beside the grid, raise,
// smooth and flatten by 0379 point 3's rates. Needs no graphics card.
#include <3d/landscape.h>
#include <assets/landscape.h>
#include <base/arena.h>

#include <testing/test.h>

#include <math.h>

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

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(ARENA);

	height_is_bilinear_between_four(arena);
	ray_meets_a_raised_cell(arena);
	ray_beside_the_grid_misses(arena);
	raise_lifts_the_middle_most_and_the_rim_not_at_all(arena);
	smooth_lowers_a_spike(arena);
	flatten_moves_toward_the_target(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
