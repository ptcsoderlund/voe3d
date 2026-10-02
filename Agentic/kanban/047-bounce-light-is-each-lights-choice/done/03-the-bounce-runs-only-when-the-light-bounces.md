# 03 — The bounce runs only when the light bounces
folder: 3d
after: 01
decisions: 0168, 0316, 0319

## Change
0319 point 3. Read `3d/include/3d/draw_system.h` (the frame struct,
`voe_3d_draw_system_light` and `voe_3d_draw_system_shadows`),
`3d/src/draw_shadows.c`, `3d/src/draw_bounce.h`, the header comment of
`3d/src/draw_system.c`, `3d/tests/bounce.c`, `3d/tests/bounce_scene.c`,
`3d/tests/tests.md`, `3d/src/src.md`, `3d/include/3d/3d.md`, and
`scene/include/scene/light_component.h`.

- `draw_shadows.c`: after the cascades, `voe_3d_draw_bounce` is called only
  when the world's light, the row `voe_3d_draw_system_light` reads, has
  `bounces` of 1 or more; otherwise no bounce pass, no draws, no update,
  and the call is true. A world with no light never bounces. If finding
  that row is a static in `draw_system.c`, declare it in `draw_bounce.h`
  as internal and define it once. The file's header point on the bounce
  pass says when it runs.
- `draw_system.h`: the shadows call's bounce paragraph says the pass and
  update run only for a light with bounces; the `1 + cascades + 1` passes
  and the extra draw per caster are needed only then.
- `3d/tests/bounce.c`: its sun gets `bounces = 1`, so its five-and-four
  pass checks and stale spheres hold as now; a new check that the same
  world with bounces 0 is true on a device with room for four shadow
  passes and no more, with `frame.shadow` set. Header comment updated.
- `3d/tests/bounce_scene.c`: its sun gets `bounces = 1`; header says so.
- Any other 3d test that fails because the bounce no longer runs at
  bounces 0 sets `bounces = 1` on its light where it reads the bounce.
- `3d/tests/tests.md` (bounce.c's entry), `3d/src/src.md` (draw_shadows.c's
  and draw_bounce.c's entries) and `3d/include/3d/3d.md` (draw_system.h's
  entry: the bounce pass only when the sun bounces).

## Done when
`ctest --test-dir build/debug -R '^3d/(bounce|bounce_scene|shadows)$'`
passes with the bounces-0 check in `3d/bounce`; on a machine without a
graphics card these skip, and the build of `voe_test_3d_bounce` is the proof.

The human, in the editor, walks `## How to test` in `feature.md`, steps
1 to 6: the Bounces dropdown at 0 on a new scene and in
`examples/tank_game`, even ground at 0, the tint at 1 and gone at 0, undo
and redo, saved 1 surviving a reopen, and Play agreeing with the editor.
