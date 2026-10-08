# 50 — A game's world records shape changes and has room for every sun map
folder: game
after: 49
decisions: 0168, 0389

## Change
The game's world, which the editor uses too, registers card 49's table.

- `game/src/world.c`: call `voe_3d_shape_changes_register` right after the shape registration, with the
  shape table's room. The header's count of registrations goes up by one, and it names the table.
- `game/include/game/world.h`: `VOE_GAME_WORLD_TYPES` 24. Every "twenty-three" in its comments that counts
  types rises by one. Add a phrase saying the table is runtime-only and only the shape system writes it.
- `game/include/game/frame.h`, `VOE_GAME_CAPACITIES`: a frame that relights may open one bounce sun map
  per volume per casting sun (0389). In both `.passes` and `.objects`, the bounce shadow term becomes
  `VOE_RENDER_BOUNCE_VOLUMES * VOE_RENDER_DIRECTIONAL_LIGHTS`. In `.objects` that term is the `+ 1` inside
  `(VOE_RENDER_SHADOW_CASCADES + 1)`, so split it out. The comment above says so.
- `game/tests/world.c`: the world's type count it checks rises to twenty-four. One of the types is runtime
  only and is the changes table; check that through `ecs/include/ecs/component.h`'s type walk.
- `game/tests/tests.md`: the `world.c` entry's counts match.
- `game/include/game/game.md`, `game/src/src.md`: change only where an entry counts types.

## Done when
`ctest --test-dir build/debug -R '^game/world$'` passes, counting twenty-four types, and
`grep -c VOE_RENDER_BOUNCE_VOLUMES game/include/game/frame.h` prints at least 2.
