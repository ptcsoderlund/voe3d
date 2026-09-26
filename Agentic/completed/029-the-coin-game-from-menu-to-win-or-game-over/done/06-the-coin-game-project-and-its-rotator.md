# 06 — The coin game project and its Rotator
folder: examples/coin_game
decisions: 0168, 0251, 0244, 0239, 0256, 0259, 0260

## Change
The folder is new. Read `examples/capsule/capsule.md`, `examples/capsule/.gitignore`,
`examples/capsule/project.voe3d`, `examples/capsule/main.scene`,
`examples/capsule/Code/project.c`, `examples/capsule/Code/coin.h`,
`examples/capsule/Code/coin_system.c` (the spin), `game/include/game/project.h`.

- `examples/coin_game/coin_game.md` — the project: the coin game of 0186, levels the sponsor's
  (0251); lists `main.scene` and `Code`.
- `.gitignore`, `project.voe3d` — as the capsule's.
- `main.scene` — only the capsule's Camera and Sun entities, copied without `follow_camera`,
  numbered 1 and 2: the empty level the sponsor builds on. No card edits it again.
- `Code/Code.md` — lists the files below.
- `Code/rotator.h`, `Code/rotator_system.c` — `rotator` { `degrees_per_second` FLOAT32,
  default 90 }, menu "Rotator", capacity `VOE_GAME_WORLD_AUTHORED`; `rotator_register`;
  `rotator_system_run(world, seconds)` turns every rotator with a transform about world Y by
  degrees × seconds (radians), through the transform intent. Header: it spins any entity, only
  in the game, because project systems run only there (0241).
- `Code/project.c` — the four entry points: register rotator; before the move, rotator; after
  the move, nothing yet; the interface ends the ui frame and returns true. Header: which system
  goes in which slot, and that later cards add to the list.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty, `$p/Build/editor/loaded/project-1.so` exists, and `cmake -S $p/Build/game -B
   $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game` exits 0.
