# Code

The tank game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `tank_hull.h` — the Tank / Hull component: a drive speed, default 4 m/s, and a turn, default 90 deg/s.
- `tank_hull_system.c` — turns each hull on A/D and drives it along its own forward on W/S.
- `tank_turret.h` — the Tank / Turret component: a turn, default 180 deg/s, and an aim offset, default 0.
- `tank_turret_system.c` — turns each turret about the world's up toward where the mouse pointer meets the ground.
- `tank_gun.h` — the Tank / Gun component: a prefab to fire, default `shell`, a rate, default 6 a second, a muzzle offset and the wait to the next shot.
- `tank_gun_system.c` — while the left button or Space is held, spawns each ready gun's prefab at its muzzle, fired along its turret's barrel.
- `tank_shell.h` — the Tank / Shell component: a speed, default 30 m/s, and a life, default 3 s.
- `tank_shell_system.c` — flies each shell along its own forward and removes it when its life runs out.
- `tank_spawner.h` — the Tank / Spawner component: a prefab to spawn, default `enemy_tank`, a period, default 4 s, a most, default 6, and the wait to the next spawn.
- `tank_spawner_system.c` — spawns each ready spawner's prefab at it, turned as it is, while the world holds fewer enemies than its most.
- `tank_enemy.h` — the Tank / Enemy component: a speed, default 2 m/s, and a life, default 20 s.
- `tank_enemy_system.c` — drives each enemy along its own forward and removes it, turret and all, when its life runs out.
- `project.c` — the four entry points: registers all six types, runs the hull, turret, gun, shell, spawner, then enemy before the move, nothing after, draws no interface.
