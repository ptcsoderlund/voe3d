# 09 — A game frame draws the lamps' shadows
folder: game
after: 08
decisions: 0168, 0325

## Change
0325 points 6 and 7.
- `game/src/frame.c`: the arena mark, then `voe_3d_draw_system_point_lights`, then
  `voe_3d_draw_system_shadows`, then the window pass, the rewind after the pass begins as now.
- `game/include/game/frame.h`: `VOE_GAME_CAPACITIES` gains `.point_shadow_size =
  VOE_3D_POINT_SHADOW_TEXELS`, one more pass, and one more object per drawn thing (the
  `2 + VOE_RENDER_SHADOW_CASCADES` factor becomes 3 +). The header's order of a frame, in a phrase.
- `game/src/src.md`: `frame.c`'s entry order.
- `game/tests/frame.c`: one more headless frame: a cube, a floor and a lamp with `cast_shadows`
  true beside them, drawn twice with no refusal. Entry in `game/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R "^game/frame$"` passes.
