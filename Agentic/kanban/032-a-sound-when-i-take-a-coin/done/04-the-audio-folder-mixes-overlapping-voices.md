# 04 — The `audio` folder mixes overlapping voices
folder: audio
decisions: 0168, 0175, 0265, 0266

## Change
The folder does not exist; create it with `audio/audio.md`, `include/audio/`, `src/` with
`src.md`, `tests/` with `tests.md`, and the four-line `CMakeLists.txt`
(`voe_module(audio DEPENDS platform assets base)`). Register it (0175): its row in
`cmake/voe.cmake` after `assets`'s, `platform assets base`, with a comment naming 0265 (the
mixer is ours, effects come here, the device only receives finished samples); and
`add_subdirectory(audio)` after `assets` in the root `CMakeLists.txt`. Read
`platform/include/platform/sound.h`, `file.h`, `path.h` and `assets/include/assets/sound.h`.

- New `audio/include/audio/mixer.h`:
  - `VOE_AUDIO_VOICES` 16, `VOE_AUDIO_CLIPS` 32; opaque `voe_audio_mixer`.
  - `voe_audio_mixer *voe_audio_mixer_new(const char *folder);` — long-lived, its own arenas;
    `folder` (copied) is where relative paths are read from. Opens no device.
  - `void voe_audio_mixer_destroy(voe_audio_mixer *);`
  - `void voe_audio_mixer_play(voe_audio_mixer *, const char *path);` — `""` is a no-op; the
    first ask for a path reads `folder`/`path`, decodes it and converts it to 48 kHz stereo
    (mono to both sides, rate by linear interpolation) and keeps it; a failed read or decode
    is one `VOE_BASE_ERROR("audio", …)` naming the path, remembered so it is never retried
    or reported again; past `VOE_AUDIO_CLIPS` paths, reported once. A kept clip starts a new
    voice from its first frame; all busy, the one playing longest is restarted with it.
  - `void voe_audio_mixer_mix(voe_audio_mixer *, float *out, uint32_t frames);` — `frames`
    stereo frames: the sum of every voice, clamped to ±1, zeros where none; a voice past its
    end is free again.
  - `[[nodiscard]] bool voe_audio_mixer_pump(voe_audio_mixer *, voe_platform_sound *device);`
    — mixes the device's room and writes it; false when the write fails.
  - Header points: why every sound goes through here in float (0265: effects later, never in
    the backends), why a play overlaps rather than cuts or waits (032), why a load is lazy and
    a failure is silence plus one line (032: a broken file is not a crash), that paths are
    relative to the folder and never absolute (0266 point 3), and that the caller pumps once
    a frame from one thread.
- New `audio/src/mixer.c` — the clip list, the voices, mix and pump.
- New `audio/tests/mixer.c` — writes WAVs into a folder the test makes under its working
  directory (`platform/file.h`, `folder.h`): a mono 16-bit 24 kHz clip of N frames mixes to
  2N stereo frames, both sides equal; two plays 100 frames apart sum where they overlap and
  the first is not cut; seventeen plays leave sixteen voices; a missing path reports once
  over two plays and mixes zeros; `""` reports nothing; a loud clip played twice clamps to 1.
- `audio/audio.md` — what the folder is, its entries.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder audio` prints `FINDINGS: 0`
   (runs `audio/mixer`).
