// THE CLAIM: a normal transformed by the normal matrix stays perpendicular to
// the surface it belongs to, and one transformed by the world matrix does not.
//
// THIS IS THE ONE TEST THAT WOULD PASS WITH THE WRONG MATRIX IF IT WERE WRITTEN
// CARELESSLY. For a rotation, or a rotation with a uniform scale, the inverse
// transpose *is* the world matrix's rotation — so a test that only ever scaled
// something evenly would pass while the engine multiplied by the wrong thing.
// The first case below therefore uses a scale with three different numbers in
// it, and it checks the mistake as well as the fix: the world matrix is applied
// to the same normal and the answer is required to be visibly wrong. The three
// after it are the cases where the two matrices agree, and each of them is a
// different way of getting the arithmetic wrong without this noticing.
//
// PERPENDICULARITY IS THE PROPERTY AND NOT THE ELEMENTS. What a normal matrix is
// *for* is keeping n·t at zero for every tangent t of the surface, so that is
// what is checked — against the two tangents the normal was built from with a
// cross product. Comparing sixteen elements against a hand-computed matrix would
// test the arithmetic; this tests the reason for it.
//
// THE SURFACE: a triangle at the origin whose two edges are (1, 1, 0) and
// (0, 0, 1), so its normal is (1, -1, 0). Both edges have to be slanted with
// respect to the scale, or the scale leaves them alone and there is nothing to
// notice: an edge along an axis is stretched without being turned.
#include <3d/normal_matrix.h>
#include <math/float3.h>
#include <math/float4x4.h>
#include <math/quat.h>
#include <scene/transform_component.h>

#include <testing/test.h>

#define QUARTER_TURN 1.5707963f

// An inverse is a determinant's worth of division and the values here are small
// integers and halves, so the error is a few multiplies wide.
#define TOLERANCE 1e-5f

// The two edges of the surface, and the normal the cross product of them is.
// The point every matrix here is taken about: these transforms stand at the
// world's origin, so about it is where they are.
static const voe_math_double3 ORIGIN = { 0.0, 0.0, 0.0 };

static const voe_math_float3 EDGE_A = { 1.0f, 1.0f, 0.0f };
static const voe_math_float3 EDGE_B = { 0.0f, 0.0f, 1.0f };
static const voe_math_float3 NORMAL = { 1.0f, -1.0f, 0.0f };

// A transform with three different scales, a rotation that is not about one of
// the scaled axes, and a translation — all three, because each of them is a way
// for an inverse transpose to be got wrong and only the scale shows up on its
// own.
static voe_scene_transform squashed(void)
{
	voe_math_float3 axis = { 0.0f, 0.0f, 1.0f };
	voe_scene_transform transform = {
		.position = { 1.0f, 2.0f, 3.0f },
		.rotation = voe_math_quat_from_axis_angle(axis, QUARTER_TURN),
		.scale = { 2.0f, 0.5f, 1.5f },
	};

	return transform;
}

// The edges and the normal are directions, so the translation is not in any of
// this: voe_math_float4x4_transform_dir is the affine transform without it.
static void the_normal_stays_perpendicular_to_the_surface(void)
{
	voe_math_float4x4 world = voe_scene_transform_matrix(squashed(), ORIGIN);
	voe_math_float4x4 normals = voe_3d_normal_matrix(world);
	voe_math_float3 edge_a = voe_math_float4x4_transform_dir(world, EDGE_A);
	voe_math_float3 edge_b = voe_math_float4x4_transform_dir(world, EDGE_B);
	voe_math_float3 right = voe_math_float4x4_transform_dir(normals,
								NORMAL);
	voe_math_float3 wrong = voe_math_float4x4_transform_dir(world, NORMAL);

	// The claim, twice: once against each edge of the surface.
	VOE_TEST_CHECK_FLOAT(voe_math_float3_dot(right, edge_a), 0.0f,
			     TOLERANCE);
	VOE_TEST_CHECK_FLOAT(voe_math_float3_dot(right, edge_b), 0.0f,
			     TOLERANCE);

	// And the mistake, so that the check above is known to be measuring
	// something. Transforming a normal by the world matrix leaves it a long
	// way from perpendicular to this surface, which on screen is a flat face
	// shaded as if it were angled.
	VOE_TEST_CHECK(voe_math_float3_dot(wrong, edge_a) > 1.0f);

	// It points out of the same side of the surface it did before, which is
	// the other way to get this wrong: a normal matrix that came out negated
	// keeps every dot product at zero and lights the inside of everything.
	VOE_TEST_CHECK(voe_math_float3_dot(
			       voe_math_float3_normalize(right),
			       voe_math_float3_normalize(voe_math_float3_cross(
				       edge_a, edge_b))) > 0.9f);
}

// With no scale in it the normal matrix is the world matrix's rotation, exactly,
// and this is what says the transpose and the inverse were both applied rather
// than one of them twice — a lone transpose would come out as the rotation the
// other way round and pass every perpendicularity check above.
static void a_rotation_is_its_own_normal_matrix(void)
{
	voe_math_float3 axis = { 0.3f, 0.9f, 0.2f };
	voe_scene_transform transform = {
		.position = { 4.0f, 5.0f, 6.0f },
		.rotation = voe_math_quat_from_axis_angle(
			voe_math_float3_normalize(axis), 0.7f),
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_math_float4x4 world = voe_scene_transform_matrix(transform, ORIGIN);
	voe_math_float4x4 normals = voe_3d_normal_matrix(world);

	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 3; column++)
			VOE_TEST_CHECK_FLOAT(normals.m[row][column],
					     world.m[row][column], TOLERANCE);
	}
}

// A uniform scale changes the length and not the direction, so the normal comes
// back pointing the same way and the shader's normalize takes care of the rest.
// This is the case that hides a missing normal matrix, which is why it is here
// as a claim rather than as an assumption.
static void a_uniform_scale_only_changes_the_length(void)
{
	voe_scene_transform transform = {
		.position = { 0.0f, 0.0f, 0.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 3.0f, 3.0f, 3.0f },
	};
	voe_math_float4x4 world = voe_scene_transform_matrix(transform, ORIGIN);
	voe_math_float4x4 normals = voe_3d_normal_matrix(world);
	voe_math_float3 turned = voe_math_float3_normalize(
		voe_math_float4x4_transform_dir(normals, NORMAL));
	voe_math_float3 expected = voe_math_float3_normalize(NORMAL);

	VOE_TEST_CHECK_FLOAT(turned.x, expected.x, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(turned.y, expected.y, TOLERANCE);
	VOE_TEST_CHECK_FLOAT(turned.z, expected.z, TOLERANCE);
}

// A scale of nothing on one axis has no inverse, and `math`'s inverse asserts
// rather than handing back infinities — so this is the case that would take the
// program down mid-frame if the determinant were not checked. A file may
// legitimately hold such a transform.
static void a_flattened_object_gets_the_identity(void)
{
	voe_scene_transform transform = {
		.position = { 1.0f, 1.0f, 1.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 0.0f, 1.0f },
	};
	voe_math_float4x4 world = voe_scene_transform_matrix(transform, ORIGIN);
	voe_math_float4x4 normals = voe_3d_normal_matrix(world);
	voe_math_float4x4 identity = voe_math_float4x4_identity();

	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++)
			VOE_TEST_CHECK_FLOAT(normals.m[row][column],
					     identity.m[row][column], 0.0f);
	}
}

int main(void)
{
	the_normal_stays_perpendicular_to_the_surface();
	a_rotation_is_its_own_normal_matrix();
	a_uniform_scale_only_changes_the_length();
	a_flattened_object_gets_the_identity();

	return voe_test_result();
}
