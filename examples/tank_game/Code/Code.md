# Code

The tank game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `tank_hull.h` — the Tank / Hull component: a drive speed, default 4 m/s, and a turn, default 90 deg/s.
- `tank_hull_system.c` — turns each hull on A/D and drives it along its own forward on W/S.
- `project.c` — the four entry points: registers the hull, runs it before the move, nothing after, draws no interface.
