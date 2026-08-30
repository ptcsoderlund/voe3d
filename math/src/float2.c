// The float2 operations. See include/math/float2.h for the type and the rules.
#include <math/float2.h>

#include <assert.h>
#include <math.h>

voe_math_float2 voe_math_float2_add(voe_math_float2 a, voe_math_float2 b)
{
	return (voe_math_float2){ a.x + b.x, a.y + b.y };
}

voe_math_float2 voe_math_float2_sub(voe_math_float2 a, voe_math_float2 b)
{
	return (voe_math_float2){ a.x - b.x, a.y - b.y };
}

voe_math_float2 voe_math_float2_mul(voe_math_float2 a, voe_math_float2 b)
{
	return (voe_math_float2){ a.x * b.x, a.y * b.y };
}

voe_math_float2 voe_math_float2_div(voe_math_float2 a, voe_math_float2 b)
{
	assert(b.x != 0.0f && b.y != 0.0f);
	return (voe_math_float2){ a.x / b.x, a.y / b.y };
}

voe_math_float2 voe_math_float2_scale(voe_math_float2 v, float s)
{
	return (voe_math_float2){ v.x * s, v.y * s };
}

voe_math_float2 voe_math_float2_neg(voe_math_float2 v)
{
	return (voe_math_float2){ -v.x, -v.y };
}

float voe_math_float2_dot(voe_math_float2 a, voe_math_float2 b)
{
	return a.x * b.x + a.y * b.y;
}

float voe_math_float2_length(voe_math_float2 v)
{
	return sqrtf(voe_math_float2_dot(v, v));
}

voe_math_float2 voe_math_float2_normalize(voe_math_float2 v)
{
	float len = voe_math_float2_length(v);

	assert(len > 0.0f);
	return voe_math_float2_scale(v, 1.0f / len);
}

voe_math_float2 voe_math_float2_lerp(voe_math_float2 a, voe_math_float2 b, float t)
{
	return voe_math_float2_add(a, voe_math_float2_scale(voe_math_float2_sub(b, a), t));
}

voe_math_float2 voe_math_float2_min(voe_math_float2 a, voe_math_float2 b)
{
	return (voe_math_float2){ fminf(a.x, b.x), fminf(a.y, b.y) };
}

voe_math_float2 voe_math_float2_max(voe_math_float2 a, voe_math_float2 b)
{
	return (voe_math_float2){ fmaxf(a.x, b.x), fmaxf(a.y, b.y) };
}

voe_math_float2 voe_math_float2_clamp(voe_math_float2 v, voe_math_float2 lo,
				      voe_math_float2 hi)
{
	assert(lo.x <= hi.x && lo.y <= hi.y);
	return voe_math_float2_min(voe_math_float2_max(v, lo), hi);
}
