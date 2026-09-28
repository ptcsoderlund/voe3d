# Code

The tank game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `tank_hull.h` — the Tank / Hull component: a drive speed, default 4 m/s, and a turn, default 90 deg/s.
- `tank_hull_system.c` — turns each hull on A/D and drives it along its own forward on W/S.
- `tank_turret.h` — the Tank / Turret component: a turn, default 180 deg/s, and an aim offset, default 0.
- `tank_turret_system.c` — turns each turret about the world's up toward where the mouse pointer meets the ground.
- `project.c` — the four entry points: registers the hull and the turret, runs the hull then the turret before the move, nothing after, draws no interface.
