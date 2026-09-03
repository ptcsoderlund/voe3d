// THE ROTATION TYPE, AND THE ONE THING THAT MATTERS ABOUT IT IS WHICH WAY IT
// TURNS. A quaternion that rotates by the wrong sign, or by twice the angle
// asked for, produces a picture that moves and looks plausible — so the checks
// below are all about direction and amount, and every one of them names a
// concrete axis and a concrete vector rather than an algebraic identity.
//
// RIGHT-HANDED IS THE CLAIM. CLAUDE.md fixes the engine as right-handed with +Y
// up and -Z forward, so a positive angle about +Y takes +Z towards +X. Get that
// backwards and the orbiting camera in render goes the other way round, which
// nothing else in the tree would notice.
//
// THE ANGLE IS NOT DOUBLED, WHICH IS THE OTHER CLASSIC. _from_axis_angle halves
// the angle before the sine and the matrix is quadratic in the components, so
// the two cancel. Forget the half and every rotation is twice as far as asked —
// which at 90 degrees looks like a 180, and at 180 looks like nothing at all.
// The 90-degree cases below are what pin it: a doubled angle sends +Z to -Z
// rather than to +X.
//
// IT IS TESTED THROUGH THE MATRIX BECAUSE THE MATRIX IS THE WHOLE SURFACE.
// voe_math_quat_from_axis_angle and voe_math_float4x4_from_quat are the only two
// functions there are, and one of them consumes the other's output; a test that
// read the components directly would be asserting the half-angle formula back at
// itself. Transforming a known vector is the same statement in the form the
// engine actually uses.
#include <math/float3.h>
#include <math/float4x4.h>
#include <math/quat.h>

#include <testing/test.h>

// Radians. Written out rather than taken from a header, because M_PI is not
// standard C and the engine has no constant of its own until something needs
// one for more than a test.
#define QUARTER_TURN 1.5707963f
#define HALF_TURN 3.1415927f

// Two roundings through sinf and cosf and then nine multiplications, so a few
// parts in a million is the honest bound. Tight enough that a sign error or a
// doubled angle is nowhere near it.
#define TOLERANCE 1e-5f

static const voe_math_float3 X = { 1.0f, 0.0f, 0.0f };
static const voe_math_float3 Y = { 0.0f, 1.0f, 0.0f };
static const voe_math_float3 Z = { 0.0f, 0.0f, 1.0f };

static void check_vector(voe_math_float3 actual, voe_math_float3 expected)
{
	VOE_TEST_CHECK_FLOAT(actual.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(actual.z, expected.z, TOLERANCE);
}

// A direction and not a point, because a rotation has no translation to take.
// _transform_dir is the operation the caller in render uses for a normal and
// _transform_point is the one it uses for a position; on a pure rotation they
// agree, and the row of zeroes checked below is why.
static voe_math_float3 turn(voe_math_float3 axis, float angle,
			    voe_math_float3 v)
{
	voe_math_quat q = voe_math_quat_from_axis_angle(axis, angle);

	return voe_math_float4x4_transform_dir(voe_math_float4x4_from_quat(q), v);
}

// ZERO IS THE IDENTITY, AND IT IS THE STILL CUBE IN render. One of the two cubes
// card 015 draws is deliberately not moving, so a rotation of nothing has to be
// exactly a rotation of nothing rather than nearly one.
static void check_zero_is_identity(void)
{
	voe_math_quat q = voe_math_quat_from_axis_angle(Y, 0.0f);
	voe_math_float4x4 m = voe_math_float4x4_from_quat(q);
	voe_math_float4x4 identity = voe_math_float4x4_identity();

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++)
			VOE_TEST_CHECK_FLOAT(m.m[row][column],
					     identity.m[row][column], 0.0f);
	}
}

// THE HANDEDNESS, ON ALL THREE AXES. One axis would leave two thirds of a
// mistyped matrix element unchecked, and the three cyclic cases are the shape
// that catches a transposed row: transposing this matrix reverses every one of
// them at once, so all three passing is what says the layout is right way round
// as well as the sign.
static void check_right_handed(void)
{
	// +Y takes +Z to +X. This is the camera orbit's direction in render.
	check_vector(turn(Y, QUARTER_TURN, Z), X);
	// +Z takes +X to +Y.
	check_vector(turn(Z, QUARTER_TURN, X), Y);
	// +X takes +Y to +Z.
	check_vector(turn(X, QUARTER_TURN, Y), Z);

	// And the axis itself never moves, whatever the angle. A matrix built
	// from the wrong three of the nine elements can still turn one vector
	// correctly; it cannot also leave the axis alone.
	check_vector(turn(Y, QUARTER_TURN, Y), Y);
	check_vector(turn(X, HALF_TURN, X), X);
}

// THE AMOUNT, WHICH IS WHERE A FORGOTTEN HALF-ANGLE SHOWS UP. A half turn about
// +Y sends +Z to -Z; a doubled angle would send it back to +Z, which is exactly
// what "no rotation at all" looks like.
static void check_amount(void)
{
	voe_math_float3 back = { 0.0f, 0.0f, -1.0f };
	voe_math_float3 diagonal;

	check_vector(turn(Y, HALF_TURN, Z), back);

	// An eighth of a turn, so that both components are the same non-trivial
	// number rather than a zero and a one. A doubled angle lands on +X here
	// and a halved one on a different pair, and neither is inside the
	// tolerance.
	diagonal = turn(Y, QUARTER_TURN * 0.5f, Z);
	VOE_TEST_CHECK_FLOAT(diagonal.x, 0.70710678f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(diagonal.y, 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(diagonal.z, 0.70710678f, TOLERANCE);
}

// A ROTATION AND NOTHING ELSE: NO SCALE, NO SHEAR, NO REFLECTION, NO
// TRANSLATION. This is the property that makes the matrix safe to multiply into
// a transform chain, and a determinant of -1 rather than +1 is the one mistake
// here that still looks like a rotation until something is culled — a reflection
// reverses winding.
static void check_is_a_rotation(void)
{
	voe_math_float3 axis = { 1.0f, 2.0f, -3.0f };
	voe_math_quat q = voe_math_quat_from_axis_angle(axis, 0.9f);
	voe_math_float4x4 m = voe_math_float4x4_from_quat(q);
	voe_math_float3 row0 = { m.m[0][0], m.m[0][1], m.m[0][2] };
	voe_math_float3 row1 = { m.m[1][0], m.m[1][1], m.m[1][2] };
	voe_math_float3 row2 = { m.m[2][0], m.m[2][1], m.m[2][2] };

	VOE_TEST_CHECK_FLOAT(voe_math_float4x4_determinant(m), 1.0f, TOLERANCE);

	VOE_TEST_CHECK_FLOAT(voe_math_float3_length(row0), 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(voe_math_float3_length(row1), 1.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(voe_math_float3_length(row2), 1.0f, TOLERANCE);

	VOE_TEST_CHECK_FLOAT(voe_math_float3_dot(row0, row1), 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(voe_math_float3_dot(row0, row2), 0.0f, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(voe_math_float3_dot(row1, row2), 0.0f, TOLERANCE);

	// The last row and the last column are the identity's. Anything else
	// here is a translation or a projective term a rotation has no business
	// having.
	VOE_TEST_CHECK_FLOAT(m.m[0][3], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[1][3], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[2][3], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[3][0], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[3][1], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[3][2], 0.0f, 0.0f);
	VOE_TEST_CHECK_FLOAT(m.m[3][3], 1.0f, 0.0f);
}

// THE AXIS NEED NOT BE UNIT, WHICH IS WHAT THE HEADER PROMISES. A caller handing
// in an axis it computed should not have to normalize it first, and a
// _from_axis_angle that skipped the normalize would produce a quaternion that
// scales as well as rotates — which the assert in _from_quat would catch, but
// only in a debug build and only after somebody ran it.
static void check_axis_need_not_be_unit(void)
{
	check_vector(turn(voe_math_float3_scale(Y, 7.5f), QUARTER_TURN, Z), X);
	check_vector(turn(voe_math_float3_scale(Y, 0.02f), QUARTER_TURN, Z), X);
}

// TWICE BY HALF IS ONCE BY THE WHOLE, WHICH IS THE CLAIM THAT MAKES THIS TYPE
// WORTH HAVING. Nothing composes quaternions yet — there is no _mul on purpose —
// so composition is checked where it does happen, on the matrices, through
// voe_math_float4x4_mul. If a later card adds _mul, this is the check it has to
// agree with.
static void check_composition(void)
{
	voe_math_float4x4 half = voe_math_float4x4_from_quat(
		voe_math_quat_from_axis_angle(Y, QUARTER_TURN * 0.5f));
	voe_math_float4x4 whole = voe_math_float4x4_from_quat(
		voe_math_quat_from_axis_angle(Y, QUARTER_TURN));
	voe_math_float4x4 twice = voe_math_float4x4_mul(half, half);

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++)
			VOE_TEST_CHECK_FLOAT(twice.m[row][column],
					     whole.m[row][column], TOLERANCE);
	}
}

int main(void)
{
	check_zero_is_identity();
	check_right_handed();
	check_amount();
	check_is_a_rotation();
	check_axis_need_not_be_unit();
	check_composition();
	return voe_test_result();
}
