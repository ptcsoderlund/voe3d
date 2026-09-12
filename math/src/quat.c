// The quat operations. See include/math/quat.h for the type and the rules.
#include <math/quat.h>

#include <assert.h>
#include <math.h>

voe_math_quat voe_math_quat_from_axis_angle(voe_math_float3 axis, float angle)
{
	// Normalized here rather than asserted on, so that a caller may hand in
	// an axis it computed. A zero axis asserts inside _normalize, which is
	// where that mistake already has a message.
	voe_math_float3 unit = voe_math_float3_normalize(axis);
	// Half the angle, which is what makes a quaternion a rotation rather
	// than a rotation of twice as much: the matrix in float4x4.c is
	// quadratic in these components, so every term doubles the angle back.
	float half = angle * 0.5f;
	float sine = sinf(half);

	return (voe_math_quat){ unit.x * sine, unit.y * sine, unit.z * sine,
				cosf(half) };
}

voe_math_quat voe_math_quat_mul(voe_math_quat a, voe_math_quat b)
{
	// The Hamilton product, written out. The three cross-product terms in
	// each of x, y and z are what make it non-commutative, which is the
	// whole reason the order in the header matters.
	return (voe_math_quat){
		a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
		a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
	};
}

float voe_math_quat_length(voe_math_quat q)
{
	// Written out rather than through a dot product, because this type has
	// no _dot: nothing has wanted one.
	return sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

voe_math_quat voe_math_quat_normalize(voe_math_quat q)
{
	float len = voe_math_quat_length(q);

	// The same assert voe_math_float4_normalize makes, for the same reason.
	assert(len > 0.0f);
	return (voe_math_quat){ q.x / len, q.y / len, q.z / len, q.w / len };
}
