# 09 — An editor's view lines every fog volume's box
folder: 3d
after: 08
decisions: 0168, 0347, 0365, 0428, 0493

## Change
0493 point 7: the fog fades so no box shows, so an editor's view lines each volume's box as it
lines a light blocker's: the unselected faint in the world's depth, the selected one bright
through. Read `3d/src/draw_marks.h`, in `3d/src/draw_marks.c` the two light blocker calls, in
`3d/include/3d/draw_system.h` the `light_blockers` member's comment and `voe_3d_rows_marked`, the
lines of `3d/src/draw_system.c` that zero `frame.light_blockers` and call the two blocker marks,
`voe_3d_fog_volume_shape` in `3d/include/3d/fog_volume.h` (card 07), and `3d/tests/draw_markers.c`.

- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_3d_rows_marked fog_volumes` after
  `light_blockers`, commented as it is: only an editor's view sets it; every fog volume row with a
  transform lined from `voe_3d_fog_volume_shape` at the frame's lag by
  `voe_3d_collider_marker_quads`, an off owner's left out (0428); the unselected as one geometry in
  the world's depth with the blockers', the `selected` one with the selected blocker's lines; the
  pool's room as the blockers'. `voe_3d_rows_marked`'s comment names `fog_volumes` among its users.
- `3d/src/draw_marks.h`, `3d/src/draw_marks.c`: `voe_3d_draw_marks_fog_volumes` and
  `voe_3d_draw_marks_fog_volume`, as the two blocker calls are, sharing their line-building with
  them rather than copying it where the blocker calls already have a helper for it; the header's
  list follows.
- `3d/src/draw_system.c`: `_frame` zeroes `frame.fog_volumes` beside `frame.light_blockers`;
  `_run` calls the two new marks right after the blockers' two.
- `3d/tests/draw_markers.c`: a case: a world with one fog volume and `fog_volumes` shown adds
  draws against it not shown, selecting it adds the selected draw, and an off one adds none.
- `3d/src/src.md` (`draw_marks.h`, `draw_marks.c`, `draw_system.c`), `3d/tests/tests.md`: follow,
  each at most 300 characters.

## Done when
The tests `3d/draw_markers`, `3d/light_blockers` and `3d/draw_system` pass after the folder's
build.
