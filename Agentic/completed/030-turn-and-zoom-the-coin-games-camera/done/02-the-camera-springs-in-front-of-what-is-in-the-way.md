# 02 — The camera springs in front of what is in the way
folder: examples/coin_game
decisions: 0168, 0261, 0253, 0250

## Change
Read `examples/coin_game/Code/player_camera.h`, `Code/player_camera_system.c`,
`physics/include/physics/overlap.h`, `physics/include/physics/shape.h`,
`physics/include/physics/collider_component.h` (the capsule kind),
`math/include/math/quat.h`.

- `Code/player_camera_system.c` — `player_camera_run` sets `arm` by the spring arm of 0261
  point 5 before placing the camera:
  - a static helper that says whether a capsule of radius 0.25 m from the first player's
    position along the rotation's +Z for a given length overlaps anything solid:
    `voe_physics_overlap` with a capsule shape (centre halfway, rotation turning local +Y onto
    the arm, `half.x` 0.25, `half.y` length / 2 + 0.25), ignoring the player's entity, 16
    contacts, blocked when any contact is not a trigger;
  - the full distance clear: the wanted length is distance; else bisect [0, distance] to
    1 cm for the longest clear length;
  - `arm` becomes the wanted length at once when shorter, else grows towards it at most
    10 m/s × the step (the step's seconds are needed: change `player_camera_run` to take
    `double seconds`).
- `Code/player_camera.h` — the new signature; header: the spring arm, what counts as in the
  way (solid colliders, not triggers, not the player), in at once and out at 10 m/s, and the
  constraint that past 16 contacts a solid one may be missed.
- `Code/project.c` — passes `step->seconds`.
- `Code/Code.md` — player_camera_system.c's entry names the spring arm.

## Done when
1. `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples/coin_game` prints
   `FINDINGS: 0`.
2. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/coin_game/. $p` and `rm -rf $p/Build`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0,
   `grep -v -e camera_turn_speed -e camera_distance_min -e camera_distance_max $p/err` prints
   nothing, `$p/Build/editor/loaded/project-1.so` exists, and `cmake -S $p/Build/game -B
   $p/Build/debug -G Ninja && cmake --build $p/Build/debug --target game` exits 0.
