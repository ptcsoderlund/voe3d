# audio

Sound played by path: the mixer that reads WAVs, keeps them as 48 kHz stereo
float, sums overlapping voices and pushes finished frames to the `platform`
device (ADR-0265). Effects belong here, never in a backend.

- `include` — the public headers, in `include/audio/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/audio/mixer.h` — the mixer: start a voice by path and hold its
  handle to stop, tune or move it, loop, pitch, volume, placement from a
  listener, the held sweep, mix, pump. Its header says why a stale handle is
  harmless and which voice a full mixer takes.
- `include/audio/place.h` — the listener made from a camera and the left and
  right gains a world point gets from it. Its header says why pan is screen x
  and why the balance law.
- `include/audio/sound_component.h` — a sound a thing carries at "Audio /
  Sound": path, playing, loop, volume, pitch, its replace and control intents
  and the runtime row holding its voice.
- `include/audio/sound_system.h` — the run that drains the sound intents,
  keeps a voice row per sound and, with a mixer, plays each through a
  listener from the camera; NULL mixer is the editor's case.
