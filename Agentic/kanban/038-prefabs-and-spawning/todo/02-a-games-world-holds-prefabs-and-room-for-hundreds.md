# 02 — A game's world holds prefab rows, and room for things by the hundred
folder: game
decisions: 0168, 0283, 0237

## Change
Needs card 01. 0283 points 2 and 11.

- `game/src/world.c`: register `voe_scene_prefab_register` (scene/prefab_system.h) after the
  parent, at `VOE_GAME_WORLD_AUTHORED` (only authored roots and their parts carry them).
  `MAX_ENTITIES` 4096, `MAX_TRANSFORMS` 1024 (parents take the same), `STRUCTURE_REQUESTS` 2048,
  `STRUCTURE_BYTES` 256 KiB. Header comment: fifteen types; the numbers' reasons (spawned shells
  and enemies by the hundred, 0283 point 11).
- `game/include/game/world.h`: `VOE_GAME_WORLD_TYPES` 15, `VOE_GAME_WORLD_MAX_DRAWN` 256; the
  header's list of types names the two new ones; identities stay 32 and why (spawned things
  carry none).
- `game/include/game/scene.h`: include `scene/prefab_component.h`, so a cooked scene naming a
  placed copy's prefab row compiles.
- `game/tests/world.c`: the two new keys resolve through `game/scene.h`, all different, and the
  world counts fifteen types.
- `game/include/game/game.md`, `game/src/src.md`, `game/tests/tests.md`: the counts and lists.

`VOE_GAME_WORLD_TYPES` and `_MAX_DRAWN` are read by macros in `editor/src/add_menu.h`,
`inspector.h`, `view_passes.h` and `game/include/game/frame.h`; they only grow and need no edit.
If `game/tests/frame.c` fails on device room at 256 drawn, say so in the report with the numbers
rather than lowering it.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_game $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_game_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^game/"` exits 0, and `cmake --build --preset debug --target voe_editor` exits 0.
