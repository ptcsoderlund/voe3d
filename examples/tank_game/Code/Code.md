# Code

The tank game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `tank_hull.h` — the Tank / Hull component: a drive speed, default 4 m/s, and a turn, default 90 deg/s.
- `tank_hull_system.c` — turns each hull and drives it along its own forward as the control row says: W/A/S/D or the left stick; gives a hull with no emitter tread dust, played while it drives, and one with no sound the engine hum, pitched up with the drive.
- `tank_turret.h` — the Tank / Turret component: a turn, default 180 deg/s, and an aim offset, default 0; the call that turns any turret toward a point, for its owner.
- `tank_turret_system.c` — turns the player's turrets about the world's up toward where the mouse pointer meets the ground, or the right stick's direction on screen, held when let go, through the exported turn call.
- `tank_gun.h` — the Tank / Gun component: a prefab to fire, default `shell`, a rate, default 6 a second, a muzzle offset and the wait to the next shot.
- `tank_gun_system.c` — while the control row's fire holds (left button, Space or right trigger), fires each ready gun's prefab at its muzzle along its turret's barrel, owned by its tank; gives a gun with no emitter a muzzle flash and bursts it each shot, and plays the shot sound at the muzzle.
- `tank_shell.h` — the Tank / Shell component: a speed, default 30 m/s, a life, default 3 s, and a radius, default 0.1 m; the runtime-only shot row: owner, where the sweep starts, and what it hit.
- `tank_shell_system.c` — flies each shell, stops it where it hits, swaps a breakable the player hit for its wreck, and removes it on a hit or when its life runs out, playing the hit sound, or the explosion at a wreck.
- `tank_breakable.h` — the Tank / Breakable component: a wreck prefab, default empty, that only the player's shell's hit swaps the thing for, once a step; an enemy's shell changes nothing.
- `tank_breakable.c` — the breakable's key and registration; it has no system.
- `tank_lives.h` — the runtime-only lives row on the player's hull: 3 at the start, one off per shot that hits the hull, never below 0; the enemies read it for whom to fire at; the hull is made solid when it has no collider.
- `tank_lives_system.c` — adds the lives row to the first hull, makes it solid with the enemy's box when it has no collider, takes a life for each shot that hit it this step, and draws `Lives N` in a HUD panel at the top left.
- `tank_spawner.h` — the Tank / Spawner component: a prefab to spawn, default `enemy_tank`, a period, default 4 s, a most, default 6, and the wait to the next spawn.
- `tank_spawner_system.c` — spawns each ready spawner's prefab at it, turned as it is, while the world holds fewer enemies than its most.
- `tank_enemy.h` — the Tank / Enemy component: a speed, default 2 m/s, a life, default 20 s, a prefab to fire, default `shell`, a rate, default 0.5 a second, a range, default 30 m, a muzzle offset and the wait to the next shot.
- `tank_enemy_system.c` — drives each enemy forward, aims its turret at the player's hull in range, fires once on target with flash and shot sound, gives it a lower engine hum, and removes it, turret and all, when its life runs out.
- `tank_camera.h` — the runtime-only camera fit row: the camera's authored field of view, kept so the level's width stays in view at any window shape.
- `tank_camera_system.c` — widens the camera's lens when the window is narrower than 16:9, so the width framed at 16:9 stays in view.
- `tank_control.h` — the runtime-only control row on the player's hull: drive, turn, aim, fire and whether the pad is in use.
- `tank_control_system.c` — reads the keyboard, mouse and lowest connected gamepad into the control row, with a radial dead zone on each stick.
- `project.c` — the four entry points: registers all eleven types, runs the control first, then the hull, turret, gun, shell, lives, spawner, enemy, then camera before the move, nothing after; the interface is the lives' HUD.
