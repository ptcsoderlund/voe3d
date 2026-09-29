# 03 — Colliders have the room of drawn things
folder: game
after: 02
decisions: 0168, 0293

## Change
0293 point 6. Spawned enemies and wrecks now carry colliders, and 32 would
fill within a minute of play.

- `game/src/world.c`: the collider table is registered with
  `VOE_GAME_WORLD_MAX_DRAWN` rows; the body table keeps
  `VOE_GAME_WORLD_AUTHORED`.
- `game/include/game/world.h`: its header says colliders have the room of
  drawn things because spawned things collide (0293), bodies the authored
  room.
- `game/tests/world.c`: a fresh world takes more than
  `VOE_GAME_WORLD_AUTHORED` collider rows added through the structural queue
  (on that many new entities with transforms), and all of them are there
  after the apply. Read `game/tests/frame.c`'s header for adding a collider.
- `game/src/src.md`: the `world.c` entry, if it names the capacity.

## Done when
`ctest --test-dir build/debug -R '^game/world$'` passes, after the folder's
build.
