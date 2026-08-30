// The float4 operations. See include/math/float4.h for the type and the rules.
#include <math/float4.h>

#include <assert.h>
#include <math.h>

voe_math_float4 voe_math_float4_add(voe_math_float4 a, voe_math_float4 b)
{
	return (voe_math_float4){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

voe_math_float4 voe_math_float4_sub(voe_math_float4 a, voe_math_float4 b)
{
	return (voe_math_float4){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

voe_math_float4 voe_math_float4_mul(voe_math_float4 a, voe_math_float4 b)
{
	return (voe_math_float4){ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}

voe_math_float4 voe_math_float4_div(voe_math_float4 a, voe_math_float4 b)
{
	assert(b.x != 0.0f && b.y != 0.0f && b.z != 0.0f && b.w != 0.0f);
	return (voe_math_float4){ a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w };
}

voe_math_float4 voe_math_float4_scale(voe_math_float4 v, float s)
{
	return (voe_math_float4){ v.x * s, v.y * s, v.z * s, v.w * s };
}

voe_math_float4 voe_math_float4_neg(voe_math_float4 v)
{
	return (voe_math_float4){ -v.x, -v.y, -v.z, -v.w };
}

float voe_math_float4_dot(voe_math_float4 a, voe_math_float4 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float voe_math_float4_length(voe_math_float4 v)
{
	return sqrtf(voe_math_float4_dot(v, v));
}

voe_math_float4 voe_math_float4_normalize(voe_math_float4 v)
{
	float len = voe_math_float4_length(v);

	assert(len > 0.0f);
	return voe_math_float4_scale(v, 1.0f / len);
}

voe_math_float4 voe_math_float4_lerp(voe_math_float4 a, voe_math_float4 b, float t)
{
	return voe_math_float4_add(a, voe_math_float4_scale(voe_math_float4_sub(b, a), t));
}

voe_math_float4 voe_math_float4_min(voe_math_float4 a, voe_math_float4 b)
{
	return (voe_math_float4){ fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z),
				  fminf(a.w, b.w) };
}

voe_math_float4 voe_math_float4_max(voe_math_float4 a, voe_math_float4 b)
{
	return (voe_math_float4){ fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z),
				  fmaxf(a.w, b.w) };
}

voe_math_float4 voe_math_float4_clamp(voe_math_float4 v, voe_math_float4 lo,
				      voe_math_float4 hi)
{
	assert(lo.x <= hi.x && lo.y <= hi.y && lo.z <= hi.z && lo.w <= hi.w);
	return voe_math_float4_min(voe_math_float4_max(v, lo), hi);
}
