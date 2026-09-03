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
// builds one by construction, and voe_math_float4x4_from_quat asserts it before
// using the short formula that only holds for a unit quaternion.
//
// THERE IS ONE WAY TO BUILD ONE AND THAT IS ON PURPOSE. No _identity, no _mul,
// no _normalize, no _slerp: nothing composes or interpolates a rotation yet, and
// rule 10 says a function is written when something calls it. The day a node
// hierarchy or an animation needs them, that is the card they arrive on. A
// rotation of nothing is _from_axis_angle with an angle of zero, which is the
// identity and is tested as such.
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
