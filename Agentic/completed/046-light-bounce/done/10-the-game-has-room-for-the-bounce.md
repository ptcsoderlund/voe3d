# 10 — The game has room for the bounce
folder: game
after: 09
decisions: 0168, 0308

## Change
0308 point 1: the shadows call now opens one more pass and draws every
caster once more. Read `game/include/game/frame.h` (the capacities comment
and `VOE_GAME_CAPACITIES`), the shadows call's comment in
`3d/include/3d/draw_system.h`, `game/src/frame.c` and `game/tests/frame.c`.

- `frame.h`, `VOE_GAME_CAPACITIES`: `.passes` gains one (the bounce pass);
  `.objects`' caster term counts one more pass, 2 × max drawn ×
  (2 + cascades). The comment above it says why: a bounce pass of the
  casters after the cascades, one draw each.
- `frame.c`: nothing to change, since the frame's `target` left zero is the
  window; its header comment gains a phrase that the shadows call also feeds
  the bounce.
- `game/tests/frame.c`: if a case sizes a device by hand rather than with
  `VOE_GAME_CAPACITIES`, give it the extra pass; nothing else changes.

## Done when
The test `game/frame` passes after the folder's build, and
`grep -n 'VOE_RENDER_SHADOW_CASCADES' game/include/game/frame.h` shows the
passes term with the added one.
