# 24 — A world has room for 128 authored entities
folder: game
after: 18, 19
decisions: 0168, 0283, 0337

## Change
0337 point 3, the world's half. Read `game/include/game/world.h` and `game/game.md`.

- `world.h`: `VOE_GAME_WORLD_AUTHORED` is 128. Its comment keeps why it is small beside the drawn
  room and adds why 128: room for a level several times the tank game's, which filled 32 (0337),
  while still leaving drawn room for spawned things. The paragraph naming the editor's Scene panel
  still holds; nothing else in the header changes.
- `game.md`: only if its world.h entry names 32; then no number.

The editor's arrays sized by this number grow with it; card 25 counts its interface budget again.

## Done when
`grep -q 'define VOE_GAME_WORLD_AUTHORED 128' game/include/game/world.h` exits 0, and
`grep -rn 'AUTHORED 32\|stays at 32' game/` prints nothing.
