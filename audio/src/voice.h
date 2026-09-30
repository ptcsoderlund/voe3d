// A clip and a voice as the mixer keeps them, and the read-and-sum of one voice
// into a mix. Internal to audio: mixer.c owns every clip and voice and calls
// voe_audio_voice_sum once per busy voice per mix.
//
//     if (voice->busy)
//             voe_audio_voice_sum(voice, &clips[voice->clip], out, frames);
//
// A clip is 48 kHz stereo float, MIXER_CHANNELS interleaved. A voice reads it
// at a fractional position that advances by its pitch each frame, and ramps
// each channel from the gain the last mix ended on (gain_from) to its target
// (gain), which mixer.c sets from volume and place before each sum.
//
// Nothing here checks bounds against the clip beyond what the position
// guarantees: mixer.c only hands in a voice whose clip is loaded and not empty.
#pragma once

#include <math/double3.h>
#include <platform/sound.h>

#include <stdint.h>

#define MIXER_CHANNELS VOE_PLATFORM_SOUND_CHANNELS

struct mixer_clip {
	const char *path;
	bool failed;
	uint64_t frames;
	const float *samples;
};

struct mixer_voice {
	bool busy;
	bool loop;
	bool held;
	// Tuned (or started) since the last sweep.
	bool touched;
	uint32_t clip;
	// Never 0 once started, so no handle is id 0.
	uint32_t generation;
	double position;
	float pitch;
	float volume;
	// Placed at where, heard through the mixer's listener when it has one.
	bool placed;
	voe_math_double3 where;
	// Per channel: the gain to reach by the end of the next mix, and the one
	// it ramps from.
	float gain[MIXER_CHANNELS];
	float gain_from[MIXER_CHANNELS];
	// The play count when this voice started; the smallest is the oldest.
	uint64_t started;
};

// Adds the voice's next frames stereo frames into out. Clears busy when a
// one-shot passes its last frame; a loop wraps.
void voe_audio_voice_sum(struct mixer_voice *voice, const struct mixer_clip *clip,
			 float *out, uint32_t frames);
