# 03 — A pass marks every place with no mesh
folder: 3d
after: 02
decisions: 0168, 0354, 0365

## Change
Additive: no caller changes, a zeroed field marks nothing.

`3d/include/3d/draw_system.h` (public; do not split):
- New `voe_3d_rows_marked`, beside `voe_3d_point_lights_marked` and with
  its fields (shown, selected, material, colour, selected_colour, pixels,
  size), its comment saying it is "every row of a kind, one selected" and
  that the frame's `places` and (next card) `light_blockers` use it (0365
  point 6).
- `voe_3d_frame` gains `voe_3d_rows_marked places;` after `point_lights`:
  when `shown`, every entity `voe_3d_place_marker_wanted`
  (`3d/include/3d/place_marker.h`) is marked with
  `voe_3d_place_marker_quads` in the world layer inside the world's depth,
  the selected one alone in `selected_colour`, the rest as one geometry in
  `colour`; the pool needs `VOE_3D_PLACE_MARKER_VERTICES` and `_INDICES` per
  marked entity, two ranges, two objects; a pool too small draws none.
  Only an editor's view sets it.
- `#include <3d/place_marker.h>`; `voe_3d_draw_system_frame`'s comment
  lists `places` among the fields that come back zeroed; `_run`'s comment
  says `frame.places` is drawn with the world after the point lights.

`3d/src/draw_marks.h`: `void voe_3d_draw_marks_places(world, device,
arena, frame)`, in the order list after point lights; header points
updated.

`3d/src/draw_marks.c`: the function, as `voe_3d_draw_marks_point_lights`
is, walking the transform table's owners
(`scene/include/scene/transform_component.h`) and skipping those not
wanted; top comment lists the places.

`3d/src/draw_system.c`: `_frame` zeroes `places` as it does the other
marks; `_run` calls `voe_3d_draw_marks_places` right after
`voe_3d_draw_marks_point_lights`.

`3d/tests/draw_markers.c`: new test: a world with a camera, a light and
one bare transform in view; with `places` shown it draws one more draw than
zeroed; with that entity selected, two more (rest geometry empty draws
none, so count what the code does and state it); with a shape added to the
entity, none more. Reuse `draws_with_a_marker`'s way of counting (widen it
or add a sibling).

Maps: `3d/src/src.md`'s `draw_marks.c` and `draw_marks.h` entries and
`3d/tests/tests.md`'s `draw_markers.c` entry name the places; `3d/3d.md`'s
`draw_system.h` entry mentions the place markers.

## Done when
`ctest --test-dir build/debug -R '^3d/draw_markers$'` passes after a build
(it skips without a graphics card; on this machine it runs).
