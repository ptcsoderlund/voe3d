// float4x4's tests, and the two claims in include/math/float4x4.h that are
// different claims: how the sixteen floats are laid out in memory, and what
// multiplying by a vector means. Getting one right and the other wrong is the
// bug that looks like nothing, so both are asserted separately here.
//
// THIS IS THE CPU HALF ONLY. The header's other requirement — that slangc is
// invoked with -matrix-layout-row-major, so that the shader agrees about the
// layout this file pins down — cannot be checked from here. It needs a matrix
// sent to the GPU and read back, and it belongs to the first card that sends
// one. A green run of this file says nothing about that flag.
//
// The assert paths are not tested. Inverting a singular matrix aborts the
// process by design, and a test that aborts reports nothing.
#include <math/float4x4.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// Two tolerances, and which one an assertion uses is a claim about the
// operation. EXACT is for values that are representable and reached in one add
// or one multiply, where a near miss is a wrong result and not rounding. NEAR is
// for the ones that went through the determinant and a division.
#define EXACT 0.0f
#define NEAR 1e-6f

// The sixteen elements, in the order they are written on the page: row 0 first,
// which is also the order they sit in memory. A failure names the element, so
// the report says m[1][3] and not "the matrix".
static void check_matrix(voe_math_float4x4 m, const float expected[16],
			 float tolerance, const char *file, int line,
			 const char *expr)
{
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			char named[256];

			snprintf(named, sizeof(named), "%s.m[%d][%d]",
				 expr, row, col);
			voe_test_check_float(m.m[row][col],
					     expected[row * 4 + col],
					     tolerance, file, line, named);
		}
	}
}

#define CHECK_MATRIX(expr, tolerance, ...)				\
	check_matrix((expr), (const float[16]){ __VA_ARGS__ },		\
		     (tolerance), __FILE__, __LINE__, #expr)

#define CHECK3(expr, tolerance, ex, ey, ez) do {			\
	voe_math_float3 result = (expr);				\
									\
	voe_test_check_float(result.x, (ex), (tolerance), __FILE__,	\
			     __LINE__, #expr ".x == " #ex);		\
	voe_test_check_float(result.y, (ey), (tolerance), __FILE__,	\
			     __LINE__, #expr ".y == " #ey);		\
	voe_test_check_float(result.z, (ez), (tolerance), __FILE__,	\
			     __LINE__, #expr ".z == " #ez);		\
} while (0)

#define CHECK4(expr, tolerance, ex, ey, ez, ew) do {			\
	voe_math_float4 result = (expr);				\
									\
	voe_test_check_float(result.x, (ex), (tolerance), __FILE__,	\
			     __LINE__, #expr ".x == " #ex);		\
	voe_test_check_float(result.y, (ey), (tolerance), __FILE__,	\
			     __LINE__, #expr ".y == " #ey);		\
	voe_test_check_float(result.z, (ez), (tolerance), __FILE__,	\
			     __LINE__, #expr ".z == " #ez);		\
	voe_test_check_float(result.w, (ew), (tolerance), __FILE__,	\
			     __LINE__, #expr ".w == " #ew);		\
} while (0)

// Every element distinct, so that any transposition or reordering shows up. It
// is singular, which is why nothing here inverts it.
static voe_math_float4x4 counting(void)
{
	voe_math_float4x4 m = {};

	for (int row = 0; row < 4; row++)
		for (int col = 0; col < 4; col++)
			m.m[row][col] = (float)(row * 4 + col + 1);
	return m;
}

// The layout claim, asserted as what it is: a statement about bytes. Element
// (row, col) is row * 4 + col floats into the object, with nothing in between —
// which is the whole reason an upload to the GPU is a straight copy and not a
// transpose.
static void storage_is_row_major_and_has_no_padding(void)
{
	voe_math_float4x4 m = counting();
	float flat[16];

	VOE_TEST_CHECK_INT((long long)sizeof(voe_math_float4x4),
			   16 * (long long)sizeof(float));

	memcpy(flat, &m, sizeof(flat));

	// The expected value names the index it belongs to, so a failure is
	// unambiguous without a message per element.
	for (int i = 0; i < 16; i++)
		VOE_TEST_CHECK_INT((long long)flat[i], i + 1);
}

static void identity_and_the_two_constructors(void)
{
	CHECK_MATRIX(voe_math_float4x4_identity(), EXACT,
		     1, 0, 0, 0,
		     0, 1, 0, 0,
		     0, 0, 1, 0,
		     0, 0, 0, 1);

	CHECK_MATRIX(voe_math_float4x4_from_scale(
			     (voe_math_float3){ 2.0f, 3.0f, 4.0f }), EXACT,
		     2, 0, 0, 0,
		     0, 3, 0, 0,
		     0, 0, 4, 0,
		     0, 0, 0, 1);
}

// The semantic claim, and the half of it that is easiest to get backwards:
// vectors are columns, so translation is the last column — m[0][3], m[1][3],
// m[2][3] — and not the last row. The flat offsets are checked too, because the
// last column and the last row are the same three numbers to anyone reading the
// values and different bytes to the GPU.
static void translation_is_the_last_column(void)
{
	voe_math_float4x4 t = voe_math_float4x4_from_translation(
		(voe_math_float3){ 7.0f, 8.0f, 9.0f });
	float flat[16];

	CHECK_MATRIX(voe_math_float4x4_from_translation(
			     (voe_math_float3){ 7.0f, 8.0f, 9.0f }), EXACT,
		     1, 0, 0, 7,
		     0, 1, 0, 8,
		     0, 0, 1, 9,
		     0, 0, 0, 1);

	memcpy(flat, &t, sizeof(flat));
	VOE_TEST_CHECK_FLOAT(flat[3], 7.0f, EXACT);
	VOE_TEST_CHECK_FLOAT(flat[7], 8.0f, EXACT);
	VOE_TEST_CHECK_FLOAT(flat[11], 9.0f, EXACT);

	// The last row is untouched. Were the convention the other way round,
	// these three would hold the translation instead.
	VOE_TEST_CHECK_FLOAT(flat[12], 0.0f, EXACT);
	VOE_TEST_CHECK_FLOAT(flat[13], 0.0f, EXACT);
	VOE_TEST_CHECK_FLOAT(flat[14], 0.0f, EXACT);
}

// _mul on matrices is the matrix product. The operands are chosen so that a
// component-wise implementation — which is what _mul means on a vector — gives a
// different answer: it would leave the last column at zero.
static void mul_is_the_matrix_product(void)
{
	voe_math_float4x4 s = voe_math_float4x4_from_scale(
		(voe_math_float3){ 2.0f, 3.0f, 4.0f });
	voe_math_float4x4 t = voe_math_float4x4_from_translation(
		(voe_math_float3){ 1.0f, 1.0f, 1.0f });

	CHECK_MATRIX(voe_math_float4x4_mul(s, t), EXACT,
		     2, 0, 0, 2,
		     0, 3, 0, 3,
		     0, 0, 4, 4,
		     0, 0, 0, 1);

	// Identity on either side changes nothing, which no component-wise
	// product manages.
	CHECK_MATRIX(voe_math_float4x4_mul(voe_math_float4x4_identity(), s),
		     EXACT,
		     2, 0, 0, 0,
		     0, 3, 0, 0,
		     0, 0, 4, 0,
		     0, 0, 0, 1);
}

// Composition reads right to left: mul(a, b) applies b first. The two orders
// give different points, and that difference is the assertion — a
// multiplication that composed the other way would still look plausible on its
// own.
static void composition_applies_the_right_hand_side_first(void)
{
	voe_math_float4x4 t = voe_math_float4x4_from_translation(
		(voe_math_float3){ 10.0f, 0.0f, 0.0f });
	voe_math_float4x4 s = voe_math_float4x4_from_scale(
		(voe_math_float3){ 2.0f, 2.0f, 2.0f });
	voe_math_float3 p = { 1.0f, 1.0f, 1.0f };

	// Scale first, then translate: (2,2,2) moved by 10.
	CHECK3(voe_math_float4x4_transform_point(voe_math_float4x4_mul(t, s), p),
	       EXACT, 12.0f, 2.0f, 2.0f);

	// Translate first, then scale: (11,1,1) doubled.
	CHECK3(voe_math_float4x4_transform_point(voe_math_float4x4_mul(s, t), p),
	       EXACT, 22.0f, 2.0f, 2.0f);
}

// M*v with v a column: component row of the result is row of the matrix dotted
// with the whole vector. The matrix has every element distinct, so multiplying
// by the transpose — the mistake this exists to catch — gives 90 where this
// expects 30.
static void mul_float4_treats_the_vector_as_a_column(void)
{
	voe_math_float4x4 m = counting();
	voe_math_float4 v = { 1.0f, 2.0f, 3.0f, 4.0f };

	CHECK4(voe_math_float4x4_mul_float4(m, v), EXACT,
	       30.0f, 70.0f, 110.0f, 150.0f);
}

// Affine, and the difference between the two is the translation: a point takes
// it, a direction does not.
static void transform_point_takes_translation_and_dir_does_not(void)
{
	voe_math_float4x4 m = voe_math_float4x4_mul(
		voe_math_float4x4_from_translation(
			(voe_math_float3){ 10.0f, 20.0f, 30.0f }),
		voe_math_float4x4_from_scale(
			(voe_math_float3){ 2.0f, 2.0f, 2.0f }));
	voe_math_float3 v = { 1.0f, 2.0f, 3.0f };

	CHECK3(voe_math_float4x4_transform_point(m, v), EXACT,
	       12.0f, 24.0f, 36.0f);
	CHECK3(voe_math_float4x4_transform_dir(m, v), EXACT,
	       2.0f, 4.0f, 6.0f);

	// The same two, spelled as float4s: w == 1 is a point, w == 0 is a
	// direction, and that is where the difference above comes from.
	CHECK4(voe_math_float4x4_mul_float4(m, (voe_math_float4){ 1.0f, 2.0f,
								 3.0f, 1.0f }),
	       EXACT, 12.0f, 24.0f, 36.0f, 1.0f);
	CHECK4(voe_math_float4x4_mul_float4(m, (voe_math_float4){ 1.0f, 2.0f,
								 3.0f, 0.0f }),
	       EXACT, 2.0f, 4.0f, 6.0f, 0.0f);
}

static void transpose_swaps_rows_and_columns(void)
{
	CHECK_MATRIX(voe_math_float4x4_transpose(counting()), EXACT,
		     1, 5,  9, 13,
		     2, 6, 10, 14,
		     3, 7, 11, 15,
		     4, 8, 12, 16);

	// Twice is the original, and a transpose that did nothing would pass
	// that on its own — which is why the elements above are checked first.
	CHECK_MATRIX(voe_math_float4x4_transpose(
			     voe_math_float4x4_transpose(counting())), EXACT,
		     1,  2,  3,  4,
		     5,  6,  7,  8,
		     9, 10, 11, 12,
		     13, 14, 15, 16);
}

static void determinant_of_the_known_cases(void)
{
	voe_math_float4x4 zero_row = voe_math_float4x4_identity();

	VOE_TEST_CHECK_FLOAT(
		voe_math_float4x4_determinant(voe_math_float4x4_identity()),
		1.0f, EXACT);

	// Scale multiplies volume, translation does not change it.
	VOE_TEST_CHECK_FLOAT(
		voe_math_float4x4_determinant(voe_math_float4x4_from_scale(
			(voe_math_float3){ 2.0f, 3.0f, 4.0f })),
		24.0f, EXACT);
	VOE_TEST_CHECK_FLOAT(
		voe_math_float4x4_determinant(
			voe_math_float4x4_from_translation(
				(voe_math_float3){ 5.0f, -6.0f, 7.0f })),
		1.0f, EXACT);

	// A zero row is singular, and exactly zero rather than nearly so:
	// every term of the determinant has a factor from that row.
	zero_row.m[2][2] = 0.0f;
	VOE_TEST_CHECK_FLOAT(voe_math_float4x4_determinant(zero_row),
			     0.0f, EXACT);
}

// The round trip both ways: the product is the identity, and a point that went
// through the matrix comes back. The second is the one a reader believes.
static void inverse_undoes_the_matrix(void)
{
	voe_math_float4x4 m = voe_math_float4x4_mul(
		voe_math_float4x4_from_translation(
			(voe_math_float3){ 1.0f, -2.0f, 3.0f }),
		voe_math_float4x4_from_scale(
			(voe_math_float3){ 2.0f, 4.0f, 8.0f }));
	voe_math_float4x4 inverse = voe_math_float4x4_inverse(m);
	voe_math_float3 p = { 4.0f, 8.0f, 16.0f };

	CHECK_MATRIX(voe_math_float4x4_inverse(m), NEAR,
		     0.5f, 0,     0,      -0.5f,
		     0,    0.25f, 0,       0.5f,
		     0,    0,     0.125f, -0.375f,
		     0,    0,     0,       1);

	CHECK_MATRIX(voe_math_float4x4_mul(m, inverse), NEAR,
		     1, 0, 0, 0,
		     0, 1, 0, 0,
		     0, 0, 1, 0,
		     0, 0, 0, 1);

	CHECK3(voe_math_float4x4_transform_point(
		       inverse, voe_math_float4x4_transform_point(m, p)),
	       NEAR, 4.0f, 8.0f, 16.0f);
}

int main(void)
{
	storage_is_row_major_and_has_no_padding();
	identity_and_the_two_constructors();
	translation_is_the_last_column();
	mul_is_the_matrix_product();
	composition_applies_the_right_hand_side_first();
	mul_float4_treats_the_vector_as_a_column();
	transform_point_takes_translation_and_dir_does_not();
	transpose_swaps_rows_and_columns();
	determinant_of_the_known_cases();
	inverse_undoes_the_matrix();

	return voe_test_result();
}
