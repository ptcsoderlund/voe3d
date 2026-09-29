# 06 — The tank game reads its controls into one row
folder: examples
after: 01
decisions: 0168, 0292, 0239, 0272, 0261

## Change
0292 point 7, the reading half; 07 makes the tank obey it. Files under
`examples/tank_game/`: new `Code/tank_control.h` and `Code/tank_control_system.c`;
`Code/project.c`, `Code/Code.md`. Read `Code/tank_camera.h` and `Code/tank_camera_system.c`
for a runtime-only row added through the structural queue, and `platform/include/platform/input.h`
for the keys, buttons, pointer and pads. Never edit `main.scene` (0272).

- `tank_control.h`: `tank_control` and its `tank_control_key`, runtime-only, capacity 1, no menu: `drive` and `turn`
  in −1..1 (turn positive to the left, about +Y), `aim_x` and `aim_y` (the right stick's
  direction on screen, 0 and 0 when held), `fire`, `pad` (the pad is in use), and the pointer's
  last `x` and `y` for telling it moved. A register call and a run call taking the step. Header
  points: one writer, the row is read by the hull, turret and gun; 0292 point 7's rules for
  the pad, the dead zone, the fire threshold and last touched; nothing headless.
- `tank_control_system.c`: on the first step with a window and a `tank_hull` row, add the row
  to that hull's entity. Each step with the row and a window:
  - The pad: the lowest connected slot of `voe_platform_input_gamepad`, or none.
  - A stick through one radial dead zone function: length under 0.2 is 0; past it the same
    direction at (length − 0.2) / 0.8, at most 1.
  - Pad touched: a stick past its dead zone, a trigger past 0.5, any button. Keyboard touched:
    W, A, S, D or Space, a mouse button, or the pointer not where the row last saw it.
  - `pad` becomes true when the pad was touched, false when the keyboard was (keyboard wins a
    tie) or no pad is connected, else stays.
  - With `pad`: `drive` the left stick's y, `turn` minus its x, `aim` the right stick. Without:
    `drive` W minus S, `turn` A minus D, `aim` 0.
  - `fire`: the left mouse button or Space, or the right trigger past 0.5.
  - The whole row through `voe_ecs_component_set`.
- `project.c`: register it; run it first before the move, ahead of the hull.
- `Code.md`: the two files.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0 and
`grep -q tank_control_run examples/tank_game/Code/project.c` exits 0.
