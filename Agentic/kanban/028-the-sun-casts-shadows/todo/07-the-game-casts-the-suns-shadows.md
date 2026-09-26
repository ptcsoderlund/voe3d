# 07 — The game casts the sun's shadows
folder: game
decisions: 0168, 0258, 0254

## Change
The game Play starts draws shadows (feature steps 4, 5, 7, 8 in the game). Read
`3d/include/3d/draw_system.h` for `voe_3d_draw_system_shadows` and the pass camera's third
member (card 06).

- `game/include/game/frame.h` — `VOE_GAME_CAPACITIES`: `.shadow_size = VOE_3D_SHADOW_TEXELS`,
  `.passes = 1 + VOE_RENDER_SHADOW_CASCADES`, `.objects = VOE_GAME_WORLD_MAX_DRAWN * (1 +
  VOE_RENDER_SHADOW_CASCADES)`; the comment above it says what each extra is for. The ORDER
  paragraph names the shadow passes between the draw's opening and the window pass. Include
  `3d/shadow_cascades.h` for the constant.
- `game/src/frame.c` — after the frame is computed and the draw opened: `_shadows`, false
  treated as a refused pass is today; the window pass's camera gets `frame.shadow`.
- `game/src/src.md` — `frame.c`'s entry: the shadow passes, then the window pass.
- `game/tests/frame.c` — add: a world with a camera, a light and two shapes, one over the
  other, framed on a headless device made with `VOE_GAME_CAPACITIES`: `voe_game_frame` true
  (every pass and object fits); the same with the light gone still true. Keep its existing
  claims.
- `game/tests/tests.md` — `frame.c`'s entry grows by that claim.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder game` prints `FINDINGS: 0`.
2. `ctest --test-dir build/debug -R '^game/'` passes.
