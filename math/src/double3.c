// The double3 operations. See include/math/double3.h for the type and the rules.
#include <math/double3.h>

voe_math_double3 voe_math_double3_add(voe_math_double3 a, voe_math_double3 b)
{
	return (voe_math_double3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

voe_math_double3 voe_math_double3_sub(voe_math_double3 a, voe_math_double3 b)
{
	return (voe_math_double3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

voe_math_double3 voe_math_double3_from_float3(voe_math_float3 v)
{
	return (voe_math_double3){ v.x, v.y, v.z };
}

voe_math_float3 voe_math_double3_to_float3(voe_math_double3 v)
{
	return (voe_math_float3){ (float)v.x, (float)v.y, (float)v.z };
}
