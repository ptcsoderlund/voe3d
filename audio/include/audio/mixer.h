// The mixer: sounds played by path, summed into 48 kHz stereo float frames and
// pushed to the sound device.
//
//     voe_audio_mixer *mixer = voe_audio_mixer_new(folder);  // beside the program
//     voe_audio_mixer_play(mixer, "pickup.wav");              // a one-shot
//     voe_audio_voice hum = voe_audio_mixer_start(mixer, (voe_audio_start){
//             .path = "hum.wav", .loop = true, .volume = 1, .pitch = 1 });
//     voe_audio_mixer_tune(mixer, hum, 0.5f, 1.2f);          // while it plays
//     ...once a frame, on the thread that plays:
//     if (!voe_audio_mixer_pump(mixer, device)) ...           // the device went away
//     voe_audio_mixer_destroy(mixer);
//
// EVERY SOUND GOES THROUGH HERE, IN FLOAT (ADR-0265). The device only receives
// finished samples; effects are work in this folder, never in an OS backend.
//
// A VOICE HAS A HANDLE (ADR-0304). Its id is slot and generation, 0 none; a
// voice that ended or was stolen leaves its handle stale, and every call on a
// stale handle is a no-op (false for _playing), so holding one is never a crash.
//
// LOOP, PITCH, VOLUME. A loop wraps with no seam: its last frame interpolates
// into its first. Pitch is the read rate, linearly interpolated, clamped to
// [0.25, 4]; volume is a gain clamped to [0, 4]. A tuned gain ramps linearly
// across the next mix call, so a change every step never clicks.
//
// A PLAY OVERLAPS; IT NEITHER CUTS NOR WAITS (032). Each start takes a voice of
// its own. With all VOE_AUDIO_VOICES busy it takes the oldest voice that does
// not loop: the newest sound is the one the player just caused. When every
// voice loops, the start is refused and returns 0.
//
// HELD VOICES ARE SWEPT. A voice started `held` must be tuned or moved between
// two _sweep calls or that sweep stops it, so a sound whose owner is gone ends.
//
// A VOICE MAY BE PLACED at a world point, heard from the listener _listen set
// by the law in audio/place.h; it follows each _move, the new gains ramping in
// as a volume does. With no listener, a placed voice plays unplaced.
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
// ONE THREAD, PUMPED ONCE A FRAME. No lock: start, mix and pump are called from
// the game's own loop (ADR-0266 point 5).
//
// CONSTRAINTS: a clip longer than VOE_AUDIO_CLIP_SECONDS converted is refused
// as a failed load, bounding what a hostile file can make us allocate. Clips
// are never unloaded; the clip list is a linear scan VOE_AUDIO_CLIPS keeps short.
#pragma once

#include <audio/place.h>
#include <platform/sound.h>

#include <stdint.h>

#define VOE_AUDIO_VOICES 32
#define VOE_AUDIO_CLIPS 32
#define VOE_AUDIO_CLIP_SECONDS 600

typedef struct voe_audio_mixer voe_audio_mixer;

// A playing sound; id 0 is none. See "A VOICE HAS A HANDLE" above.
typedef struct {
	uint32_t id;
} voe_audio_voice;

// What a start plays and how.
typedef struct {
	const char *path;
	bool loop;
	bool held;
	float volume;
	float pitch;
	bool placed;
	voe_math_double3 where;
} voe_audio_start;

// A mixer reading relative paths from folder (copied). Long-lived, with its
// own arenas; opens no device.
voe_audio_mixer *voe_audio_mixer_new(const char *folder);

void voe_audio_mixer_destroy(voe_audio_mixer *mixer);

// Starts a voice from the clip's first frame. 0 for "", a clip that failed,
// or every voice looping.
voe_audio_voice voe_audio_mixer_start(voe_audio_mixer *mixer, voe_audio_start start);

// A one-shot of path: no loop, not held, volume 1, pitch 1.
void voe_audio_mixer_play(voe_audio_mixer *mixer, const char *path);

// A one-shot of path placed at where.
voe_audio_voice voe_audio_mixer_play_at(voe_audio_mixer *mixer, const char *path,
					voe_math_double3 where);

// Hears placed voices from listener (copied); NULL is none.
void voe_audio_mixer_listen(voe_audio_mixer *mixer, const voe_audio_listener *listener);

// Places the voice at where and marks a held voice kept at the next sweep.
void voe_audio_mixer_move(voe_audio_mixer *mixer, voe_audio_voice voice,
			  voe_math_double3 where);

// Ends the voice now; it is silent from the next mix.
void voe_audio_mixer_stop(voe_audio_mixer *mixer, voe_audio_voice voice);

bool voe_audio_mixer_playing(const voe_audio_mixer *mixer, voe_audio_voice voice);

// Sets volume and pitch (clamped) and marks a held voice kept at the next sweep.
void voe_audio_mixer_tune(voe_audio_mixer *mixer, voe_audio_voice voice,
			  float volume, float pitch);

// Stops every held voice not tuned or moved since the last sweep, then clears
// the marks.
void voe_audio_mixer_sweep(voe_audio_mixer *mixer);

// Writes frames stereo frames to out: every voice summed and clamped to ±1,
// zeros where none plays. A voice past its end is free again.
void voe_audio_mixer_mix(voe_audio_mixer *mixer, float *out, uint32_t frames);

// Mixes as many frames as the device has room for and writes them. False when
// the write fails.
[[nodiscard]] bool voe_audio_mixer_pump(voe_audio_mixer *mixer,
					voe_platform_sound *device);
