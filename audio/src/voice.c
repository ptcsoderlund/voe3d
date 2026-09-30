// One voice read and summed into a mix, behind voice.h: linear interpolation
// at the voice's pitch, a loop wrapped across its end, and the gain ramped
// linearly across the call's frames.
//
// THE LOOP HAS NO SEAM. Past its last frame a loop interpolates into its first
// and its position wraps by whole clip lengths, so the frame after the end is
// the start, never a zero. A one-shot's last frame pairs with itself.
//
// THE RAMP ENDS ON THE TARGET. Frame f of n takes gain_from + (gain -
// gain_from) * (f + 1) / n, so the mix's last frame is at the target and the
// next mix starts from it with no step.
#include "voice.h"

#include <base/assert.h>

#include <stddef.h>

// One channel of the clip at the voice's fractional position.
static float voice_sample(const struct mixer_voice *voice,
			  const struct mixer_clip *clip, uint32_t channel)
{
	const uint64_t before = (uint64_t)voice->position;
	const uint64_t after = before + 1 < clip->frames ? before + 1 :
			       voice->loop ? 0 : before;
	const float weight = (float)(voice->position - (double)before);
	const float a = clip->samples[before * MIXER_CHANNELS + channel];
	const float b = clip->samples[after * MIXER_CHANNELS + channel];

	return a + (b - a) * weight;
}

void voe_audio_voice_sum(struct mixer_voice *voice, const struct mixer_clip *clip,
			 float *out, uint32_t frames)
{
	VOE_BASE_ASSERT(voice != NULL && clip != NULL && out != NULL,
			"a sum needs a voice, its clip and somewhere to write");
	VOE_BASE_ASSERT(clip->frames > 0, "a voice plays a clip with frames");
	const double length = (double)clip->frames;
	const float step = frames > 0 ? (voice->gain - voice->gain_from) / (float)frames : 0.0f;

	for (uint32_t f = 0; f < frames && voice->busy; f++) {
		const float gain = voice->gain_from + step * (float)(f + 1);

		for (uint32_t c = 0; c < MIXER_CHANNELS; c++)
			out[f * MIXER_CHANNELS + c] += gain * voice_sample(voice, clip, c);
		voice->position += voice->pitch;
		if (voice->position < length)
			continue;
		if (voice->loop)
			voice->position -= (double)(uint64_t)(voice->position / length) * length;
		else
			voice->busy = false;
	}
	voice->gain_from = voice->gain;
}
