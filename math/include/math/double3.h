// double3 — a three-component double vector, spelled as Slang spells it.
//
// A pure value type like float3: passed and returned by value, owned by nobody.
//
// It exists for one thing: a world position (ADR-0250). A float keeps about
// seven digits, which is 8 mm steps 100 km out, so positions are double while
// rotation, scale and everything the GPU sees stay float.
//
// _to_float3 narrows, and is meant for the difference of two nearby positions
// (a position relative to the camera or to a query's origin), which is small
// and fits a float with room to spare. Narrowing a raw world position throws
// away exactly the precision this type is here to keep.
//
// Only add, sub, widen and narrow are here. Length, scale and dot are added
// when something calls them (rule 10); the math inside a query runs in float,
// on differences.
#pragma once

#include <math/float3.h>

typedef struct {
	double x, y, z;
} voe_math_double3;

voe_math_double3 voe_math_double3_add(voe_math_double3 a, voe_math_double3 b);
voe_math_double3 voe_math_double3_sub(voe_math_double3 a, voe_math_double3 b);
voe_math_double3 voe_math_double3_from_float3(voe_math_float3 v);
voe_math_float3 voe_math_double3_to_float3(voe_math_double3 v);
