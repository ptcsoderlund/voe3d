// The mixer: sounds played by path, summed into 48 kHz stereo float frames and
// pushed to the sound device.
//
//     voe_audio_mixer *mixer = voe_audio_mixer_new(folder);  // beside the program
//     voe_audio_mixer_play(mixer, "pickup.wav");              // from any system
//     ...once a frame, on the thread that plays:
//     if (!voe_audio_mixer_pump(mixer, device)) ...           // the device went away
//     ...
//     voe_audio_mixer_destroy(mixer);
//
// EVERY SOUND GOES THROUGH HERE, IN FLOAT (ADR-0265). The device only receives
// finished samples; the voices are summed here in 32-bit float from clip to
// device, so a later effect (volume per sound, filters, reverb) is work in this
// folder and never touches an OS backend.
//
// A PLAY OVERLAPS; IT NEITHER CUTS NOR WAITS (032). Two coins taken in quick
// succession are two sounds heard together, so each play starts a voice of its
// own. With all VOE_AUDIO_VOICES busy, the voice playing longest is restarted
// with the new sound: the newest sound is the one the player just caused.
//
// A LOAD IS LAZY, AND A FAILURE IS SILENCE PLUS ONE LINE (032: a broken file is
// not a crash). The first play of a path reads, decodes and converts it to
// 48 kHz stereo (mono to both sides, the rate by linear interpolation) and
// keeps it for every later play. A file that will not read or decode is one
// error naming the path, and the path is remembered so it is never tried or
// reported again. Past VOE_AUDIO_CLIPS distinct paths, a new one is reported
// once and plays nothing.
//
// PATHS ARE RELATIVE TO THE FOLDER, NEVER ABSOLUTE (ADR-0266 point 3). The
// folder is given once at _new; a path is what a component carries, such as a
// coin's `pickup.wav`, and "" is no sound, silently.
//
// ONE THREAD, PUMPED ONCE A FRAME. No lock: play, mix and pump are called from
// the game's own loop (ADR-0266 point 5).
//
// CONSTRAINTS: a clip is refused, as a failed load, when converted it would be
// longer than VOE_AUDIO_CLIP_SECONDS; that bounds what a hostile rate or length
// can make the mixer allocate. Clips are never unloaded; the mixer's memory
// lasts until _destroy. The clip list is a linear scan by path, which
// VOE_AUDIO_CLIPS keeps short.
#pragma once

#include <platform/sound.h>

#include <stdint.h>

#define VOE_AUDIO_VOICES 16
#define VOE_AUDIO_CLIPS 32
#define VOE_AUDIO_CLIP_SECONDS 600

typedef struct voe_audio_mixer voe_audio_mixer;

// A mixer reading relative paths from folder (copied). Long-lived, with its
// own arenas; opens no device.
voe_audio_mixer *voe_audio_mixer_new(const char *folder);

void voe_audio_mixer_destroy(voe_audio_mixer *mixer);

// Starts path from its first frame on a voice of its own; see above.
void voe_audio_mixer_play(voe_audio_mixer *mixer, const char *path);

// Writes frames stereo frames to out: every voice summed and clamped to ±1,
// zeros where none plays. A voice past its end is free again.
void voe_audio_mixer_mix(voe_audio_mixer *mixer, float *out, uint32_t frames);

// Mixes as many frames as the device has room for and writes them. False when
// the write fails.
[[nodiscard]] bool voe_audio_mixer_pump(voe_audio_mixer *mixer,
					voe_platform_sound *device);
