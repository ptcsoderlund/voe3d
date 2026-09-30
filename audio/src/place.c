// The listener and the placement law behind audio/place.h: the reference from
// where the view axis meets y = 0, the falloff, the screen-x pan and the
// balance law, all in float on a difference taken in double.
#include <audio/place.h>

#include <base/assert.h>

#include <math.h>
#include <stddef.h>

voe_audio_listener voe_audio_listener_make(voe_math_double3 position,
					   voe_math_float3 right,
					   voe_math_float3 forward,
					   float tan_half_width)
{
	VOE_BASE_ASSERT(tan_half_width > 0.0f, "a listener sees a width");
	// Forward is a unit vector, so the ray parameter at y = 0 is its distance.
	const double along = forward.y != 0.0f ? -position.y / (double)forward.y : 0.0;
	const voe_audio_listener listener = {
		.position = position, .right = right, .forward = forward,
		.tan_half_width = tan_half_width,
		.reference = along > 0.0 ? (float)along : VOE_AUDIO_REFERENCE,
	};

	VOE_BASE_ASSERT(listener.reference > 0.0f, "a reference is a distance");
	return listener;
}

voe_audio_gains voe_audio_unplaced(void)
{
	return (voe_audio_gains){ 1.0f, 1.0f };
}

voe_audio_gains voe_audio_place(const voe_audio_listener *listener,
				voe_math_double3 where)
{
	VOE_BASE_ASSERT(listener != NULL, "a place is heard by a listener");
	const voe_math_float3 d = voe_math_double3_to_float3(
		voe_math_double3_sub(where, listener->position));
	const float distance = voe_math_float3_length(d);

	if (distance == 0.0f)
		return voe_audio_unplaced();
	const float ratio = listener->reference / distance;
	const float gain = fminf(1.0f, ratio * ratio);
	const float ahead = voe_math_float3_dot(d, listener->forward);
	const float side = voe_math_float3_dot(d, listener->right);
	float pan = side > 0.0f ? 1.0f : side < 0.0f ? -1.0f : 0.0f;

	if (ahead > 0.0f)
		pan = fmaxf(-1.0f, fminf(1.0f, side / (ahead * listener->tan_half_width)));
	const voe_audio_gains gains = {
		.left = gain * fminf(1.0f, 1.0f - pan),
		.right = gain * fminf(1.0f, 1.0f + pan),
	};

	VOE_BASE_ASSERT(gains.left >= 0.0f && gains.right >= 0.0f, "a gain is never negative");
	return gains;
}
