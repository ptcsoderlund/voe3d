# 02 — The default sound device takes float frames
folder: platform
decisions: 0168, 0265, 0266

## Change
Read `platform/include/platform/library.h` first: both backends open their system library
with it, so neither the build nor a programmer's install list changes (0265).

- New `platform/include/platform/sound.h`:
  - `VOE_PLATFORM_SOUND_RATE` 48000, `VOE_PLATFORM_SOUND_CHANNELS` 2,
    `VOE_PLATFORM_SOUND_QUEUE` 2400 (frames queued at most, 50 ms).
  - Opaque `voe_platform_sound`.
  - `[[nodiscard]] voe_platform_sound *voe_platform_sound_new(void);` — the default output
    device, interleaved stereo float at 48 kHz, the system converting to the hardware.
    NULL, reported naming the library or the call, when the library or a device is missing.
  - `void voe_platform_sound_destroy(voe_platform_sound *);` — NULL is a no-op.
  - `uint32_t voe_platform_sound_room(voe_platform_sound *);` — frames it takes now without
    blocking and without holding more than the queue; recovers an underrun quietly first.
  - `[[nodiscard]] bool voe_platform_sound_write(voe_platform_sound *, const float *frames, uint32_t count);`
    — `count` ≤ the last room (asserted); false, reported, when the device went away.
  - Header points: why pushed from the caller's loop and not a callback thread (0266 point 5),
    why loaded at run time, that the device only ever receives finished samples (0265), and
    that silence is the caller's answer to NULL.
- New `platform/src/sound_wayland.c` — ALSA through `voe_platform_library_new("libasound.so.2")`:
  resolve `snd_pcm_open`, `snd_pcm_set_params`, `snd_pcm_get_params`, `snd_pcm_avail_update`,
  `snd_pcm_writei`, `snd_pcm_recover`, `snd_pcm_close`, `snd_strerror`; their prototypes and the
  few constants (`SND_PCM_STREAM_PLAYBACK`, `SND_PCM_NONBLOCK`, `SND_PCM_FORMAT_FLOAT_LE`,
  `SND_PCM_ACCESS_RW_INTERLEAVED`) declared in the file, no ALSA header. Device `"default"`,
  non-blocking, soft resample on, latency 100 ms. Room is `min(avail, QUEUE − (buffer − avail))`,
  floored at 0; a negative avail or write goes through `snd_pcm_recover`.
- New `platform/src/sound_win32.c` — WASAPI in shared mode through COM in C (`COBJMACROS`):
  `ole32.dll` loaded with the library call for `CoInitializeEx`, `CoCreateInstance`,
  `CoTaskMemFree`; the four GUIDs defined in the file, so no link change. Default render
  endpoint, `AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY`,
  48 kHz stereo float, 100 ms buffer, started at once. Room from `GetCurrentPadding` capped
  the same way; write through `GetBuffer`/`ReleaseBuffer`. Written, not verified (ADR-0130).
- New `platform/tests/sound.c` — new: NULL is accepted (say it skipped); otherwise room is at
  most the queue, writing `room` frames of silence is true, destroy. Never waits on a device.
- `platform/platform.md` — `sound.h` entry, and the opening paragraph names sound.
- `platform/src/src.md`, `platform/tests/tests.md` — entries for the three new files.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder platform` prints `FINDINGS: 0`
   (runs `platform/sound`).
2. `grep -c "asoundlib\|asound)" platform/src/sound_wayland.c cmake/voe.cmake` prints 0 for
   both files (no ALSA header, no link).
