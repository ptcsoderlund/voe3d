# 11 — A 3d frame holds every directional light
folder: 3d
after: 10
decisions: 0168, 0357, 0349, 0350

## Change
Decision 0357 point 1 in `3d`: a frame finds the lights after the first, and the loop hands them to
the pass. Shadows and bounce for them are cards 12 and 13. Read the header of
`3d/include/3d/draw_system.h` from `voe_3d_frame` to `voe_3d_draw_system_shadows`.

- `3d/include/3d/draw_system.h`:
  - `voe_3d_frame` gains `voe_render_directional_light *more_lights; uint32_t more_count;` after
    `blockers`. Comment: _frame leaves them zeroed, `voe_3d_draw_system_lights` fills them, and
    `voe_3d_draw_system_shadows` sets their shadow records.
  - `voe_3d_draw_system_light`: no assert on more than one; it gives the first light row in table
    order. Its comment says so.
  - New `[[nodiscard]] bool voe_3d_draw_system_lights(const voe_ecs_world *world, voe_3d_frame
    *frame, voe_base_arena *arena)`. It fills the 2nd to 4th light rows in table order, each light
    made as `voe_3d_draw_system_light` makes the first. Each entry's `blockers` is the kept
    blockers' mask at its entity's place, as `sun` is for the first light. Its `bounces` and
    `bounce_strength` come from its row, and its shadow is zeroed. Rows past the fourth are left
    out. Comment points:
    - called after `voe_3d_draw_system_light_blockers` and before `voe_3d_draw_system_shadows`;
    - every picture of a world makes it;
    - always true today, as the blockers' call is.
  - New `voe_render_pass_camera voe_3d_draw_system_camera(const voe_3d_frame *frame)`: the pass
    camera from view, light, shadow, points, blockers and `more`.
  - The usage example at the top calls `_lights` and `_camera`, and the order paragraph names
    them.
- `3d/src/draw_system.c`: `voe_3d_draw_system_light` gives the first row; add `_camera`.
- `3d/src/draw_light_blockers.c`: add `voe_3d_draw_system_lights`. It shares the mask-at-a-place
  code the first light's `sun` uses, factored into one static function. Header: one point on the
  further lights.
- `3d/src/draw_bounce.c`: its `voe_render_bounce_shadow_pass_begin` call (line ~299) passes sun 0.
  Change nothing else.
- `3d/src/src.md`: the `draw_system.c` and `draw_light_blockers.c` entries.
- New `3d/tests/directional_lights.c`, built as `3d/tests/light_blockers.c` builds its world (read
  its header), with its `3d/tests/tests.md` entry. Cases, no graphics card:
  - one light gives `more_count` 0 and the light as before;
  - a sun then a moon: `light` is the sun's, and `more_lights[0]` the moon's direction, colour ×
    intensity, bounces and strength;
  - five lights keep three;
  - a Room around the moon: the moon's `blockers` has the Room's bit, and the first light's
    `sun` does not;
  - `_camera` carries `more`.

  Do not touch `3d/tests/draw_system.c`.

## Done when
`ctest --test-dir build/debug -R "^3d/(directional_lights|light_blockers|no_light|bounce)$"` passes
after the 3d build.
