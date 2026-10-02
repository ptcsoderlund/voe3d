# 04 — Only what casts is drawn into the sun's maps
folder: 3d
after: 02, 03
decisions: 0168, 0316, 0324

## Change
0324 points 4 and 5. Read `3d/src/draw_shadows.c`, `3d/src/draw_bounce.h`,
`3d/include/3d/draw_system.h` (the shadows call's comment, around line
413), `3d/include/3d/shape_component.h`, `3d/include/3d/model_component.h`,
`scene/include/scene/light_component.h`, `3d/tests/shadows.c`,
`3d/tests/bounce.c`, `3d/tests/bounce_scene.c`, `3d/tests/tests.md`,
`3d/src/src.md`, `3d/include/3d/3d.md`.

- `draw_shadows.c`:
  - beside `light_bounces`, a static `light_casts`: the world has one light
    row (as `light_bounces` finds it) and its `cast_shadows` is true. The
    shadows call returns true with `shadow` zeroed and no pass opened when
    it is false, with the other early outs. No light row casts nothing.
  - `voe_3d_draw_casters` skips a mesh whose entity has a shape row with
    `cast_shadows` false (the shape row the walk already fetches);
    `draw_model_casters` skips a model row with it false. The bounce map
    goes through the same walk, so it agrees with no change of its own.
  - Header points: who casts gains the two flags; the cascades need a light
    that casts, and so does the bounce, since it follows them.
- `draw_system.h`: the shadows call's comment says the cascades open only
  for a light row that casts (none for a world with no light, so not for
  the editor's preview), and that a shape or model whose flag is off is
  not a caster.
- `3d/tests/shadows.c`, `bounce.c`, `bounce_scene.c`, and any other 3d test
  that fails because its light or its shapes no longer cast: the light and
  the shape and model rows built from literals that must cast set
  `cast_shadows = true`; header comments say so.
- `3d/tests/shadows.c` gains two cases: the same world with the light's
  `cast_shadows` false leaves the floor under the cube as lit as the floor
  beside it, and with the light casting but the standing cube's shape
  `cast_shadows` false the floor under it reads lit as well, while the
  cube itself is still drawn.
- `3d/tests/tests.md` (shadows.c's entry), `3d/src/src.md`
  (draw_shadows.c's entry) and `3d/include/3d/3d.md` (draw_system.h's
  entry: who casts) where they no longer say what the file does.

## Done when
`ctest --test-dir build/debug -R '^3d/(shadows|bounce|bounce_scene)$'`
passes with both new shadows cases; on a machine without a graphics card
these skip, and building `voe_test_3d_shadows` is the proof.
