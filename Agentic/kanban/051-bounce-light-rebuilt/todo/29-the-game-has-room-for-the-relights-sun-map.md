# 29 — The game's device has room for the relight's sun map
folder: game
after: 28
decisions: 0168, 0329

## Change
0329 point 3, as card 15 did for the capture passes: on a frame that relights a casting sun, one
more pass after the capture passes, drawing every caster at most once.
- `game/include/game/frame.h`: in `VOE_GAME_CAPACITIES`, `.passes` gains one for the bounce shadow
  pass, and `.objects`' factor `(2 + VOE_RENDER_SHADOW_CASCADES + VOE_RENDER_BOUNCE_CAPTURE_PASSES)`
  gains one. The comment above it names that pass in a phrase, beside the capture passes.
- `game/src/frame.c`: the header's order of a frame: the capture passes, the bounce's sun map when
  it relights, then the relight.

## Done when
`ctest --test-dir build/debug -R "^game/frame$"` passes and
`grep -qi "bounce shadow\|sun map" game/include/game/frame.h` exits 0.
