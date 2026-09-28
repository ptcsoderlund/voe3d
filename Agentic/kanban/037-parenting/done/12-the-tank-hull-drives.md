# 12 — The tank's hull drives on WASD
folder: examples
decisions: 0168, 0272, 0251, 0242, 0256

## Change
Needs card 06. The tank game gets its first code (0272), written as the game keeps it. Agents
never edit `examples/tank_game/main.scene` (0251). Follow `examples/capsule/Code/` as the pattern:
read `examples/capsule/Code/Code.md`, the header comments of its `project.c`, `player.h` and
`player_system.c`, and `game/include/game/project.h`.

- New folder `examples/tank_game/Code/` with `Code.md` (one entry per file).
- `Code/tank_hull.h`: the `tank_hull` component, key `tank_hull_key`, described: `speed` in
  metres a second (default 4) and `turn` in degrees a second (default 90); menu `Tank / Hull`;
  `bool tank_hull_register(voe_ecs_world *)` and `void tank_hull_system_run(voe_ecs_world *,
  voe_platform_window *, double seconds)`.
- `Code/tank_hull_system.c`: registers through `voe_game_project_component`; each step, for every
  hull with a transform: A/D turn its row about +Y by `turn`·seconds (A left), W/S move it along
  its own −Z (W forward) by `speed`·seconds; one transform intent each. Nothing with no window
  (headless). Header points: a hull is a root and moves its own row (turret and barrel ride on it
  by parenting, 0271); a full queue leaves it where it was this step.
- `Code/project.c`: the four entry points — registers the hull; runs the hull system before the
  move; after the move nothing; no interface (end the frame, return true, as the capsule does).
- `examples/tank_game/tank_game.md`: a `Code` entry.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only
-DVOE_BASE_DESCRIPTIONS=1 $(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1;
done)` exits 0, and `bash ~/.claude/skills/checks/scripts/checks.sh --folder examples` prints
`FINDINGS: 0`.
