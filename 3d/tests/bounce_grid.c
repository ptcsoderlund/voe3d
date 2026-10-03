// The probe volume fitted to the eye: that the eye's cell is the grid's lowest
// cell plus (12, VOE_3D_BOUNCE_BELOW, 12) and its corner that cell's lowest
// corner about the eye, near the origin, at negative coordinates and 10 km out;
// that 2 m along x moves it one cell; that a move inside the eye's cell keeps
// it; and that the relight's sun view puts the volume's centre in the map's
// middle within a texel and its eight corners inside, for a sun at 45 degrees
// and one straight down. Needs no graphics card.
#include <3d/bounce_grid.h>
#include <math/double3.h>
#include <math/float4x4.h>

#include <testing/test.h>

#include <math.h>
#include <stdio.h>

static void the_eye_stands_in_its_cell(voe_math_double3 eye)
{
	voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(eye);
	const double at[3] = { eye.x, eye.y, eye.z };
	const int32_t below[3] = { VOE_RENDER_BOUNCE_PROBES_XZ / 2,
				   VOE_3D_BOUNCE_BELOW,
				   VOE_RENDER_BOUNCE_PROBES_XZ / 2 };
	const float corner[3] = { grid.corner.x, grid.corner.y, grid.corner.z };

	VOE_TEST_CHECK_INT(below[0], 12);
	VOE_TEST_CHECK_INT(below[1], 8);
	for (int axis = 0; axis < 3; axis++) {
		int32_t own = (int32_t)floor(at[axis] / VOE_RENDER_BOUNCE_SPACING);
		double lowest = (double)grid.cell[axis] * VOE_RENDER_BOUNCE_SPACING;

		VOE_TEST_CHECK_INT(grid.cell[axis] + below[axis], own);
		VOE_TEST_CHECK(fabs(at[axis] + (double)corner[axis] - lowest) < 1e-3);
	}
}

static void two_metres_along_x_is_one_cell(void)
{
	voe_3d_bounce_grid before =
		voe_3d_bounce_grid_fit((voe_math_double3){ 0.7, 1.5, -3.2 });
	voe_3d_bounce_grid after =
		voe_3d_bounce_grid_fit((voe_math_double3){ 2.7, 1.5, -3.2 });

	VOE_TEST_CHECK_INT(after.cell[0], before.cell[0] + 1);
	VOE_TEST_CHECK_INT(after.cell[1], before.cell[1]);
	VOE_TEST_CHECK_INT(after.cell[2], before.cell[2]);
}

static void a_move_inside_the_cell_keeps_it(void)
{
	voe_3d_bounce_grid before =
		voe_3d_bounce_grid_fit((voe_math_double3){ 0.2, 4.1, -3.9 });
	voe_3d_bounce_grid after =
		voe_3d_bounce_grid_fit((voe_math_double3){ 1.9, 5.8, -2.1 });

	for (int axis = 0; axis < 3; axis++)
		VOE_TEST_CHECK_INT(after.cell[axis], before.cell[axis]);
}

// `at`, about the eye, through `light` into the map: x and y across it, -1 to
// 1, and z its depth.
static voe_math_float4 in_the_map(voe_render_view light, voe_math_float3 at)
{
	return voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(light.projection, light.view),
		(voe_math_float4){ at.x, at.y, at.z, 1.0f });
}

// The sun view of the volume about `eye`, for a sun along `direction`: the
// volume's centre in the map's middle within a texel, its eight corners inside.
static void the_sun_map_holds_the_volume(voe_math_double3 eye,
					 voe_math_float3 direction)
{
	voe_3d_bounce_grid grid = voe_3d_bounce_grid_fit(eye);
	voe_render_view light = voe_3d_bounce_grid_sun(grid, eye, direction);
	const float sides[3] = {
		(float)VOE_RENDER_BOUNCE_PROBES_XZ * VOE_RENDER_BOUNCE_SPACING,
		(float)VOE_RENDER_BOUNCE_PROBES_Y * VOE_RENDER_BOUNCE_SPACING,
		(float)VOE_RENDER_BOUNCE_PROBES_XZ * VOE_RENDER_BOUNCE_SPACING
	};
	float texel = 2.0f / (float)VOE_RENDER_BOUNCE_SHADOW_TEXELS;
	voe_math_float4 middle = in_the_map(
		light, (voe_math_float3){ grid.corner.x + 0.5f * sides[0],
					  grid.corner.y + 0.5f * sides[1],
					  grid.corner.z + 0.5f * sides[2] });

	printf("middle %g %g texel %g\n", middle.x, middle.y, texel);
	VOE_TEST_CHECK(fabsf(middle.x) <= texel && fabsf(middle.y) <= texel);
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

int main(void)
{
	the_eye_stands_in_its_cell((voe_math_double3){ 0.0, 0.0, 0.0 });
	the_eye_stands_in_its_cell((voe_math_double3){ 0.7, 1.5, -3.2 });
	the_eye_stands_in_its_cell((voe_math_double3){ -5.3, -12.9, -0.1 });
	the_eye_stands_in_its_cell((voe_math_double3){ 10000.0, 3.0, -2500.0 });
	the_eye_stands_in_its_cell((voe_math_double3){ 10001.3, 10.9, -2501.7 });
	two_metres_along_x_is_one_cell();
	a_move_inside_the_cell_keeps_it();
	the_sun_map_holds_the_volume((voe_math_double3){ 0.7, 1.5, -3.2 },
				     (voe_math_float3){ -0.70710678f, -0.70710678f,
							0.0f });
	the_sun_map_holds_the_volume((voe_math_double3){ 10001.3, 10.9, -2501.7 },
				     (voe_math_float3){ 0.0f, -1.0f, 0.0f });

	return voe_test_result();
}
