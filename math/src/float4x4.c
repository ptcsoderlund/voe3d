// The float4x4 operations. See include/math/float4x4.h for the layout, the
// multiplication convention and the slangc flag they depend on.
#include <math/float4x4.h>

#include <assert.h>

// The twelve 2x2 sub-determinants shared by the determinant and the inverse.
// s holds the six taken from the top two rows, t the six from the bottom two.
// Returns the determinant, so a caller that only wants that can ignore both.
static float voe_math_float4x4_cofactors(voe_math_float4x4 m, float *s, float *t)
{
	s[0] = m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0];
	s[1] = m.m[0][0] * m.m[1][2] - m.m[0][2] * m.m[1][0];
	s[2] = m.m[0][0] * m.m[1][3] - m.m[0][3] * m.m[1][0];
	s[3] = m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1];
	s[4] = m.m[0][1] * m.m[1][3] - m.m[0][3] * m.m[1][1];
	s[5] = m.m[0][2] * m.m[1][3] - m.m[0][3] * m.m[1][2];

	t[0] = m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0];
	t[1] = m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0];
	t[2] = m.m[2][0] * m.m[3][3] - m.m[2][3] * m.m[3][0];
	t[3] = m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1];
	t[4] = m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1];
	t[5] = m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2];

	return s[0] * t[5] - s[1] * t[4] + s[2] * t[3]
	     + s[3] * t[2] - s[4] * t[1] + s[5] * t[0];
}

voe_math_float4x4 voe_math_float4x4_identity(void)
{
	return (voe_math_float4x4){ {
		{ 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f },
	} };
}

voe_math_float4x4 voe_math_float4x4_from_translation(voe_math_float3 t)
{
	voe_math_float4x4 r = voe_math_float4x4_identity();

	r.m[0][3] = t.x;
	r.m[1][3] = t.y;
	r.m[2][3] = t.z;
	return r;
}

voe_math_float4x4 voe_math_float4x4_from_scale(voe_math_float3 s)
{
	voe_math_float4x4 r = voe_math_float4x4_identity();

	r.m[0][0] = s.x;
	r.m[1][1] = s.y;
	r.m[2][2] = s.z;
	return r;
}

voe_math_float4x4 voe_math_float4x4_mul(voe_math_float4x4 a, voe_math_float4x4 b)
{
	voe_math_float4x4 r;

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {
			r.m[row][col] = a.m[row][0] * b.m[0][col]
				      + a.m[row][1] * b.m[1][col]
				      + a.m[row][2] * b.m[2][col]
				      + a.m[row][3] * b.m[3][col];
		}
	}
	return r;
}

voe_math_float4 voe_math_float4x4_mul_float4(voe_math_float4x4 m, voe_math_float4 v)
{
	float r[4];

	for (int row = 0; row < 4; row++) {
		r[row] = m.m[row][0] * v.x + m.m[row][1] * v.y
		       + m.m[row][2] * v.z + m.m[row][3] * v.w;
	}
	return (voe_math_float4){ r[0], r[1], r[2], r[3] };
}

voe_math_float3 voe_math_float4x4_transform_point(voe_math_float4x4 m, voe_math_float3 p)
{
	float r[3];

	for (int row = 0; row < 3; row++) {
		r[row] = m.m[row][0] * p.x + m.m[row][1] * p.y
		       + m.m[row][2] * p.z + m.m[row][3];
	}
	return (voe_math_float3){ r[0], r[1], r[2] };
}

voe_math_float3 voe_math_float4x4_transform_dir(voe_math_float4x4 m, voe_math_float3 d)
{
	float r[3];

	for (int row = 0; row < 3; row++) {
		r[row] = m.m[row][0] * d.x + m.m[row][1] * d.y + m.m[row][2] * d.z;
	}
	return (voe_math_float3){ r[0], r[1], r[2] };
}

voe_math_float4x4 voe_math_float4x4_transpose(voe_math_float4x4 m)
{
	voe_math_float4x4 r;

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++)
			r.m[row][col] = m.m[col][row];
	}
	return r;
}

float voe_math_float4x4_determinant(voe_math_float4x4 m)
{
	float s[6];
	float t[6];

	return voe_math_float4x4_cofactors(m, s, t);
}

voe_math_float4x4 voe_math_float4x4_inverse(voe_math_float4x4 m)
{
	float s[6];
	float t[6];
	float det = voe_math_float4x4_cofactors(m, s, t);
	voe_math_float4x4 r;
	float d;

	assert(det != 0.0f);
	d = 1.0f / det;

	r.m[0][0] = ( m.m[1][1] * t[5] - m.m[1][2] * t[4] + m.m[1][3] * t[3]) * d;
	r.m[0][1] = (-m.m[0][1] * t[5] + m.m[0][2] * t[4] - m.m[0][3] * t[3]) * d;
	r.m[0][2] = ( m.m[3][1] * s[5] - m.m[3][2] * s[4] + m.m[3][3] * s[3]) * d;
	r.m[0][3] = (-m.m[2][1] * s[5] + m.m[2][2] * s[4] - m.m[2][3] * s[3]) * d;

	r.m[1][0] = (-m.m[1][0] * t[5] + m.m[1][2] * t[2] - m.m[1][3] * t[1]) * d;
	r.m[1][1] = ( m.m[0][0] * t[5] - m.m[0][2] * t[2] + m.m[0][3] * t[1]) * d;
	r.m[1][2] = (-m.m[3][0] * s[5] + m.m[3][2] * s[2] - m.m[3][3] * s[1]) * d;
	r.m[1][3] = ( m.m[2][0] * s[5] - m.m[2][2] * s[2] + m.m[2][3] * s[1]) * d;

	r.m[2][0] = ( m.m[1][0] * t[4] - m.m[1][1] * t[2] + m.m[1][3] * t[0]) * d;
	r.m[2][1] = (-m.m[0][0] * t[4] + m.m[0][1] * t[2] - m.m[0][3] * t[0]) * d;
	r.m[2][2] = ( m.m[3][0] * s[4] - m.m[3][1] * s[2] + m.m[3][3] * s[0]) * d;
	r.m[2][3] = (-m.m[2][0] * s[4] + m.m[2][1] * s[2] - m.m[2][3] * s[0]) * d;

	r.m[3][0] = (-m.m[1][0] * t[3] + m.m[1][1] * t[1] - m.m[1][2] * t[0]) * d;
	r.m[3][1] = ( m.m[0][0] * t[3] - m.m[0][1] * t[1] + m.m[0][2] * t[0]) * d;
	r.m[3][2] = (-m.m[3][0] * s[3] + m.m[3][1] * s[1] - m.m[3][2] * s[0]) * d;
	r.m[3][3] = ( m.m[2][0] * s[3] - m.m[2][1] * s[1] + m.m[2][2] * s[0]) * d;

	return r;
}
