// The bounce grid fitted to a view: that its centre stands 32 m ahead of the
// eye within a cell, that its corner is its whole cell about the eye 10 km out
// too, that 2 m along x moves it one cell, that every grid corner lands inside
// the light view's box, and that a 1 cm move keeps the world origin on a whole
// 8-texel block of the bounce map. Needs no graphics card.
#include <3d/bounce_grid.h>
#include <3d/projection.h>
#include <math/double3.h>
#include <math/float3.h>
#include <math/float4.h>
#include <math/float4x4.h>
#include <math/quat.h>
#include <scene/camera_component.h>
#include <scene/transform_component.h>

#include <testing/test.h>

#include <math.h>

#define FOV_Y 1.0471976f
#define ASPECT 1.7777778f
#define SIDE ((float)VOE_RENDER_BOUNCE_PROBES * VOE_RENDER_BOUNCE_SPACING)

static voe_math_float3 a_sun(void)
{
	return voe_math_float3_normalize((voe_math_float3){ 0.3f, -1.0f, 0.2f });
}

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
	return voe_3d_bounce_grid_fit(*view, at, a_sun());
}

static voe_math_float4 light_clip(voe_3d_bounce_grid grid, voe_math_float3 point)
{
	return voe_math_float4x4_mul_float4(
		voe_math_float4x4_mul(grid.light.projection, grid.light.view),
		(voe_math_float4){ point.x, point.y, point.z, 1.0f });
}

static void the_centre_stands_ahead_of_the_eye(voe_math_double3 at, float turn)
{
	voe_render_view view;
	voe_3d_bounce_grid grid = fit_at(at, turn, &view);
	float forward[3] = { -view.view.m[2][0], -view.view.m[2][1],
			     -view.view.m[2][2] };
	float corner[3] = { grid.corner.x, grid.corner.y, grid.corner.z };

	for (int axis = 0; axis < 3; axis++) {
		float centre = corner[axis] + SIDE * 0.5f;

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

static void every_grid_corner_is_inside_the_light_box(voe_math_double3 at,
						       float turn)
{
	voe_render_view view;
	voe_3d_bounce_grid grid = fit_at(at, turn, &view);

	for (int i = 0; i < 8; i++) {
		voe_math_float3 point = voe_math_float3_add(grid.corner, (voe_math_float3){
			(i & 1) ? SIDE : 0.0f, (i & 2) ? SIDE : 0.0f,
			(i & 4) ? SIDE : 0.0f });
		voe_math_float4 clip = light_clip(grid, point);

		VOE_TEST_CHECK(fabsf(clip.x) <= 1.0f);
		VOE_TEST_CHECK(fabsf(clip.y) <= 1.0f);
		VOE_TEST_CHECK(clip.z >= 0.0f && clip.z <= 1.0f);
		VOE_TEST_CHECK_FLOAT(clip.w, 1.0f, 1e-6f);
	}
}

// The world origin's place on the map, in blocks of eight texels.
static float origin_in_blocks(voe_3d_bounce_grid grid, voe_math_double3 at,
			      int axis)
{
	voe_math_float4 clip = light_clip(
		grid, voe_math_double3_to_float3(voe_math_double3_sub(
			      (voe_math_double3){ 0.0, 0.0, 0.0 }, at)));
	float value = axis == 0 ? clip.x : clip.y;

	return (value * 0.5f + 0.5f) * (float)VOE_RENDER_BOUNCE_TEXELS / 8.0f;
}

static void a_centimetre_keeps_the_origin_on_its_block(void)
{
	voe_math_double3 at = { 3.3, 1.2, -5.7 };
	voe_math_double3 moved = { 3.31, 1.2, -5.7 };
	voe_render_view view;
	voe_3d_bounce_grid before = fit_at(at, 0.34906585f, &view);
	voe_3d_bounce_grid after = fit_at(moved, 0.34906585f, &view);

	for (int axis = 0; axis < 2; axis++) {
		float a = origin_in_blocks(before, at, axis);
		float b = origin_in_blocks(after, moved, axis);

		VOE_TEST_CHECK(fabsf(a - roundf(a)) < 1e-3f);
		VOE_TEST_CHECK_FLOAT(b, a, 1e-3f);
	}
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
	every_grid_corner_is_inside_the_light_box(origin, 0.0f);
	every_grid_corner_is_inside_the_light_box(far_out, 0.34906585f);
	a_centimetre_keeps_the_origin_on_its_block();

	return voe_test_result();
}
