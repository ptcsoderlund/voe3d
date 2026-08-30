// float2 — a two-component float vector, spelled as Slang spells it.
//
// A pure value type: passed and returned by value, never through a pointer, and
// owned by nobody. The type name and the component names match Slang's float2 so
// that a value here and a value in a shader are the same thing, written the same
// way.
//
// The component-wise operations follow Slang's operators rather than any
// mathematical vocabulary: voe_math_float2_mul is Slang's `a * b`, component by
// component — it is not a dot product. Multiplying by a scalar is _scale.
//
// _normalize and _div assert rather than return a quiet NaN. A zero-length
// vector is a bug at the call site, not a value to propagate.
#pragma once

typedef struct {
	float x, y;
} voe_math_float2;

voe_math_float2 voe_math_float2_add(voe_math_float2 a, voe_math_float2 b);
voe_math_float2 voe_math_float2_sub(voe_math_float2 a, voe_math_float2 b);
voe_math_float2 voe_math_float2_mul(voe_math_float2 a, voe_math_float2 b);
voe_math_float2 voe_math_float2_div(voe_math_float2 a, voe_math_float2 b);
voe_math_float2 voe_math_float2_scale(voe_math_float2 v, float s);
voe_math_float2 voe_math_float2_neg(voe_math_float2 v);

float voe_math_float2_dot(voe_math_float2 a, voe_math_float2 b);
float voe_math_float2_length(voe_math_float2 v);
voe_math_float2 voe_math_float2_normalize(voe_math_float2 v);

voe_math_float2 voe_math_float2_lerp(voe_math_float2 a, voe_math_float2 b, float t);
voe_math_float2 voe_math_float2_min(voe_math_float2 a, voe_math_float2 b);
voe_math_float2 voe_math_float2_max(voe_math_float2 a, voe_math_float2 b);
voe_math_float2 voe_math_float2_clamp(voe_math_float2 v, voe_math_float2 lo,
				      voe_math_float2 hi);
