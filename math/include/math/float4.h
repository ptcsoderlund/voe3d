// float4 — a four-component float vector, spelled as Slang spells it.
//
// A pure value type: passed and returned by value, never through a pointer, and
// owned by nobody. The type name and the component names match Slang's float4 so
// that a value here and a value in a shader are the same thing, written the same
// way.
//
// The component-wise operations follow Slang's operators rather than any
// mathematical vocabulary: voe_math_float4_mul is Slang's `a * b`, component by
// component — it is not a dot product. Multiplying by a scalar is _scale.
//
// This is the vector a float4x4 multiplies; see float4x4.h.
//
// _normalize and _div assert rather than return a quiet NaN. A zero-length
// vector is a bug at the call site, not a value to propagate.
#pragma once

typedef struct {
	float x, y, z, w;
} voe_math_float4;

voe_math_float4 voe_math_float4_add(voe_math_float4 a, voe_math_float4 b);
voe_math_float4 voe_math_float4_sub(voe_math_float4 a, voe_math_float4 b);
voe_math_float4 voe_math_float4_mul(voe_math_float4 a, voe_math_float4 b);
voe_math_float4 voe_math_float4_div(voe_math_float4 a, voe_math_float4 b);
voe_math_float4 voe_math_float4_scale(voe_math_float4 v, float s);
voe_math_float4 voe_math_float4_neg(voe_math_float4 v);

float voe_math_float4_dot(voe_math_float4 a, voe_math_float4 b);
float voe_math_float4_length(voe_math_float4 v);
voe_math_float4 voe_math_float4_normalize(voe_math_float4 v);

voe_math_float4 voe_math_float4_lerp(voe_math_float4 a, voe_math_float4 b, float t);
voe_math_float4 voe_math_float4_min(voe_math_float4 a, voe_math_float4 b);
voe_math_float4 voe_math_float4_max(voe_math_float4 a, voe_math_float4 b);
voe_math_float4 voe_math_float4_clamp(voe_math_float4 v, voe_math_float4 lo,
				      voe_math_float4 hi);
