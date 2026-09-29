# Code

The tank game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `tank_hull.h` — the Tank / Hull component: a drive speed, default 4 m/s, and a turn, default 90 deg/s.
- `tank_hull_system.c` — turns each hull and drives it along its own forward as the control row says: W/A/S/D or the left stick.
- `tank_turret.h` — the Tank / Turret component: a turn, default 180 deg/s, and an aim offset, default 0.
- `tank_turret_system.c` — turns each turret about the world's up toward where the mouse pointer meets the ground, or the right stick's direction on screen, held when let go.
- `tank_gun.h` — the Tank / Gun component: a prefab to fire, default `shell`, a rate, default 6 a second, a muzzle offset and the wait to the next shot.
- `tank_gun_system.c` — while the control row's fire holds (left button, Space or right trigger), spawns each ready gun's prefab at its muzzle, fired along its turret's barrel.
- `tank_shell.h` — the Tank / Shell component: a speed, default 30 m/s, and a life, default 3 s.
- `tank_shell_system.c` — flies each shell along its own forward and removes it when its life runs out.
- `tank_spawner.h` — the Tank / Spawner component: a prefab to spawn, default `enemy_tank`, a period, default 4 s, a most, default 6, and the wait to the next spawn.
- `tank_spawner_system.c` — spawns each ready spawner's prefab at it, turned as it is, while the world holds fewer enemies than its most.
- `tank_enemy.h` — the Tank / Enemy component: a speed, default 2 m/s, and a life, default 20 s.
- `tank_enemy_system.c` — drives each enemy along its own forward and removes it, turret and all, when its life runs out.
- `tank_camera.h` — the runtime-only camera fit row: the camera's authored field of view, kept so the level's width stays in view at any window shape.
- `tank_camera_system.c` — widens the camera's lens when the window is narrower than 16:9, so the width framed at 16:9 stays in view.
- `tank_control.h` — the runtime-only control row on the player's hull: drive, turn, aim, fire and whether the pad is in use.
- `tank_control_system.c` — reads the keyboard, mouse and lowest connected gamepad into the control row, with a radial dead zone on each stick.
- `project.c` — the four entry points: registers all eight types, runs the control first (pad beside keyboard and mouse, last touched wins), then the hull, turret, gun, shell, spawner, enemy, then camera before the move, nothing after, draws no interface.
