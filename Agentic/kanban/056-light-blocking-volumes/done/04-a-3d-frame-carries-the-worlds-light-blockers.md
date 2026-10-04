# 04 — A 3d frame carries the world's light blockers
folder: 3d
after: 01, 03
decisions: 0168, 0250, 0347

## Change
3d turns the blocker table into a pass's blockers (0347 point 2). Read the headers of
`3d/include/3d/draw_system.h` (`voe_3d_frame`, `voe_3d_draw_system_point_lights`),
`3d/src/draw_point_lights.c` as the pattern, `scene/include/scene/light_blocker_component.h`,
`scene/include/scene/transform_system.h` (`voe_scene_transform_between`),
`physics/include/physics/shape.h`, and `voe_render_light_blocker` in
`render/include/render/device.h`.

- `3d/include/3d/light_blocker.h` (new): `[[nodiscard]] bool voe_3d_light_blocker_shape(const
  voe_ecs_world *world, voe_ecs_entity entity, float lag, voe_physics_shape *out)`: the box a
  blocker is at `lag`: kind box, centre the world place in double, its rotation, half |scale| ×
  size / 2 per axis. False, `out` untouched, with no blocker row, no transform or a dead entity.
  Header points: what the box is; shared by the pass's records and the editor's lines, so both
  agree; why a physics shape (the collider lines draw it).
- `3d/include/3d/draw_system.h`: `voe_3d_frame` gains `voe_render_light_blockers blockers` after
  `points`, commented as `points` is; `voe_3d_draw_system_frame` leaves it zeroed and its list says
  so. `[[nodiscard]] bool voe_3d_draw_system_light_blockers(const voe_ecs_world *world,
  voe_3d_frame *frame, voe_base_arena *arena)`: every blocker with a transform, in table order, at
  most VOE_RENDER_LIGHT_BLOCKERS, its record about `frame->eye` in float (0250) at `frame->lag`
  from its shape: rows by device.h's formula, sphere its centre and |half|; a half of nought on any
  axis left out. Array in `arena`; no table fills none and is true. Comment: called before
  `voe_3d_draw_system_shadows`, which hands the same blockers to the bounce (card 08); every picture
  of a world makes it.
- `3d/src/light_blocker.c`, `3d/src/draw_light_blockers.c` (new, header comments): the two calls.
- `3d/tests/light_blockers.c` (new): a world with transforms, parents and blockers registered; a
  blocker's shape at a scaled, turned transform; a child of a moved parent follows it; the frame's
  record about an eye holds the eye-relative centre and not a point past its face (test the rows
  directly); no transform or no table fills none; size 0 is left out. And a GPU case modelled on
  `3d/tests/point_lights.c`'s: a lit ground shape under a sun, a blocker over part of it, drawn
  through `voe_3d_draw_system_frame`, `_light_blockers` and `_run` with `.blockers` in the pass
  camera, reads black inside and lit outside.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: entries for the new files; the draw_system.h
  entry names blockers. Each at most 300 characters.

## Done when
The tests `3d/light_blockers`, `3d/point_lights` and `3d/draw_system` pass after the folder's build.
