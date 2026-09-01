// float2's tests. Everything include/math/float2.h promises that can be checked
// from outside: the component-wise operations, that _mul is Slang's `a * b` and
// not a dot product, that _scale takes a scalar, and the geometry — dot, length,
// normalize, lerp, min, max and clamp.
//
// The assert paths are not tested. A zero divisor and a zero-length normalize
// abort the process by design, and a test that aborts reports nothing.
#include <math/float2.h>

#include <testing/test.h>

// Two tolerances, and which one an assertion uses is a claim about the
// operation. EXACT is for values that are representable and reached in one add
// or one multiply, where a near miss is a wrong result and not rounding. NEAR is
// for the ones that went through a square root or a division.
#define EXACT 0.0f
#define NEAR 1e-6f

// One check per component, so a failure names the component that differed rather
// than only the vector. The expression is stringified here instead of being
// handed to VOE_TEST_CHECK_FLOAT so the message reads as the call was written.
#define CHECK2(expr, tolerance, ex, ey) do {				\
	voe_math_float2 result = (expr);				\
									\
	voe_test_check_float(result.x, (ex), (tolerance), __FILE__,	\
			     __LINE__, #expr ".x == " #ex);		\
	voe_test_check_float(result.y, (ey), (tolerance), __FILE__,	\
			     __LINE__, #expr ".y == " #ey);		\
} while (0)

static const voe_math_float2 a = { 1.0f, 2.0f };
static const voe_math_float2 b = { 4.0f, 5.0f };

static void arithmetic_is_component_wise(void)
{
	CHECK2(voe_math_float2_add(a, b), EXACT, 5.0f, 7.0f);
	CHECK2(voe_math_float2_sub(a, b), EXACT, -3.0f, -3.0f);
	CHECK2(voe_math_float2_neg(a), EXACT, -1.0f, -2.0f);
	CHECK2(voe_math_float2_div((voe_math_float2){ 8.0f, 6.0f },
				   (voe_math_float2){ 2.0f, 4.0f }),
	       EXACT, 4.0f, 1.5f);
}

// The one that catches Slang's spelling being read as mathematical vocabulary.
// _mul is `a * b` component by component; the dot product is the sum of those
// same components and has its own name.
static void mul_is_component_wise_and_dot_is_not(void)
{
	CHECK2(voe_math_float2_mul(a, b), EXACT, 4.0f, 10.0f);
	VOE_TEST_CHECK_FLOAT(voe_math_float2_dot(a, b), 14.0f, EXACT);
}

// _scale is the one that takes a scalar. Nothing else does.
static void scale_takes_a_scalar(void)
{
	CHECK2(voe_math_float2_scale(a, 3.0f), EXACT, 3.0f, 6.0f);
	CHECK2(voe_math_float2_scale(a, 0.0f), EXACT, 0.0f, 0.0f);
}

static void length_and_normalize(void)
{
	voe_math_float2 v = { 3.0f, 4.0f };

	VOE_TEST_CHECK_FLOAT(voe_math_float2_length(v), 5.0f, EXACT);
	CHECK2(voe_math_float2_normalize(v), NEAR, 0.6f, 0.8f);
	VOE_TEST_CHECK_FLOAT(
		voe_math_float2_length(voe_math_float2_normalize(v)),
		1.0f, NEAR);
}

// The endpoints are exact on purpose: a lerp that does not return a at t == 0
// and b at t == 1 is wrong however good it looks in between.
static void lerp_hits_both_endpoints(void)
{
	CHECK2(voe_math_float2_lerp(a, b, 0.0f), EXACT, 1.0f, 2.0f);
	CHECK2(voe_math_float2_lerp(a, b, 1.0f), EXACT, 4.0f, 5.0f);
	CHECK2(voe_math_float2_lerp(a, b, 0.5f), EXACT, 2.5f, 3.5f);
}

// Per component, and mixed on purpose so that returning one whole argument
// rather than choosing component by component fails.
static void min_max_and_clamp_are_per_component(void)
{
	voe_math_float2 p = { 1.0f, 5.0f };
	voe_math_float2 q = { 4.0f, 2.0f };
	voe_math_float2 lo = { 0.0f, 0.0f };
	voe_math_float2 hi = { 2.0f, 2.0f };

	CHECK2(voe_math_float2_min(p, q), EXACT, 1.0f, 2.0f);
	CHECK2(voe_math_float2_max(p, q), EXACT, 4.0f, 5.0f);
	CHECK2(voe_math_float2_clamp((voe_math_float2){ -1.0f, 9.0f }, lo, hi),
	       EXACT, 0.0f, 2.0f);
	CHECK2(voe_math_float2_clamp((voe_math_float2){ 1.0f, 1.0f }, lo, hi),
	       EXACT, 1.0f, 1.0f);
}

int main(void)
{
	arithmetic_is_component_wise();
	mul_is_component_wise_and_dot_is_not();
	scale_takes_a_scalar();
	length_and_normalize();
	lerp_hits_both_endpoints();
	min_max_and_clamp_are_per_component();

	return voe_test_result();
}
