// The probe volume fitted to the eye: that the eye's cell is the grid's lowest
// cell plus (12, VOE_3D_BOUNCE_BELOW, 12) and its corner that cell's lowest
// corner about the eye, near the origin, at negative coordinates and 10 km out;
// that 2 m along x moves it one cell; and that a move inside the eye's cell
// keeps it. Needs no graphics card.
#include <3d/bounce_grid.h>
#include <math/double3.h>

#include <testing/test.h>

#include <math.h>

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

int main(void)
{
	the_eye_stands_in_its_cell((voe_math_double3){ 0.0, 0.0, 0.0 });
	the_eye_stands_in_its_cell((voe_math_double3){ 0.7, 1.5, -3.2 });
	the_eye_stands_in_its_cell((voe_math_double3){ -5.3, -12.9, -0.1 });
	the_eye_stands_in_its_cell((voe_math_double3){ 10000.0, 3.0, -2500.0 });
	the_eye_stands_in_its_cell((voe_math_double3){ 10001.3, 10.9, -2501.7 });
	two_metres_along_x_is_one_cell();
	a_move_inside_the_cell_keeps_it();

	return voe_test_result();
}
