# 23 — The 3d tests index fits the point lights entry
folder: 3d/tests
after: none
decisions: 0168, 0320

## Change
The `point_lights.c` entry in `3d/tests/tests.md` is 308 characters; the cap is
300 (card 15 added "falloff as authored"). Read `3d/tests/tests.md` and the
header comment of `3d/tests/point_lights.c`, nothing else.

- `3d/tests/tests.md`: shorten the `point_lights.c` entry to one sentence under
  300 characters, in the shape of its neighbours. Keep its points: the frame's
  point lights are the table's about the eye, with colour times intensity and
  the authored falloff; dark, unplaced and parented ones handled; one light
  lights the ground under it and not a far corner; the picture skips without a
  graphics card. The detail dropped stays in the file's header, which already
  carries it.
- No other file changes; do not raise the cap.

## Done when
`checks.sh --folder 3d/tests` prints no finding naming `3d/tests/tests.md`.
