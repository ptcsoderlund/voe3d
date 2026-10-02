# 04 — A 3d frame carries the world's point lights
folder: 3d
after: 01, 03
decisions: 0168, 0250, 0320

## Change
3d turns the point light table into a pass's lights (0320 point 6). Read the header of
`3d/include/3d/draw_system.h` and its `voe_3d_frame`, `scene/include/scene/point_light_component.h`,
`scene/include/scene/transform_system.h` (the world place at a lag), and `3d/src/pick.c`'s
`has_store` for skipping a table the world never registered.

- `3d/include/3d/draw_system.h`:
  - `voe_3d_frame` gains `voe_render_point_lights points`, with a comment: zero is none, what fills
    it, and that the caller hands it to the pass camera. `voe_3d_draw_system_frame` leaves it zeroed;
    its comment's list of what comes back zeroed names it.
  - `[[nodiscard]] bool voe_3d_draw_system_point_lights(const voe_ecs_world *world, voe_3d_frame
    *frame, voe_base_arena *arena)`: fills `frame->points` from every point light whose entity has a
    transform: position the world place at `frame->lag` about `frame->eye` in float (0250), range as
    authored, colour × `voe_scene_point_light_strength`; a strength of 0 is left out; past
    VOE_RENDER_POINT_LIGHTS the rest are left out in table order. The array is in `arena`. A world with
    no point light table fills none and returns true. False with `points` zeroed when the arena is
    full. Comment points: one call per pass's frame; the editor's views and the game both call it, so
    they agree; lights are not shadowed.
- `3d/src/draw_point_lights.c` (new, header comment): the call.
- `3d/tests/bounce_scene.c:189`, `3d/tests/draw_water.c:171`, `3d/tests/models.c:314`,
  `3d/tests/shadows.c:220`: each positional `(voe_render_pass_camera){ frame.view, frame.light,
  frame.shadow }` becomes designated, so card 03's new member does not fail -Werror. No other change.
- `3d/tests/point_lights.c` (new): a world with transforms and point lights registered; a steady light
  at a position and an eye elsewhere gives the eye-relative position, its range and colour ×
  intensity; a flashing light never flashed is left out, and is in after a flash and a system run; a
  light with no transform is left out; a light parented to a moved parent is at the parent's place
  plus its offset; a world with no point light table fills none; an arena too small returns false.
  And a GPU case modelled on `3d/tests/draw_water.c`: a lit ground shape with a sun of intensity 0
  and one point light over it, drawn through `voe_3d_draw_system_frame`, `_point_lights` and `_run`
  with `.points` in the pass camera, reads a lit pixel under the light and black away from it.
- `3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: the draw_system.h entry names point lights; one
  entry each for the new files. Each at most 300 characters.

## Done when
The tests `3d/point_lights`, `3d/bounce_scene`, `3d/draw_water`, `3d/models` and `3d/shadows` pass
after the folder's build.
