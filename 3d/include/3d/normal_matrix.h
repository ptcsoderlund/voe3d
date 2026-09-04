// A world matrix into the matrix its normals want, and the only place in the
// engine where that difference is written down as arithmetic.
//
// A NORMAL IS NOT CARRIED BY A TRANSFORM THE WAY A POINT IS. Transform a
// surface's points by a matrix and transform its normal by the same matrix, and
// under a non-uniform scale the two no longer agree: the surface tilts one way
// and the normal tilts the other, so the normal stops being perpendicular to the
// thing it belongs to. What looks wrong on screen is the *lighting* — a flat
// face shaded as if it were angled, shading that slides across a solid object as
// it turns — and nothing about the geometry looks wrong at all, which is why
// this is a bug people chase in the shader for an afternoon.
//
// THE ANSWER IS THE INVERSE TRANSPOSE, AND IT IS THE SAME MATRIX FOR EVERYTHING
// THIS ENGINE DID BEFORE CARD 019. For a rotation, or a rotation with one
// uniform scale, the inverse transpose is the rotation again (up to a factor the
// shader normalizes away), so a model that is not squashed looks identical
// either way — which is exactly what makes this easy to leave out and hard to
// notice missing. The moment something has a scale of (1, 2, 1) on it, the two
// are different matrices.
//
// IT IS HERE AND NOT IN `math`, FOR THE REASON voe_3d_projection IS. `math`
// holds a transpose and an inverse and knows nothing about what a normal is;
// this is a fact about surfaces and about what `render`'s object record carries,
// and this folder is the one that knows both. It is a function rather than two
// lines inside the draw system so that it has somewhere to be tested — a
// non-uniformly scaled object being lit correctly is a claim, and
// 3d/tests/normal_matrix.c is where it is made.
//
// IT IS A FULL 4x4 BECAUSE THAT IS WHAT THE RECORD HOLDS. Only the upper-left
// three by three means anything for a direction; the shader multiplies with a w
// of zero, so the last column is ignored whatever the inverse transpose left in
// it, and the last row is ignored with it.
#pragma once

#include <math/float4x4.h>

// The matrix to transform this world matrix's normals with: the inverse
// transpose, not normalized — the shader normalizes what it interpolates anyway,
// so scaling this would be arithmetic done once per object to save nothing.
//
// A SINGULAR WORLD MATRIX COMES BACK AS THE IDENTITY. A scale of zero on any
// axis — a flattened object, which a file may legitimately hold and
// 3d/src/import.c says so — has no inverse, and `math`'s inverse asserts rather
// than handing back infinities. There is no right answer for the normals of a
// surface with no thickness in one direction, so the model's own normals are
// used unchanged: finite, visibly wrong if anybody ever looks at such a thing,
// and not a crash in the middle of a frame. So does one that is nearly
// singular; the source file says how near and why that is a threshold rather
// than a comparison with zero.
voe_math_float4x4 voe_3d_normal_matrix(voe_math_float4x4 world);
