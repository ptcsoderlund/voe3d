# tests

One plain C program per `audio` module, found by the build. The WAVs a test
plays are written by the test into a folder under its working directory.

- `mixer.c` — resampling, overlap, the voice limit, a missing file reported
  once, `""` quiet, the clamp at 1, a seamless loop, pitch, a ramped volume,
  stop and stale handles, the steal rule, the held sweep and a placed voice.
- `place.c` — the placement law: centred and full at the reference, the
  screen edges and behind, the falloff, the reference from the view axis.
- `sound_component.c` — the default row and menu path, add, get and the
  table, both submits, and the voice row runtime-only.
