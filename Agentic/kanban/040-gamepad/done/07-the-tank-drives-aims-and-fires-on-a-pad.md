# 07 — The tank drives, aims and fires on a pad
folder: examples
after: 03, 05, 06
decisions: 0168, 0292, 0239, 0272
read: feature.md

## Change
0292 point 7, the obeying half, and the last card of 040. Files under `examples/tank_game/`:
`Code/tank_hull.h`, `Code/tank_hull_system.c`, `Code/tank_turret.h`,
`Code/tank_turret_system.c`, `Code/tank_gun.h`, `Code/tank_gun_system.c`, `Code/project.c`,
`Code/Code.md`, `tank_game.md`. Read `Code/tank_control.h` (06). Each system finds the one
`tank_control` row by iterating its table; no row (headless, or before its first step) is
nothing moving, as no window was before. Never edit `main.scene` (0272).

- Hull: drives `speed × drive` and turns `turn × turn` for the step's seconds, in place of
  reading W/A/S/D; so a stick at rest stops it dead. `tank_hull_system_run` drops its window
  parameter; `project.c`'s call follows. Header: driven by the control row, analogue on a pad.
- Turret: without `pad`, the pointer's aim as today. With `pad` and an `aim` of non-zero
  length: the camera's +X and −Z flattened onto the ground and normalised give screen right
  and up; the aim point is the turret's world position plus right × `aim_x` + up × `aim_y`,
  turned toward exactly as a pointer's point is. With `pad` and no `aim`: the turret holds.
  Header: both aims and the hold.
- Gun: fires while the row's `fire` holds, in place of the left button or Space. Header: fire
  comes from the control row (mouse, Space or the right trigger).
- `project.c` header, `Code.md` and `tank_game.md`: the control row first, the pad beside
  the keyboard and mouse, last touched wins.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and
`grep -c tank_control_key examples/tank_game/Code/tank_{hull,turret,gun}_system.c` shows each
at least 1.

For the human, `## How to test` in `feature.md`: steps 1–4 in the editor's Play on
`examples/tank_game` on Linux, with a pad, touching the keyboard between to see it take over;
step 5 on Windows with Ship and the shipped game.
