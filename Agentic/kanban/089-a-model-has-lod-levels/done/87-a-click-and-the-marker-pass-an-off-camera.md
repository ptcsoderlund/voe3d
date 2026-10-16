# 87 — A click and the marker pass an off camera
folder: 3d
after: 86
decisions: 0168, 0427, 0428, 0441

## Change
0441 point 4 replaces 0428 point 8: cameras no longer ignore the switch.
An off camera is passed by a click and its marker is not drawn, as any
off entity's. The editor's pick and marks only call these two files.

`3d/src/pick.c` (494 lines; open its header comment, lines 1-12, and
`keep_nearer`, about lines 285-300): `keep_nearer` keeps nothing for an
off entity, with no camera exception; drop the camera clause and its
comment's "unless it has a camera row" sentence. If `has_store` or the
camera include is then used nowhere else in the file, remove it. The
header's "a camera aside" goes.

`3d/include/3d/pick.h` (line 102 only): "a camera aside" goes; the
comment cites 0441 beside 0428.

`3d/src/draw_marks.c` (585 lines; open only `voe_3d_draw_marks_camera`,
about lines 51-80): an off marked camera (`voe_scene_identity_off`,
`scene/identity_component.h`) draws nothing, as a zeroed entity does. Its
comment says so.

`3d/src/draw_marks.h` (open only the comment above
`voe_3d_draw_marks_camera`, about line 42): gains "an off camera draws
nothing" (0441).

`3d/tests/pick.c`: the case `a_camera_is_met_under_an_off_parent` (about
line 461) becomes `an_off_camera_is_not_met`: as now, the camera under an
off parent, but the cube behind it is met; then the parent on and the
camera's own identity off: the cube again; both on: the camera, as
`a_camera_marker_wins_over_the_cube` expects. Its comment cites 0441
point 4 in place of "CAMERAS IGNORE THE SWITCH". Rename it in `main` and
the file's header. `3d/tests/tests.md`'s `pick.c` entry, if it names the
camera exception, drops it; under 300 characters.

`3d/src/src.md`: the `pick.c` and `draw_marks.c` entries, if they name
the camera exception, drop it.

## Done when
The tests `3d/pick`, `3d/camera_marker` and `3d/draw_markers` pass after
the folder's build (`3d/draw_markers` may skip without a graphics card).
