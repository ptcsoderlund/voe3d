# 20 — The capsule walks by its body, jumps, falls and collects coins
folder: examples/capsule
decisions: 0168, 0239, 0249, 0253, 0254, 0250, 0246

## Change
The example's code (0249: gravity, the jump and the coin are the project's). Its systems now run
once per fixed step (card 17), with `seconds` the step. Read `game/include/game/project.h`,
`physics/include/physics/body_system.h`, `physics/overlap.h`, `physics/shape.h`,
`ecs/include/ecs/structure.h`, `math/double3.h`.

- `Code/keyboard_input.h`, `keyboard_system.c` — two more read-only fields: `jump` (Space went
  down since the last step) and `jump_down` (Space's level then, which the edge is found against).
- `Code/player.h`, `player_system.c` — `player` gains `jump_height` (m, default 1.2) and
  `gravity` (m/s², default 9.81). The system no longer moves the transform: for each entity with
  player, keyboard_input and a body, it reads the body row and submits it whole with `velocity`
  = the walk on XZ as before, and Y: on the floor, √(2·gravity·jump_height) when `jump`, else
  0; in the air the body's own Y; then less gravity × seconds. Header: Space in the air does
  nothing because only the floor branch reads `jump`.
- `Code/coin.h`, `coin_system.c` (new) — `coin` {`spin` FLOAT32, rad/s, default 2}, menu "Coin":
  each step turns the coin about Y by spin × seconds (transform intent), and when
  `voe_physics_overlap` of its own shape finds an entity with `player`, destroys the coin through
  the structural queue. Header: what a trigger is for; the query only reads.
- `Code/follow_camera_system.c` — the target's double position less the float offset
  (`from_float3`, `sub`).
- `Code/project.c` — registers coin too; runs keyboard, player, coin, follow.
- `Code/Code.md` — entries for coin; the changed ones.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/capsule` prints `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/capsule/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty and `$p/Build/editor/loaded/project-1.so` exists.
