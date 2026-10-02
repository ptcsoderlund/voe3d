// The probe volume fitted to a view: that its centre stands 24 m ahead of the
// eye within a cell, that its corner is its whole cell about the eye 10 km out
// too, and that 2 m along x moves it one cell. Needs no graphics card.
#include <3d/bounce_grid.h>
#include <3d/projection.h>
#include <math/double3.h>
#include <math/float3.h>
#include <math/quat.h>
#include <scene/camera_component.h>
#include <scene/transform_component.h>

#include <testing/test.h>

#include <math.h>

#define FOV_Y 1.0471976f
#define ASPECT 1.7777778f

// The volume's side along each axis, in metres.
static const float SIDES[3] = {
	(float)VOE_RENDER_BOUNCE_PROBES_XZ * VOE_RENDER_BOUNCE_SPACING,
	(float)VOE_RENDER_BOUNCE_PROBES_Y * VOE_RENDER_BOUNCE_SPACING,
	(float)VOE_RENDER_BOUNCE_PROBES_XZ * VOE_RENDER_BOUNCE_SPACING,
};

// A camera at `at`, turned `turn` radians about Y, and its grid.
static voe_3d_bounce_grid fit_at(voe_math_double3 at, float turn,
				 voe_render_view *view)
{
	voe_scene_camera lens = { .fov_y = FOV_Y, .near_plane = 0.1f,
				  .far_plane = 1000.0f };
	voe_scene_transform pose = {
		.position = at,
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, turn),
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	VOE_TEST_CHECK(voe_3d_view(pose, lens, ASPECT, view));
	return voe_3d_bounce_grid_fit(*view, at);
}

static void the_centre_stands_ahead_of_the_eye(voe_math_double3 at, float turn)
{
	voe_render_view view;
	voe_3d_bounce_grid grid = fit_at(at, turn, &view);
	float forward[3] = { -view.view.m[2][0], -view.view.m[2][1],
			     -view.view.m[2][2] };
	float corner[3] = { grid.corner.x, grid.corner.y, grid.corner.z };

	for (int axis = 0; axis < 3; axis++) {
		float centre = corner[axis] + SIDES[axis] * 0.5f;

		VOE_TEST_CHECK(fabsf(centre - forward[axis] * VOE_3D_BOUNCE_AHEAD) <=
			       VOE_RENDER_BOUNCE_SPACING);
	}
}

static void the_corner_is_the_cell_about_the_eye(voe_math_double3 at)
{
	voe_render_view view;
	voe_3d_bounce_grid grid = fit_at(at, 0.34906585f, &view);
	double eye[3] = { at.x, at.y, at.z };
	float corner[3] = { grid.corner.x, grid.corner.y, grid.corner.z };

	for (int axis = 0; axis < 3; axis++) {
		double world = eye[axis] + (double)corner[axis];
		double cell = (double)grid.cell[axis] * VOE_RENDER_BOUNCE_SPACING;

		VOE_TEST_CHECK(fabs(world - cell) < 1e-3);
	}
}

static void two_metres_along_x_is_one_cell(void)
{
	voe_render_view view;
	voe_3d_bounce_grid before =
		fit_at((voe_math_double3){ 0.7, 1.5, -3.2 }, 0.0f, &view);
	voe_3d_bounce_grid after =
		fit_at((voe_math_double3){ 2.7, 1.5, -3.2 }, 0.0f, &view);

	VOE_TEST_CHECK_FLOAT(view.view.m[2][2], 1.0f, 1e-6f);
	VOE_TEST_CHECK_INT(after.cell[0], before.cell[0] + 1);
	VOE_TEST_CHECK_INT(after.cell[1], before.cell[1]);
	VOE_TEST_CHECK_INT(after.cell[2], before.cell[2]);
}

int main(void)
{
	voe_math_double3 origin = { 0.0, 0.0, 0.0 };
	voe_math_double3 far_out = { 10000.0, 3.0, -2500.0 };

	the_centre_stands_ahead_of_the_eye(origin, 0.0f);
	the_centre_stands_ahead_of_the_eye(far_out, 0.34906585f);
	the_corner_is_the_cell_about_the_eye(origin);
	the_corner_is_the_cell_about_the_eye(far_out);
	two_metres_along_x_is_one_cell();

	return voe_test_result();
}
