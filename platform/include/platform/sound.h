// The default sound output device, taking interleaved stereo float frames at
// 48 kHz; the system converts them to whatever the hardware wants.
//
//     voe_platform_sound *sound = voe_platform_sound_new();   // NULL: play nothing
//     ...once a frame:
//     uint32_t room = voe_platform_sound_room(sound);
//     mix room frames into a float buffer of room * VOE_PLATFORM_SOUND_CHANNELS;
//     if (!voe_platform_sound_write(sound, buffer, room)) ...  // the device went away
//     ...
//     voe_platform_sound_destroy(sound);
//
// PUSHED FROM THE CALLER'S LOOP, NOT PULLED BY A CALLBACK THREAD (ADR-0266
// point 5). The game fills the device once a frame from its own loop, so there
// is no audio thread and nothing to lock. The price is latency: room never lets
// more than VOE_PLATFORM_SOUND_QUEUE frames (50 ms) sit queued, which is enough
// to ride over one slow frame and short enough that a coin sounds when taken.
// A frame slower than that underruns; the next room recovers it quietly.
//
// LOADED AT RUN TIME. Linux speaks ALSA through platform/library.h rather than
// linking libasound, so building needs no audio package and a machine without
// ALSA or a sound device still runs (ADR-0265); Windows loads ole32 the same
// way, so the build links nothing new.
//
// THE DEVICE ONLY EVER RECEIVES FINISHED SAMPLES (ADR-0265). Mixing, volume and
// any effect happen in the caller (the audio folder) before a write; nothing
// here knows what a sound is.
//
// NULL FROM _new IS NOT AN ERROR TO HANDLE: it is reported once, naming the
// library or the call that failed, and the caller's answer is silence.
#pragma once

#include <stdint.h>

#define VOE_PLATFORM_SOUND_RATE 48000
#define VOE_PLATFORM_SOUND_CHANNELS 2
// Frames queued at most: 50 ms at VOE_PLATFORM_SOUND_RATE.
#define VOE_PLATFORM_SOUND_QUEUE 2400

typedef struct voe_platform_sound voe_platform_sound;

// The default output device, open and ready for frames. NULL, reported, when
// the system library or a device is missing.
[[nodiscard]] voe_platform_sound *voe_platform_sound_new(void);

// NULL is a no-op.
void voe_platform_sound_destroy(voe_platform_sound *sound);

// Frames the device takes now without blocking and without holding more than
// VOE_PLATFORM_SOUND_QUEUE; recovers an underrun quietly first. 0 is ordinary.
uint32_t voe_platform_sound_room(voe_platform_sound *sound);

// Writes count interleaved stereo frames; count is at most the last room
// (asserted). False, reported, when the device went away.
[[nodiscard]] bool voe_platform_sound_write(voe_platform_sound *sound, const float *frames,
					    uint32_t count);
