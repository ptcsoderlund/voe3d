# audio

Sound played by path: the mixer that reads WAVs, keeps them as 48 kHz stereo
float, sums overlapping voices and pushes finished frames to the `platform`
device (ADR-0265). Effects belong here, never in a backend.

- `include` — the public headers, in `include/audio/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/audio/mixer.h` — the mixer: play by path, mix, pump. Its header
  says why everything goes through it in float, why a play overlaps, and why a
  broken file is silence plus one line.
