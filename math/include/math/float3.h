// float3 — a three-component float vector, spelled as Slang spells it.
//
// A pure value type: passed and returned by value, never through a pointer, and
// owned by nobody. The type name and the component names match Slang's float3 so
// that a value here and a value in a shader are the same thing, written the same
// way.
//
// The component-wise operations follow Slang's operators rather than any
// mathematical vocabulary: voe_math_float3_mul is Slang's `a * b`, component by
// component — it is not a dot product. Multiplying by a scalar is _scale.
//
// _cross is right-handed: x cross y is z.
//
// _normalize and _div assert rather than return a quiet NaN. A zero-length
// vector is a bug at the call site, not a value to propagate.
#pragma once

typedef struct {
	float x, y, z;
} voe_math_float3;

voe_math_float3 voe_math_float3_add(voe_math_float3 a, voe_math_float3 b);
voe_math_float3 voe_math_float3_sub(voe_math_float3 a, voe_math_float3 b);
voe_math_float3 voe_math_float3_mul(voe_math_float3 a, voe_math_float3 b);
voe_math_float3 voe_math_float3_div(voe_math_float3 a, voe_math_float3 b);
voe_math_float3 voe_math_float3_scale(voe_math_float3 v, float s);
voe_math_float3 voe_math_float3_neg(voe_math_float3 v);

float voe_math_float3_dot(voe_math_float3 a, voe_math_float3 b);
voe_math_float3 voe_math_float3_cross(voe_math_float3 a, voe_math_float3 b);
float voe_math_float3_length(voe_math_float3 v);
voe_math_float3 voe_math_float3_normalize(voe_math_float3 v);

voe_math_float3 voe_math_float3_lerp(voe_math_float3 a, voe_math_float3 b, float t);
voe_math_float3 voe_math_float3_min(voe_math_float3 a, voe_math_float3 b);
voe_math_float3 voe_math_float3_max(voe_math_float3 a, voe_math_float3 b);
voe_math_float3 voe_math_float3_clamp(voe_math_float3 v, voe_math_float3 lo,
				      voe_math_float3 hi);
