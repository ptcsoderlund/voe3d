// double3's tests. What include/math/double3.h promises that can be checked
// from outside: add and sub keep a millimetre 100 km out, a difference of two
// far positions narrowed to float keeps it too, and widening then narrowing
// gives back the float3 it started from.
#include <math/double3.h>

#include <testing/test.h>

#define EXACT 0.0

// Per component, so a failure names the component that differed.
#define CHECK3(expr, tolerance, ex, ey, ez) do {			\
	voe_math_double3 result = (expr);				\
									\
	voe_test_check_float(result.x, (ex), (tolerance), __FILE__,	\
			     __LINE__, #expr ".x == " #ex);		\
	voe_test_check_float(result.y, (ey), (tolerance), __FILE__,	\
			     __LINE__, #expr ".y == " #ey);		\
	voe_test_check_float(result.z, (ez), (tolerance), __FILE__,	\
			     __LINE__, #expr ".z == " #ez);		\
} while (0)

// Compared against the same sum written in double, exactly: float arithmetic
// would lose the 0.001 altogether at 100000.25, so any narrowing inside add or
// sub fails here. Taking the step back off returns it to well under a micron.
static void add_and_sub_keep_a_millimetre_far_out(void)
{
	voe_math_double3 p = { 100000.25, -100000.25, 100000.25 };
	voe_math_double3 step = { 0.001, 0.001, -0.001 };
	voe_math_double3 moved = voe_math_double3_add(p, step);

	CHECK3(moved, EXACT, 100000.25 + 0.001, -100000.25 + 0.001,
	       100000.25 - 0.001);
	CHECK3(voe_math_double3_sub(moved, p), 1e-9, 0.001, 0.001, -0.001);
	CHECK3(voe_math_double3_sub(moved, step), 1e-9,
	       100000.25, -100000.25, 100000.25);
}

// The use _to_float3 is meant for: two positions 100 km out, a millimetre
// apart, subtracted in double and handed on as float relative to one another.
static void a_far_difference_narrowed_keeps_the_millimetre(void)
{
	voe_math_double3 camera = { 100000.0, 20.0, -100000.0 };
	voe_math_double3 thing = { 100000.001, 20.002, -100000.003 };
	voe_math_float3 rel = voe_math_double3_to_float3(
		voe_math_double3_sub(thing, camera));

	VOE_TEST_CHECK_FLOAT(rel.x, 0.001, 1e-6);
	VOE_TEST_CHECK_FLOAT(rel.y, 0.002, 1e-6);
	VOE_TEST_CHECK_FLOAT(rel.z, -0.003, 1e-6);
}

// Every float is a double, so the round trip is exact, and any component
// dropped or swapped on the way shows.
static void widen_then_narrow_is_the_identity(void)
{
	const voe_math_float3 v = { 1.5f, -2.25f, 3.1f };
	voe_math_float3 back = voe_math_double3_to_float3(
		voe_math_double3_from_float3(v));

	VOE_TEST_CHECK_FLOAT(back.x, v.x, EXACT);
	VOE_TEST_CHECK_FLOAT(back.y, v.y, EXACT);
	VOE_TEST_CHECK_FLOAT(back.z, v.z, EXACT);
}

int main(void)
{
	add_and_sub_keep_a_millimetre_far_out();
	a_far_difference_narrowed_keeps_the_millimetre();
	widen_then_narrow_is_the_identity();

	return voe_test_result();
}
