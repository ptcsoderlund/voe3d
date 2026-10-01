# 09 — The sun feeds the bounce
folder: 3d
after: 08
decisions: 0168, 0307, 0308

## Change
0308 points 1, 4 and 7. Read `3d/include/3d/draw_system.h` (the frame and
the shadows call), `3d/src/draw_shadows.c`, `3d/src/draw_group.h`,
`3d/include/3d/bounce_grid.h`, the bounce calls in
`render/include/render/device.h`, the `voe_scene_transform_between`
comment in `scene/include/scene/transform_system.h`, `3d/src/src.md` and
`3d/tests/shadows.c` and `3d/tests/tests.md`.

- `draw_system.h`: `voe_3d_frame` gains `voe_render_target target`, the
  target the frame draws to; zero (`VOE_RENDER_TARGET_WINDOW`) is the window,
  so a caller that sets nothing is unchanged. `VOE_3D_BOUNCE_REACH` 6.0f.
  The shadows call's comment points: it also opens one bounce pass after the
  cascades, draws the casters in it, and updates the frame's target's grid;
  so a caller's `passes` needs 1 + cascades + 1 and its `objects` one more
  draw per caster; false as before when any call fails; stale spheres are
  marked where a caster moved this step, so a world without a previous table
  marks none and the grid catches up by its cycle.
- `3d/src/draw_bounce.h` / `draw_bounce.c`, new, internal, header comments:
  - `uint32_t voe_3d_bounce_stale(const voe_ecs_world *world, const
    voe_3d_frame *frame, voe_math_float4 *spheres, uint32_t room)` — for
    each caster (the cascades' rule) whose `between` at lag 1 differs from
    lag 0 in position or rotation, a sphere of `VOE_3D_BOUNCE_REACH` at each
    of the two places, about the frame's eye; returns the count, never past
    `room`.
  - `[[nodiscard]] bool voe_3d_draw_bounce(voe_ecs_world *world,
    voe_render_device *device, voe_3d_frame *frame)` — fit the grid with
    `voe_3d_bounce_grid_fit` from the frame's view, eye and the sun's
    direction; open the bounce pass with its light view and the frame's
    light; draw the casters as a cascade does (share `draw_shadows.c`'s
    caster walk through `draw_group.h` or a declaration here, not a copy);
    the stale spheres in the frame's arena; `voe_render_bounce_update` for
    `frame->target`.
- `draw_shadows.c`: calls `voe_3d_draw_bounce` after the cascades, only when
  it drew cascades (no sun, 0287: neither); header comment gains a point.
- `3d/tests/bounce.c`, new, headless (the shape of `shadows.c`, includes
  `"../src/draw_bounce.h"`): a world with a sun, grey ground and a red wall;
  cases:
  - the shadows call is true with passes 5 and false with passes 4;
  - with no previous table `voe_3d_bounce_stale` gives 0;
  - with the previous table, the wall remembered and then moved 1 m, it
    gives 2, centred on the old and new places; an unmoved wall gives 0;
  - `room` 1 gives 1.
- `3d/src/src.md`, `3d/include/3d/3d.md`, `3d/tests/tests.md`: entries
  changed or added.

## Done when
The test `3d/bounce` passes, and `3d/shadows`, `3d/draw_water`,
`3d/draw_system` and `3d/no_light` still pass, after the folder's build.
