# 08 — The camera marker is picked and drawn
folder: 3d
decisions: 0168, 0203, 0205, 0223

## Change
Card 07 made `3d/include/3d/camera_marker.h`; this puts it in the pick and the draw.

- `3d/include/3d/pick.h` / `3d/src/pick.c` — `voe_3d_pick` walks the world's camera store beside its shapes:
  an entity with a camera and a transform is tested with `voe_3d_camera_marker_hit` and competes on distance
  with the shapes, nearest wins. A world with no camera store registered walks none. Header: the paragraph
  on what is pickable names the camera's box and says the frustum is not hit (0223).
- `3d/include/3d/draw_system.h` — a record `voe_3d_camera_marked`, in the shape of `voe_3d_outlined` /
  `voe_3d_gizmoed`: `entity` (the camera entity, zeroed for none), `material` (an unlit record, as the
  outline's), `colour`, `pixels`, `size`. `voe_3d_frame` gains `voe_3d_camera_marked marker`;
  `voe_3d_draw_system_frame` leaves it zeroed, as it does `outlined` and `gizmo`. The field's comment says:
  drawn in the world layer inside the world's depth, not behind the outline's clear; a zeroed record draws
  nothing; one entity for the reason `outlined` is one; a transient pool too small is a capacity chosen too
  small and draws nothing, as the outline does.
- `3d/src/draw_system.c` — `voe_3d_draw_system_run`, in the world layer after the shapes and before the
  outline's depth clear: when `frame.marker.entity` is alive and has a camera and a transform, build the
  quads with `voe_3d_camera_marker_quads` from `frame.view` and draw them as one transient range and one
  object with identity matrices, the way the outline's quads are drawn. A pass whose `frame.view` is the
  marked camera's own view is the caller's to avoid (card 13 never sets a marker there). Keep the file under
  800 lines: the marker's draw is one static helper; if the file would pass 800, that helper goes in
  `3d/src/camera_marker.c` behind a declaration in a private `3d/src/camera_marker_draw.h` with its header.
- `3d/tests/pick.c` — new tests: a camera entity in front of a cube is picked; the same camera behind the
  cube loses to the cube; a ray through the frustum's far corner, outside the box, picks nothing.
- `3d/tests/draw_system.c` — new tests, in the shape of the existing outline tests: a frame with a marker on
  a live camera draws one more object than the same frame without; a marker on a zeroed entity draws the
  same as none.
- `3d/src/src.md`, `3d/tests/tests.md` — entries that change.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `3d` exits 0, and `wc -l < 3d/src/draw_system.c` prints a
number under 800.
