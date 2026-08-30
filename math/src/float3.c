// The float3 operations. See include/math/float3.h for the type and the rules.
#include <math/float3.h>

#include <assert.h>
#include <math.h>

voe_math_float3 voe_math_float3_add(voe_math_float3 a, voe_math_float3 b)
{
	return (voe_math_float3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

voe_math_float3 voe_math_float3_sub(voe_math_float3 a, voe_math_float3 b)
{
	return (voe_math_float3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

voe_math_float3 voe_math_float3_mul(voe_math_float3 a, voe_math_float3 b)
{
	return (voe_math_float3){ a.x * b.x, a.y * b.y, a.z * b.z };
}

voe_math_float3 voe_math_float3_div(voe_math_float3 a, voe_math_float3 b)
{
	assert(b.x != 0.0f && b.y != 0.0f && b.z != 0.0f);
	return (voe_math_float3){ a.x / b.x, a.y / b.y, a.z / b.z };
}

voe_math_float3 voe_math_float3_scale(voe_math_float3 v, float s)
{
	return (voe_math_float3){ v.x * s, v.y * s, v.z * s };
}

voe_math_float3 voe_math_float3_neg(voe_math_float3 v)
{
	return (voe_math_float3){ -v.x, -v.y, -v.z };
}

float voe_math_float3_dot(voe_math_float3 a, voe_math_float3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

voe_math_float3 voe_math_float3_cross(voe_math_float3 a, voe_math_float3 b)
{
	return (voe_math_float3){
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
	};
}

float voe_math_float3_length(voe_math_float3 v)
{
	return sqrtf(voe_math_float3_dot(v, v));
}

voe_math_float3 voe_math_float3_normalize(voe_math_float3 v)
{
	float len = voe_math_float3_length(v);

	assert(len > 0.0f);
	return voe_math_float3_scale(v, 1.0f / len);
}

voe_math_float3 voe_math_float3_lerp(voe_math_float3 a, voe_math_float3 b, float t)
{
	return voe_math_float3_add(a, voe_math_float3_scale(voe_math_float3_sub(b, a), t));
}

voe_math_float3 voe_math_float3_min(voe_math_float3 a, voe_math_float3 b)
{
	return (voe_math_float3){ fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z) };
}

voe_math_float3 voe_math_float3_max(voe_math_float3 a, voe_math_float3 b)
{
	return (voe_math_float3){ fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z) };
}

voe_math_float3 voe_math_float3_clamp(voe_math_float3 v, voe_math_float3 lo,
				      voe_math_float3 hi)
{
	assert(lo.x <= hi.x && lo.y <= hi.y && lo.z <= hi.z);
	return voe_math_float3_min(voe_math_float3_max(v, lo), hi);
}
