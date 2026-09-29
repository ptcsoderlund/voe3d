# 21 — The tank fires shells that fly and vanish
folder: examples
decisions: 0168, 0283, 0272, 0256, 0271

## Change
Needs card 20. The shells of `feature.md` step 5. Every file is under `examples/tank_game/`;
read `Code/tank_hull.h`, `Code/tank_hull_system.c` and `Code/project.c` as the pattern, and
`game/include/game/project.h` for the step, spawn and remove.

- `Code/tank_gun.h`, `Code/tank_gun_system.c` (new): "Tank / Gun", on a turret: `prefab` (CHAR 64,
  default `shell`), `rate` (shots a second, default 6), `muzzle` (FLOAT3 in the turret's own
  frame, default (0, 0.3, -1.2)) and `wait` (seconds to the next shot, default 0, written only
  by the system). `tank_gun_system_run(const voe_game_project_step *step)`: every step each gun's
  `wait` falls by the step's seconds; while the left mouse button or Space is held
  (platform/input.h) and `wait` is at or below zero, it spawns `prefab` at the gun's world
  transform (`voe_scene_transform_world`) moved by `muzzle` turned by that rotation, with that
  rotation, and `wait` becomes 1 / `rate`. A refused spawn leaves `wait` as it was. Nothing
  headless. Registered with room for `VOE_GAME_WORLD_AUTHORED`.
- `Code/tank_shell.h`, `Code/tank_shell_system.c` (new): "Tank / Shell": `speed` (m/s, default
  30) and `life` (seconds, default 3, counted down by the system). `tank_shell_system_run(const
  voe_game_project_step *step)`: each shell moves along its own −Z by speed × seconds through one
  transform intent, as the hull drives; its `life` falls by the seconds and at zero it is removed
  with `voe_game_project_remove`. Room for 256.
- `Code/project.c`: registers both; before the move runs hull, turret, gun, then shell. Header:
  the new order and why the gun is after the turret (it fires along this step's aim).
- `Assets/shell.prefab` (new, scene text as `main.scene` spells it): `[1]` "Shell" with a
  transform and `tank_shell`; `[2]` "Shell body" under it (`voe_scene_parent`) with a cube
  `voe_3d_shape` in a bright colour and a transform scaled (0.15, 0.15, 0.4) — the scale is on
  the child because a spawned root is scale one (0283 point 9).
- `main.scene`: `[5]` (tank_head) gains a `[5.tank_gun]` section with the defaults.
- `Code/Code.md`, `tank_game.md`: the new files and what the tank now does.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. The human's: Play, hold the left button: shells stream from the barrel
and vanish after three seconds.
