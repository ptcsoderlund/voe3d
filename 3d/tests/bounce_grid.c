// The probe volume fitted to the level, not the eye (0331, 0332 point 2): the
// box (−20, −0.1, −20) to (20, 10, 20) fits at 2 m with its lowest cell
// (−12, −4, −12), the same cell and spacing with the eye at the origin, at
// (300, 5, −40) and 10 km out, `corner` that cell's lowest corner about each
// eye; a 60 m wide box fits at 4 m and not at 2; a box whose centre crosses a
// cell edge moves `cell` by one; no box fits at 2 m about the origin. The
// relight's sun view puts the volume's centre in the map's middle within a
// texel and its eight corners inside, for a sun at 45 degrees and one straight
// down, and its sphere, half-diagonal plus the volume's reach, grows with the
// spacing. Needs no graphics card.
#include <3d/bounce_grid.h>
#include <math/double3.h>
#include <math/float4x4.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

static const voe_math_double3 LOW = { -20.0, -0.1, -20.0 };
static const voe_math_double3 HIGH = { 20.0, 10.0, 20.0 };

// Whether `grid` is at `spacing` with its lowest cell `x`, `y`, `z`.
static bool fitted(voe_3d_bounce_grid grid, float spacing, int32_t x, int32_t y,
		   int32_t z)
{
	printf("spacing %g cell %d %d %d\n", grid.spacing, grid.cell[0],
	       grid.cell[1], grid.cell[2]);
	return grid.spacing == spacing && grid.cell[0] == x &&
	       grid.cell[1] == y && grid.cell[2] == z;
}

// The level's box seen from `eye`: the same cell and spacing, the corner the
// lowest cell's lowest corner about the eye.
static void the_eye_moves_nothing(voe_math_double3 eye)
{
	voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(LOW, HIGH, eye);
	const double at[3] = { eye.x, eye.y, eye.z };
	const float corner[3] = { grid.corner.x, grid.corner.y, grid.corner.z };

	VOE_TEST_CHECK(fitted(grid, 2.0f, -12, -4, -12));
	for (int axis = 0; axis < 3; axis++) {
		double lowest = (double)grid.cell[axis] * grid.spacing;

		VOE_TEST_CHECK(fabs(at[axis] + (double)corner[axis] - lowest) < 1e-3);
	}
}

// 60 m wide does not fit at 2 m with a cell spare; it does at 4.
static void a_wider_box_doubles_the_spacing(void)
{
	voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(
		(voe_math_double3){ -30.0, 0.0, -30.0 },
		(voe_math_double3){ 30.0, 1.0, 30.0 },
		(voe_math_double3){ 0.0, 0.0, 0.0 });

	VOE_TEST_CHECK(fitted(grid, 4.0f, -12, -6, -12));
}

// The centre at x 1.9 and at 2.1, either side of a 2 m cell edge.
static void a_centre_across_a_cell_edge_moves_one_cell(void)
{
	voe_math_double3 eye = { 0.0, 0.0, 0.0 };
	voe_3d_bounce_grid before = voe_3d_bounce_grid_fit(
		(voe_math_double3){ -8.1, 0.0, -10.0 },
		(voe_math_double3){ 11.9, 1.0, 10.0 }, eye);
	voe_3d_bounce_grid after = voe_3d_bounce_grid_fit(
		(voe_math_double3){ -7.9, 0.0, -10.0 },
		(voe_math_double3){ 12.1, 1.0, 10.0 }, eye);

	VOE_TEST_CHECK(fitted(before, 2.0f, -12, -6, -12));
	VOE_TEST_CHECK(fitted(after, 2.0f, -11, -6, -12));
}

// Min above max: the finest spacing about the world origin, the corner about
// the eye.
static void no_box_is_the_origin(void)
{
	voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(
		(voe_math_double3){ 1.0, 1.0, 1.0 },
		(voe_math_double3){ 0.0, 0.0, 0.0 },
		(voe_math_double3){ 300.0, 5.0, -40.0 });

	VOE_TEST_CHECK(fitted(grid, 2.0f, -12, -6, -12));
	VOE_TEST_CHECK(fabsf(grid.corner.x + 324.0f) < 1e-3f &&
		       fabsf(grid.corner.y + 17.0f) < 1e-3f &&
		       fabsf(grid.corner.z - 16.0f) < 1e-3f);
}

// `at`, about the eye, through `light` into the map: x and y across it, -1 to
// 1, and z its depth.
static voe_math_float4 in_the_map(voe_render_view light, voe_math_float3 at)
{
	return voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(light.projection, light.view),
		(voe_math_float4){ at.x, at.y, at.z, 1.0f });
}

// The sun view of `grid` about `eye`, for a sun along `direction`: the
// volume's centre in the map's middle within a texel, its eight corners
// inside, and the map's half width the half-diagonal plus the volume's reach.
static void the_sun_map_holds_the_volume(voe_3d_bounce_grid grid,
					 voe_math_double3 eye,
					 voe_math_float3 direction)
{
	voe_render_view light = voe_3d_bounce_grid_sun(grid, eye, direction);
	const float sides[3] = {
		(float)VOE_RENDER_BOUNCE_PROBES_XZ * grid.spacing,
		(float)VOE_RENDER_BOUNCE_PROBES_Y * grid.spacing,
		(float)VOE_RENDER_BOUNCE_PROBES_XZ * grid.spacing
	};
	float radius = 0.5f * sqrtf(sides[0] * sides[0] + sides[1] * sides[1] +
				    sides[2] * sides[2]) +
		       VOE_RENDER_BOUNCE_REACH * grid.spacing /
			       VOE_RENDER_BOUNCE_SPACING;
	float texel = 2.0f / (float)VOE_RENDER_BOUNCE_SHADOW_TEXELS;
	voe_math_float4 middle = in_the_map(
		light, (voe_math_float3){ grid.corner.x + 0.5f * sides[0],
					  grid.corner.y + 0.5f * sides[1],
					  grid.corner.z + 0.5f * sides[2] });

	printf("middle %g %g texel %g half %g radius %g\n", middle.x, middle.y,
	       texel, 1.0f / light.projection.m[0][0], radius);
	VOE_TEST_CHECK(fabsf(middle.x) <= texel && fabsf(middle.y) <= texel);
	VOE_TEST_CHECK(fabsf(1.0f / light.projection.m[0][0] - radius) <
		       1e-3f * radius);
	for (int corner = 0; corner < 8; corner++) {
		voe_math_float4 at = in_the_map(
			light,
			(voe_math_float3){
				grid.corner.x + (float)(corner & 1) * sides[0],
				grid.corner.y + (float)((corner >> 1) & 1) * sides[1],
				grid.corner.z + (float)((corner >> 2) & 1) * sides[2] });

		VOE_TEST_CHECK(fabsf(at.x) < 1.0f && fabsf(at.y) < 1.0f);
		VOE_TEST_CHECK(at.z > 0.0f && at.z < 1.0f);
	}
}

// The sun map at 2 m and at 4 m: the coarser one's sphere twice as wide.
static void the_sun_map_grows_with_the_spacing(void)
{
	voe_math_double3 eye = { 10001.3, 10.9, -2501.7 };
	voe_3d_bounce_grid fine = voe_3d_bounce_grid_fit(LOW, HIGH, eye);
	voe_3d_bounce_grid coarse = voe_3d_bounce_grid_fit(
		(voe_math_double3){ -30.0, 0.0, -30.0 },
		(voe_math_double3){ 30.0, 1.0, 30.0 }, eye);
	voe_math_float3 down = { 0.0f, -1.0f, 0.0f };

	VOE_TEST_CHECK(coarse.spacing == 2.0f * fine.spacing);
	the_sun_map_holds_the_volume(fine, eye,
				     (voe_math_float3){ -0.70710678f,
							-0.70710678f, 0.0f });
	the_sun_map_holds_the_volume(fine, eye, down);
	the_sun_map_holds_the_volume(coarse, eye, down);
	VOE_TEST_CHECK(
		fabsf(voe_3d_bounce_grid_sun(fine, eye, down).projection.m[0][0] -
		      2.0f * voe_3d_bounce_grid_sun(coarse, eye, down)
				     .projection.m[0][0]) < 1e-6f);
}

int main(void)
{
	the_eye_moves_nothing((voe_math_double3){ 0.0, 0.0, 0.0 });
	the_eye_moves_nothing((voe_math_double3){ 300.0, 5.0, -40.0 });
	the_eye_moves_nothing((voe_math_double3){ 10000.0, 3.0, -10000.0 });
	a_wider_box_doubles_the_spacing();
	a_centre_across_a_cell_edge_moves_one_cell();
	no_box_is_the_origin();
	the_sun_map_grows_with_the_spacing();

	return voe_test_result();
}
