// Snap's arithmetic: a position's dragged world axes onto the grid, a swept
// angle wrapped and rounded, and a scale's components rounded and floored at
// one step (3d/gizmo_snap.h).
//
// NONE OF IT NEEDS A GRAPHICS CARD: every claim is a value and a step in and
// the rounded value out.
#include <3d/gizmo.h>
#include <3d/gizmo_snap.h>

#include <math/double3.h>
#include <math/float3.h>

#include <testing/test.h>

#define PI 3.14159265358979323846

static float degrees(double angle)
{
	return (float)(angle * PI / 180.0);
}

// 0.37 on X at 0.5 lands 0.5 and Y and Z keep their odd values; XY rounds x
// and y and keeps z; a 1.26 on Y at 1 is 1.
static void a_position_rounds_the_axes_dragged(void)
{
	const voe_math_double3 odd = { 0.37, 1.26, -2.71 };

	voe_math_double3 on_x =
		voe_3d_gizmo_snap_position(odd, VOE_3D_GIZMO_X, 0.5);
	VOE_TEST_CHECK_FLOAT(on_x.x, 0.5, 1e-12);
	VOE_TEST_CHECK_FLOAT(on_x.y, 1.26, 1e-12);
	VOE_TEST_CHECK_FLOAT(on_x.z, -2.71, 1e-12);

	voe_math_double3 on_xy =
		voe_3d_gizmo_snap_position(odd, VOE_3D_GIZMO_XY, 0.5);
	VOE_TEST_CHECK_FLOAT(on_xy.x, 0.5, 1e-12);
	VOE_TEST_CHECK_FLOAT(on_xy.y, 1.5, 1e-12);
	VOE_TEST_CHECK_FLOAT(on_xy.z, -2.71, 1e-12);

	voe_math_double3 on_y =
		voe_3d_gizmo_snap_position(odd, VOE_3D_GIZMO_Y, 1.0);
	VOE_TEST_CHECK_FLOAT(on_y.x, 0.37, 1e-12);
	VOE_TEST_CHECK_FLOAT(on_y.y, 1.0, 1e-12);
	VOE_TEST_CHECK_FLOAT(on_y.z, -2.71, 1e-12);
}

// 100 km out a millimetre step is still a millimetre: the double keeps it.
static void far_out_the_step_stays_exact(void)
{
	const voe_math_double3 far = { 100000.0004, 2.0, 100000.0016 };

	voe_math_double3 on_zx =
		voe_3d_gizmo_snap_position(far, VOE_3D_GIZMO_ZX, 0.001);
	VOE_TEST_CHECK_FLOAT(on_zx.x, 100000.0, 1e-9);
	VOE_TEST_CHECK_FLOAT(on_zx.y, 2.0, 1e-12);
	VOE_TEST_CHECK_FLOAT(on_zx.z, 100000.002, 1e-9);

	const voe_math_double3 half = { 100000.37, 0.0, 0.0 };
	VOE_TEST_CHECK_FLOAT(
		voe_3d_gizmo_snap_position(half, VOE_3D_GIZMO_X, 0.5).x,
		100000.5, 1e-9);
}

// 22° at 15° steps is 15°; a sweep of 200°, just past π, wraps to −160° and
// snaps to −165°; −180° is the same turn as 180° and snaps there.
static void an_angle_wraps_then_rounds(void)
{
	const float step = degrees(15.0);

	VOE_TEST_CHECK_FLOAT(voe_3d_gizmo_snap_angle(degrees(22.0), step),
			     degrees(15.0), 1e-5);
	VOE_TEST_CHECK_FLOAT(voe_3d_gizmo_snap_angle(degrees(-22.0), step),
			     degrees(-15.0), 1e-5);
	VOE_TEST_CHECK_FLOAT(voe_3d_gizmo_snap_angle(degrees(200.0), step),
			     degrees(-165.0), 1e-5);
	VOE_TEST_CHECK_FLOAT(voe_3d_gizmo_snap_angle((float)-PI, step),
			     (float)PI, 1e-5);
}

// 0.04 on X scales to one step, not nought; 1.26 on X to 1.3 with Y and Z
// kept; CENTRE rounds all three; a mirrored axis stays mirrored.
static void a_scale_rounds_and_never_reaches_nought(void)
{
	const voe_math_float3 small = { 0.04f, 2.0f, 3.0f };
	VOE_TEST_CHECK_FLOAT(
		voe_3d_gizmo_snap_scale(small, VOE_3D_GIZMO_X, 0.1f).x, 0.1,
		1e-6);

	const voe_math_float3 odd = { 1.26f, 0.37f, 2.04f };

	voe_math_float3 on_x =
		voe_3d_gizmo_snap_scale(odd, VOE_3D_GIZMO_X, 0.1f);
	VOE_TEST_CHECK_FLOAT(on_x.x, 1.3, 1e-6);
	VOE_TEST_CHECK_FLOAT(on_x.y, 0.37, 1e-6);
	VOE_TEST_CHECK_FLOAT(on_x.z, 2.04, 1e-6);

	voe_math_float3 all =
		voe_3d_gizmo_snap_scale(odd, VOE_3D_GIZMO_CENTRE, 0.1f);
	VOE_TEST_CHECK_FLOAT(all.x, 1.3, 1e-6);
	VOE_TEST_CHECK_FLOAT(all.y, 0.4, 1e-6);
	VOE_TEST_CHECK_FLOAT(all.z, 2.0, 1e-6);

	const voe_math_float3 mirrored = { -1.26f, -0.04f, 1.0f };
	voe_math_float3 kept =
		voe_3d_gizmo_snap_scale(mirrored, VOE_3D_GIZMO_CENTRE, 0.1f);
	VOE_TEST_CHECK_FLOAT(kept.x, -1.3, 1e-6);
	VOE_TEST_CHECK_FLOAT(kept.y, -0.1, 1e-6);
}

int main(void)
{
	a_position_rounds_the_axes_dragged();
	far_out_the_step_stays_exact();
	an_angle_wraps_then_rounds();
	a_scale_rounds_and_never_reaches_nought();

	return voe_test_result();
}
