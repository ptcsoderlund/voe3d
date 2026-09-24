# 11 — 3d frames a scene with no light as unshaded
folder: 3d
decisions: 0168, 0238, 0218

## Change
`3d` owns turning the scene's light into `render`'s shape, and it is where the assert lives. Make
that one public function, answering "unshaded" for a world with no light, and have the frame use it.
More than one light still asserts; exactly one camera still asserts (0218).

- `3d/include/3d/draw_system.h` — add
  `voe_render_light voe_3d_draw_system_light(const voe_ecs_world *world);`: the world's one light in
  render's shape; with none, a light with `unshaded` set and the rest zero; asserts on more than
  one. Its comment says why this is public (the editor's own views and dev's monitor are lit with
  it, so every picture of a scene means the same thing by "no light"). Rewrite the "AND EXACTLY ONE
  LIGHT" paragraph and the `voe_3d_draw_system_frame` summary line: at most one light; none draws
  every surface in its material colour, unshaded (0238), instead of asserting; more than one still
  asserts because render has one sun.
- `3d/src/draw_system.c` — the static `the_sun` becomes `voe_3d_draw_system_light`, handling the
  empty table; `voe_3d_draw_system_frame` drops its `count == 1` light assert and calls it.
  The file's header comment, if it names the light assert, follows.
- `3d/tests/no_light.c` — new, needs no GPU (`voe_3d_draw_system_frame` takes no device). Build the
  world with the scene folder's register calls (`scene/camera_system.h`, `scene/light_system.h`,
  `scene/transform_system.h` headers; register the light table with no rows):
  1. a camera with a transform and no light: `voe_3d_draw_system_frame` returns, `frame.light.unshaded`
     is non-zero, `frame.blind` is false;
  2. `voe_3d_draw_system_light` on that world gives the same;
  3. after adding one light, `unshaded` is 0 and direction, intensity and colour are the light's.
  Its header says what it claims.
- `3d/tests/tests.md` — an entry for `no_light.c`.
- `3d/3d.md` — the `draw_system.h` entry mentions the scene's light in render's shape, unshaded when
  there is none.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder 3d` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^3d/no_light'` runs one test and it passes.
