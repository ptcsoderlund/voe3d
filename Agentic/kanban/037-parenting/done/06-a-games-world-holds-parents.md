# 06 — A game's world, and so the editor's, holds parents
folder: game
decisions: 0168, 0281, 0237

## Change
Needs card 03. The editor builds its world here too (0237), so this gives both the table; the
cook already writes ENTITY fields, so a played scene keeps its tree.

- `game/include/game/world.h`, `game/src/world.c`: register `voe_scene_parent_register` after the
  identity, room `MAX_TRANSFORMS` (every transform may have a parent); `VOE_GAME_WORLD_TYPES`
  13; "twelve" becomes "thirteen" and the list names the parent, in both headers and in
  `game/include/game/game.md` and `game/src/src.md`.
- `game/include/game/scene.h`: include `<scene/parent_component.h>`.
- `game/tests/world.c`: expects thirteen types and names `voe_scene_parent_key` through
  `game/scene.h`, as it does the others. Update its line in `game/tests/tests.md`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_game $(ninja -C build/debug -t
targets all | grep -oE "^voe_test_game_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^game/"` exits 0, `voe_test_game_world` among them.
