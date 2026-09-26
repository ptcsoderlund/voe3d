# 09 — The player runs, jumps, restarts and is followed
folder: examples/coin_game
decisions: 0168, 0260, 0249, 0256, 0257, 0250, 0218

## Change
Read `examples/coin_game/Code/player.h`, `Code/game_state.h`, `Code/project.c`,
`examples/capsule/Code/keyboard_system.c`, `player_system.c`, `follow_camera_system.c`,
`physics/include/physics/body_system.h`, `scene/include/scene/camera_component.h`.

- `Code/player.h`, `Code/player_system.c` — the runtime-only `player_state` { `start`
  double3, `restarts`, `jump_down` }, registered by `player_register`.
  - `player_system_run(world, window, seconds)`, for each entity with player, body and
    transform: no state yet, adds one at its position with the current restarts. Restarts moved
    on: submits its transform at `start` (rotation kept) and the body stopped, and records the
    count. Else submits the body as the capsule's player does (W/A/S/D walk, W is −Z, Space's
    edge jumps from the floor, gravity), but off the playing phase no walk and no jump. NULL
    window reads no keys.
  - `player_camera_run(world)` — the scene's one camera placed `camera_distance` behind the
    first player along the camera's own forward, rotation kept, through the transform intent.
  Header: the keys are read here, so a Player needs nothing else; where it stands is its start.
- `Code/project.c` — before the move: game, player, rotator; after it: player_camera.
- `Code/Code.md` — entries.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty, `$p/Build/editor/loaded/project-1.so` exists, and `cmake -S $p/Build/game -B
   $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game` exits 0.
