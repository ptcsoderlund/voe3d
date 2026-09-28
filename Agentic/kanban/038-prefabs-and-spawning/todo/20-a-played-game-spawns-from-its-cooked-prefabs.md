# 20 — A played game spawns from its cooked prefabs
folder: game
decisions: 0168, 0283, 0237

## Change
Needs card 19. 0283 point 10.

- `game/src/run.c`: hands `&voe_game_prefabs_cooked` (game/prefabs.h) to `voe_game_steps_run`
  in place of card 08's NULL.
- `game/include/game/run.h`: the "links only in a tree that has…" constraint names `prefabs.c`
  defining `voe_game_prefabs_cooked` beside `scene.c`; the order paragraph says the steps get the
  cooked prefabs.
- `game/include/game/game.md`, `game/src/src.md`: the entries for `run.h` and `run.c`.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_game $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_game_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^game/"` exits 0, and the hand-made tree block under `## Done when` in
`Agentic/kanban/038-prefabs-and-spawning/done/19-the-game-build-compiles-the-cooked-prefabs.md`
exits 0 again with this `run.c`. The human's: Play in `examples/tank_game` starts the game.
