// Which cube faces a caster's sphere reaches (ADR-0325 point 2), with no
// graphics card: the sphere and the mask are plain arithmetic.
//
// A SPHERE STRAIGHT ALONG +X REACHES BIT 0 ONLY. ONE ON THE +X/+Y DIAGONAL
// BITS 0 AND 2. ONE ABOUT THE LIGHT ALL SIX. ONE BEYOND RANGE NONE.
//
// A CUBE'S 24 VERTICES GIVE CENTRE 0 AND RADIUS √3 · HALF SIDE. A MATRIX
// SCALING BY (1, 3, 1) AND MOVING BY (5, 0, 0) GIVES CENTRE (5, 0, 0) AND THE
// RADIUS TIMES 3.
#include "../src/point_shadow_faces.h"

#include <testing/test.h>

#include <math.h>

#define HALF 2.0f

static const voe_math_float3 light = { 1.0f, 2.0f, 3.0f };

static bool near(float a, float b)
{
	return fabsf(a - b) < 1e-4f;
}

static uint32_t faces_at(float x, float y, float z, float radius)
{
	const voe_math_float4 sphere = { light.x + x, light.y + y, light.z + z,
					 radius };

	return voe_render_point_shadow_faces(sphere, light, 10.0f);
}

static void along_x_reaches_bit_0_only(void)
{
	VOE_TEST_CHECK_INT(faces_at(5.0f, 0.0f, 0.0f, 1.0f), 1u);
}

static void on_the_diagonal_reaches_bits_0_and_2(void)
{
	VOE_TEST_CHECK_INT(faces_at(5.0f, 5.0f, 0.0f, 1.0f), 0x5u);
}

static void about_the_light_reaches_all_six(void)
{
	VOE_TEST_CHECK_INT(faces_at(0.5f, -0.3f, 0.2f, 1.0f),
			   VOE_RENDER_POINT_SHADOW_ALL_FACES);
}

static void beyond_range_reaches_none(void)
{
	VOE_TEST_CHECK_INT(faces_at(20.0f, 0.0f, 0.0f, 1.0f), 0u);
}

// Four corners per face, six faces: every corner of the cube four times over
// three, as a mesh with its own normals per face has them.
static voe_math_float4 cube_sphere(void)
{
	voe_render_vertex v[24] = { 0 };

	for (uint32_t i = 0; i < 24; i++)
		v[i].position = (voe_math_float3){ (i & 1) ? HALF : -HALF,
						   (i & 2) ? HALF : -HALF,
						   (i & 4) ? HALF : -HALF };
	return voe_render_point_shadow_sphere(v, 24);
}

static void a_cube_gives_centre_0_and_root_3_half_side(void)
{
	const voe_math_float4 s = cube_sphere();

	VOE_TEST_CHECK(near(s.x, 0.0f) && near(s.y, 0.0f) && near(s.z, 0.0f));
	VOE_TEST_CHECK(near(s.w, sqrtf(3.0f) * HALF));
}

static void a_matrix_moves_the_centre_and_scales_the_radius(void)
{
	const voe_math_float4x4 world = voe_math_float4x4_mul(
		voe_math_float4x4_from_translation((voe_math_float3){ 5, 0, 0 }),
		voe_math_float4x4_from_scale((voe_math_float3){ 1, 3, 1 }));
	const voe_math_float4 s = cube_sphere();
	const voe_math_float4 moved =
		voe_render_point_shadow_sphere_moved(s, world);

	VOE_TEST_CHECK(near(moved.x, 5.0f) && near(moved.y, 0.0f) &&
		       near(moved.z, 0.0f));
	VOE_TEST_CHECK(near(moved.w, s.w * 3.0f));
}

int main(void)
{
	along_x_reaches_bit_0_only();
	on_the_diagonal_reaches_bits_0_and_2();
	about_the_light_reaches_all_six();
	beyond_range_reaches_none();
	a_cube_gives_centre_0_and_root_3_half_side();
	a_matrix_moves_the_centre_and_scales_the_radius();
	return voe_test_result();
}
