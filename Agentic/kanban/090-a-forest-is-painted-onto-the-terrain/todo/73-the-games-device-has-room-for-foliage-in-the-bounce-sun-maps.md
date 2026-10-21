# 73 — The game's device has room for foliage in the bounce sun maps
folder: game
after: 72
decisions: 0168, 0450, 0451

## Change
Bug 04 in the shipped game and Play: card 72 draws every foliage group not
`rest` into each bounce sun map a frame relights, one object a group, so the
device needs that room or the bounce's draws are refused.

- `game/include/game/frame.h` — `VOE_GAME_CAPACITIES`' `.objects`: the
  foliage term's multiplier grows by one sun map per volume per casting
  sun, VOE_RENDER_BOUNCE_VOLUMES × VOE_RENDER_DIRECTIONAL_LIGHTS, beside
  the window pass and the cascades. The comment above it no longer says
  foliage draws in no other pass: it names the bounce sun maps (0450) and
  that captures and the point-shadow pass still draw none.

No call changes: `game/src/frame.c` already gathers the foliage before the
shadows, and the bounce reads the store the frame hands it.

## Done when
`grep -n "VOE_3D_FOLIAGE_GROUPS" -A3 game/include/game/frame.h` shows
VOE_RENDER_BOUNCE_VOLUMES in the foliage term, and the folder's tests pass.
