# tests

One plain C program per `audio` module, found by the build. The WAVs a test
plays are written by the test into a folder under its working directory.

- `mixer.c` — resampling, overlap, the voice limit, a missing file reported
  once, `""` quiet, the clamp at 1, a seamless loop, pitch, a ramped volume,
  stop and stale handles, the steal rule and the held sweep.
