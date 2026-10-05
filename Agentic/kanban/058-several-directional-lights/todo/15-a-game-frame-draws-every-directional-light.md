# 15 — A game frame draws every directional light
folder: game
after: 13
decisions: 0168, 0357, 0349

## Change
The game's frame lights by every light (decision 0357 point 1), and its device has room for them.

- `game/src/frame.c` (lines ~104–118):
  - after `voe_3d_draw_system_light_blockers`, call `voe_3d_draw_system_lights(world, &frame,
    scratch)`, cast to void, with a comment as its neighbours have;
  - build the pass camera with `voe_3d_draw_system_camera(&frame)`.

  Read the header of `3d/include/3d/draw_system.h` for both. The file's header names the further
  lights' passes.
- `game/include/game/frame.h`:
  - `VOE_GAME_CAPACITIES`: `passes` take `VOE_RENDER_SHADOW_CASCADES ×
    VOE_RENDER_DIRECTIONAL_LIGHTS` cascades and `VOE_RENDER_DIRECTIONAL_LIGHTS` bounce shadow passes
    in place of 4 and 1, and `objects` the same per caster.
  - The comment above it says so.
  - The constraint at line ~33 says at most `VOE_RENDER_DIRECTIONAL_LIGHTS` lights are drawn, in
    table order.
- `game/src/src.md`: the `frame.c` entry.
- `game/tests/frame.c`: a new case, THE SUN AND MOON CASE, in its header. It is built as the shadow
  case is: a sun and a moon from opposite sides, both casting and bouncing once, over the cube and
  the capsule. Three frames each come back true with `VOE_GAME_CAPACITIES`: the first asks for the
  second light's maps, the second draws with them, and the third relights.
- `game/tests/tests.md`: the `frame.c` entry names it.

## Done when
`ctest --test-dir build/debug -R "^game/frame$"` passes after the game build, and `grep -q
voe_3d_draw_system_camera game/src/frame.c` exits 0.
