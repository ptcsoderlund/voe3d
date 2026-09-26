# 01 — The camera turns and zooms while playing
folder: examples/coin_game
decisions: 0168, 0261, 0260, 0256, 0259, 0218

## Change
Read `examples/coin_game/Code/player.h`, `Code/player_system.c`, `Code/game_state.h`,
`Code/project.c`, `Code/Code.md`, `game/include/game/project.h`,
`platform/include/platform/input.h`, `math/include/math/quat.h`,
`scene/include/scene/transform_system.h`.

- `Code/player.h`, `Code/player_system.c` — Player gains `camera_turn_speed`,
  `camera_distance_min`, `camera_distance_max` (FLOAT32, defaults 0.25, 2, 12, 0261 point 1);
  `player_camera_run` and its declaration leave these files. Header: `camera_distance` is now
  where the distance starts; the new numbers and their units.
- `Code/player_camera.h` (new) — the runtime-only `player_camera_state` { `yaw`, `pitch`,
  `distance`, `arm` (float), `lock_asked` (bool) } and its key; `[[nodiscard]] bool
  player_camera_register(voe_ecs_world *world)`; `void player_camera_run(voe_ecs_world
  *world)`; `void player_camera_look(const voe_game_project_frame *frame)`. Header: where the
  row lives and its one writer (this module), why the mouse is read once a frame and the
  camera placed in a step, the pitch range and that it never flips.
- `Code/player_camera_system.c` (new) — register (capacity 1, runtime-only, no menu, marker
  taken by address as player_system.c does). `player_camera_run`, after the move: no row on
  the scene's camera yet, adds one through the structural queue with yaw and pitch from the
  camera's rotation (0261 point 2) and distance = the first player's `camera_distance`
  clamped to [min, max], arm = distance. Else submits the camera's transform: rotation yaw
  about +Y then pitch about +X, position the first player's position plus that rotation's +Z
  times `arm` (for now arm = distance).
- `Code/player_camera_look.c` (new) — `player_camera_look`: no window, no row or no
  game_state, nothing. Wants the lock when the phase is playing and the right button is down;
  calls `voe_platform_input_lock_pointer` only when that differs from `lock_asked`, and
  records it. While wanted, motion × `camera_turn_speed` (degrees → radians) turns yaw (x)
  and pitch (−y, mouse up looks up); pitch clamped to [−80°, −5°], yaw wrapped to (−π, π].
  While playing, the wheel's y scales distance by 1.15 per notch (towards the person farther),
  clamped to [min, max]. Writes its own row directly.
- `Code/project.c` — register calls `player_camera_register`; after the move calls
  `player_camera_run`; the interface calls `player_camera_look` before `game_interface_run`.
  Header: the new calls.
- `Code/Code.md` — the three new files; player.h's and player_system.c's entries lose the
  camera.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0,
   `grep -v -e camera_turn_speed -e camera_distance_min -e camera_distance_max $p/err` prints
   nothing (the sponsor's scene lacks the new fields, 0251), `$p/Build/editor/loaded/project-1.so`
   exists, and `cmake -S $p/Build/game -B $p/Build/debug -G Ninja && cmake --build
   $p/Build/debug --target game` exits 0.
