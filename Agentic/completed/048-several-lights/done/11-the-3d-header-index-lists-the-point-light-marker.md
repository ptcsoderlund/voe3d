# 11 — The 3d header index lists the point light marker
folder: 3d/include/3d
after: none
decisions: 0168, 0320

## Change
`3d/include/3d/3d.md` has no entry for `point_light_marker.h`; the folder check
flags it. Read the header comment of `3d/include/3d/point_light_marker.h` and
`3d/include/3d/3d.md`, nothing else.

- `3d/include/3d/3d.md`: add one entry for `point_light_marker.h`, placed after
  the `sun_marker.h` entry, in the shape of its neighbours (one sentence, under
  300 characters). Its points: a point light drawn in an editor's view as three
  wire circles about its position, in line quads about the eye; and the ray that
  picks it (a cube about the position).
- No other file changes.

## Done when
`checks.sh --folder 3d/include/3d` prints no finding naming
`3d/include/3d/3d.md`.
