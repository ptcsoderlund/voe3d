// float4x4 — a 4x4 float matrix, spelled as Slang spells it.
//
// Row-major, and that is the whole point of the type. Element (row, column)
// lives at m[row][column], which is what the same expression means in Slang
// source, so an index written here and an index written in a shader pick out the
// same element. Uploading one to the GPU is a straight copy of the 16 floats.
//
// This costs one flag: slangc must be invoked with -matrix-layout-row-major,
// because slangc's own default is column-major for legacy reasons. Whichever
// folder ends up invoking slangc owns that flag. Without it the layouts disagree
// silently — nothing fails to compile, the transforms just come out transposed.
//
// Vectors are columns. voe_math_float4x4_mul_float4(m, v) is M·v, which is what
// Slang's mul(M, v) computes, and translation therefore sits in the last column:
// m[0][3], m[1][3], m[2][3]. Composition reads right to left, so mul(a, b)
// applies b first.
//
// _transform_point and _transform_dir are affine: they assume the matrix has no
// projective part and skip the w divide. Point takes the translation, direction
// does not. Projection matrices are not built here — they encode a clip-space
// convention, and this folder does not know about any graphics API.
//
// _inverse asserts on a singular matrix rather than returning infinities.
#pragma once

#include <math/float3.h>
#include <math/float4.h>

typedef struct {
	float m[4][4];
} voe_math_float4x4;

voe_math_float4x4 voe_math_float4x4_identity(void);
voe_math_float4x4 voe_math_float4x4_from_translation(voe_math_float3 t);
voe_math_float4x4 voe_math_float4x4_from_scale(voe_math_float3 s);

voe_math_float4x4 voe_math_float4x4_mul(voe_math_float4x4 a, voe_math_float4x4 b);
voe_math_float4 voe_math_float4x4_mul_float4(voe_math_float4x4 m, voe_math_float4 v);
voe_math_float3 voe_math_float4x4_transform_point(voe_math_float4x4 m, voe_math_float3 p);
voe_math_float3 voe_math_float4x4_transform_dir(voe_math_float4x4 m, voe_math_float3 d);

voe_math_float4x4 voe_math_float4x4_transpose(voe_math_float4x4 m);
float voe_math_float4x4_determinant(voe_math_float4x4 m);
voe_math_float4x4 voe_math_float4x4_inverse(voe_math_float4x4 m);
