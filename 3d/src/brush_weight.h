// A brush's soft weight at a distance from its centre (0379 point 3): 1
// inside radius·(1 − softness), smoothstepping to 0 at the radius. Internal to
// 3d: the sculpt and paint stamps (landscape.c) and the foliage stamps
// (foliage.c) share it, so a ring drawn for one brush means the same for all.
//
//     const float w = voe_3d_brush_weight(radius, softness, distance);
//
// Arithmetic only. A radius of 0 or a softness outside 0..1 asserts.
#pragma once

#include <base/assert.h>

static inline float voe_3d_brush_weight(float radius, float softness,
					float distance)
{
	VOE_BASE_ASSERT(radius > 0.0f, "a radius");
	VOE_BASE_ASSERT(softness >= 0.0f && softness <= 1.0f, "a softness");
	const float inner = radius * (1.0f - softness);

	if (distance <= inner)
		return 1.0f;
	if (distance >= radius)
		return 0.0f;
	const float t = (radius - distance) / (radius - inner);

	return t * t * (3.0f - 2.0f * t);
}
