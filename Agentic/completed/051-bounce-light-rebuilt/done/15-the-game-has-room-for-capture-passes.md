# 15 — The game's device has room for the capture passes
folder: game
after: 14
decisions: 0168, 0326

## Change
0326 points 3 and 8: the bounce map pass is gone and up to `VOE_RENDER_BOUNCE_CAPTURE_PASSES`
capture passes come after the point-shadow pass, each drawing every caster at most once.
- `game/include/game/frame.h`: `VOE_GAME_CAPACITIES`' `.passes` is the window pass, the
  cascades, the point-shadow pass and `VOE_RENDER_BOUNCE_CAPTURE_PASSES`; `.objects`' factor
  `(3 + VOE_RENDER_SHADOW_CASCADES)` becomes `(2 + VOE_RENDER_SHADOW_CASCADES +
  VOE_RENDER_BOUNCE_CAPTURE_PASSES)`. The comment above it: the bounce map pass goes, the
  capture passes come, in a phrase each.
- `game/src/frame.c`: the header's order of a frame: the shadow passes, then the bounce's
  capture passes and relight when a light bounces (no longer "the sun's, which also feed the
  bounce").

## Done when
`ctest --test-dir build/debug -R "^game/frame$"` passes and
`grep -q "VOE_RENDER_BOUNCE_CAPTURE_PASSES" game/include/game/frame.h` exits 0.
