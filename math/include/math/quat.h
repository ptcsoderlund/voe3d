// quat — a unit quaternion, and the one rotation type in this engine.
//
// A pure value type, exactly as the vectors are: passed and returned by value,
// never through a pointer, and owned by nobody.
//
// THE NAME IS NOT SLANG'S, BECAUSE SLANG HAS NONE. Every other type in this
// folder is spelled the way Slang spells it, so that a value here and a value in
// a shader are the same thing written the same way. Slang has no quaternion, so
// there is nothing to copy and nothing to keep in step, and `quat` is the word
// the original maths card used. Nothing stops a quaternion reaching a shader
// except that nothing has wanted to: what crosses to the GPU today is the matrix
// it became, through voe_math_float4x4_from_quat in float4x4.h.
//
// THE COMPONENT ORDER IS x, y, z, w, AND w BEING LAST IS A DECISION. Half the
// literature writes the scalar first. The vector part first is what makes this
// type's four floats the same four floats, in the same order, that glTF stores a
// node's rotation as — so an importer copies them and does not shuffle them.
//
// IT IS UNIT LENGTH AND EVERYTHING THAT TAKES ONE ASSUMES SO. _from_axis_angle
// builds one by construction, _normalize makes one out of anything that is not,
// and voe_math_float4x4_from_quat asserts it before using the short formula that
// only holds for a unit quaternion.
//
// EVERY FUNCTION HERE ARRIVED WHEN SOMETHING CALLED IT, AND THAT IS ON PURPOSE.
// Rule 10: a function is written when something calls it, not for symmetry. _mul
// came with composition; _length and _normalize came with a caller that is
// handed a rotation from outside and has to make sure it is still one before it
// keeps it. Still no _identity and no _slerp: nothing interpolates a rotation
// yet, and the day an animation does is the card they arrive on. A rotation of
// nothing is _from_axis_angle with an angle of zero, which is the identity and
// is tested as such.
//
// ROTATION IS RIGHT-HANDED, WHICH IS THIS ENGINE'S HANDEDNESS. A positive angle
// about +Y takes +Z towards +X. tests/quat.c is what says so, rather than this
// comment.
#pragma once

#include <math/float3.h>

typedef struct {
	float x, y, z, w;
} voe_math_quat;

// axis need not be unit — it is normalized here — but it must not be zero, and
// a zero one asserts. angle is in radians, as every angle in this engine is.
voe_math_quat voe_math_quat_from_axis_angle(voe_math_float3 axis, float angle);

// Two rotations, composed. IT READS RIGHT TO LEFT, THE SAME WAY THE MATRICES DO:
// _mul(a, b) turns by b and then by a, and it is the rotation
// voe_math_float4x4_from_quat turns into voe_math_float4x4_mul of the two
// matrices in that same order. tests/quat.c is what says so.
//
// Neither argument has to be unit and the result is as unit as they were, which
// is what lets a chain of these be built without normalizing between them.
voe_math_quat voe_math_quat_mul(voe_math_quat a, voe_math_quat b);

// The square root of the sum of the four components squared, exactly as
// voe_math_float4_length is over its four.
float voe_math_quat_length(voe_math_quat q);

// q divided by its length. It asserts rather than return a quiet NaN, for the
// reason float4.h gives about its own: a zero length is a bug at the call site,
// not a value to propagate.
voe_math_quat voe_math_quat_normalize(voe_math_quat q);
