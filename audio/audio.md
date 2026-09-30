# audio

Sound played by path: the mixer that reads WAVs, keeps them as 48 kHz stereo
float, sums overlapping voices and pushes finished frames to the `platform`
device (ADR-0265). Effects belong here, never in a backend.

- `include` — the public headers, in `include/audio/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/audio/mixer.h` — the mixer: start a voice by path and hold its
  handle to stop or tune it, loop, pitch, volume, the held sweep, mix, pump.
  Its header says why a stale handle is harmless and which voice a full mixer
  takes.
