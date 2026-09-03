// The quat operations. See include/math/quat.h for the type and the rules.
#include <math/quat.h>

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
