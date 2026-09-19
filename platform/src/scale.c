// The two scale conversions in scale.h. Used by window_wayland.c; OS-free, and
// built on both platforms for the test's sake.
//
// The length is integer arithmetic in 64 bits: logical × scale cannot overflow
// there for any int and any 32-bit scale, and adding half the divisor before the
// division is the round-half-away-from-zero the protocol asks for, mirrored for
// a negative length.
#include "scale.h"

int voe_platform_scale_length(int logical, uint32_t scale)
{
	int64_t product = (int64_t)logical * (int64_t)scale;

	if (product >= 0)
		return (int)((product + 60) / 120);
	return (int)-((-product + 60) / 120);
}

double voe_platform_scale_position(double logical, uint32_t scale)
{
	return logical * (double)scale / 120.0;
}
