# 08 — The tank game keeps the level's width in view
folder: examples
after: 07
decisions: 0168, 0291, 0272, 0261
read: feature.md

## Change
0291 point 5, and the last card of 039. Files under `examples/tank_game/`: new
`Code/tank_camera.h` and `Code/tank_camera_system.c`; `Code/project.c`, `Code/Code.md`,
`tank_game.md`. Read `examples/coin_game/Code/player_camera.h` for a runtime-only row on the
scene's camera, and `scene/include/scene/camera_system.h` for the whole-lens intent and what it
refuses. Never edit `main.scene` (0272).

- `tank_camera.h`: the runtime-only `tank_camera_fit` row, capacity 1, no menu: the camera's
  authored `fov_y`. A register call and a run call taking the step. Header points: why 16:9 is
  the level's width (the framing the sponsor saw), what the lens is below and above 16:9, the
  3.0 rad cap, nothing headless, one writer.
- `tank_camera_system.c`: on the first step with a window, add the row to the entity holding the
  one camera through the structural queue, holding its `fov_y`. Each step with the row and a
  window of non-zero size: aspect `a` from `voe_platform_window_size(step->window)`; `fov_y` is
  the authored one when `a >= 16/9`, otherwise `2·atan(tan(authored/2)·(16/9)/a)`, capped at
  3.0; submit the whole lens (near and far kept) only when it differs from the current one.
- `project.c`: register it; run it last before the move.
- `Code.md` and `tank_game.md`: the two files, and the camera keeping the level's width.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0 and
`grep -q tank_camera examples/tank_game/Code/project.c` exits 0.

For the human, `## How to test` in `feature.md`, all five steps, in the editor on
`examples/tank_game`: the settings are the Project button's panel (1); drag the corner tall and
wide (2); Fullscreen and Play, then Stop (3); Ship and run `Build/ship/tank_game/tank_game` (4);
reopen the editor (5). Commit the `project.voe3d` the steps change only if the sponsor asks.
