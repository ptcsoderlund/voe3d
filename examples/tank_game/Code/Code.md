# Code

The tank game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `tank_hull.h` — the Tank / Hull component: a drive speed, default 4 m/s, and a turn, default 90 deg/s.
- `tank_hull_system.c` — drives and turns each hull as the control row says, never backing out past 3 m short of the screen's bottom once the scroll row exists; gives a hull with no emitter tread dust, played while it drives, and one with no sound the engine hum, pitched up with the drive.
- `tank_turret.h` — the Tank / Turret component: a turn, default 180 deg/s, and an aim offset, default 0; the call that turns any turret toward a point, for its owner.
- `tank_turret_system.c` — turns the player's turrets about the world's up toward where the mouse pointer meets the ground, or the right stick's direction on screen, held when let go, through the exported turn call.
- `tank_gun.h` — the Tank / Gun component: a prefab to fire, default `shell`, a rate, default 6 a second, a muzzle offset and the wait to the next shot.
- `tank_gun_system.c` — while fire holds, fires each ready gun's prefab at its muzzle along its turret's barrel; each shot bursts the gun's muzzle flash and restarts the fade of its light, a child at the flash, and plays the shot sound at the muzzle.
- `tank_shell.h` — the Tank / Shell component: a speed, default 30 m/s, a life, default 3 s, and a radius, default 0.1 m; the runtime-only shot row: owner, where the sweep starts, what it hit, and the points of a breakable it wrecked.
- `tank_shell_system.c` — flies each shell, stops it where it hits, swaps a breakable the player hit for its wreck, its shot row carrying the breakable's points, and removes it on a hit or when its life runs out, playing the hit sound, or the explosion at a wreck.
- `tank_breakable.h` — the Tank / Breakable component: a wreck prefab, default empty, and the points the player scores for wrecking it, default 100, that only the player's shell's hit swaps the thing for, once a step; an enemy's shell changes nothing.
- `tank_breakable.c` — the breakable's key and registration; it has no system.
- `tank_goal.h` — the Tank / Goal component the sponsor places at the level's end: the points the player scores, default 1000, on winning when its hull's z reaches the goal's world z; it needs a transform.
- `tank_goal.c` — the goal's key and registration, with its need of a transform; it has no system.
- `tank_lives.h` — the runtime-only lives row on the player's hull: 3 at the start, one off per shot that hits the hull, never below 0; the enemies read it for whom to fire at; the hull is made solid when it has no collider.
- `tank_lives_system.c` — adds the lives row to the first hull, makes it solid with the enemy's box when it has no collider, and takes a life for each shot that hit it this step; the menu's HUD shows the count.
- `tank_spawner.h` — the Tank / Spawner component, a wave: a prefab to spawn, default `enemy_tank`, a period, default 4 s, a most, default 6, a count, default 4, 0 never ending, the wait to the next spawn and the spawns made.
- `tank_spawner_system.c` — wakes each wave when the screen's top reaches its z, then spawns its prefab at it, turned as it is, while the world holds fewer enemies than its most, until it has made its count.
- `tank_enemy.h` — the Tank / Enemy component: a speed, default 2 m/s, a life, default 20 s, a prefab to fire, default `shell`, a rate, default 0.5 a second, a range, default 30 m, a muzzle offset and the wait to the next shot.
- `tank_enemy_system.c` — drives each enemy forward, aims its turret at the player's hull in range, fires once on target with flash, the fade of its light child at the flash, and shot sound, gives it a lower engine hum, and removes it, turret and all, when its life runs out.
- `tank_fade_away.h` — the Tank / Fade away component: a wait, default 2 s, the seconds it fades, default 1 s, and its age; a thing that lies still, turns see-through and leaves with its tree, as an enemy's wreck.
- `tank_fade_away_system.c` — grows each age by the step, fades every model on the entity's tree to (age − wait) / seconds through the model's intent, only when it differs, and removes the entity once the fade ends.
- `tank_camera.h` — the runtime-only camera fit row: the camera's authored field of view, kept so the level's width stays in view at any window shape.
- `tank_camera_system.c` — widens the camera's lens when the window is narrower than 16:9, so the width framed at 16:9 stays in view.
- `tank_scroll.h` — the runtime-only scroll row on the camera: its lead on the first hull's z, and the z where the screen's bottom and top meet the hull's ground; the level runs along −Z.
- `tank_scroll_system.c` — after the move, moves the camera forward, never back, to the hull's z plus the lead, and finds the screen's bottom and top edges on the ground from its pose and lens.
- `tank_control.h` — the runtime-only control row on the player's hull: drive, turn, aim, fire and whether the pad is in use.
- `tank_control_system.c` — reads the keyboard, mouse and lowest connected gamepad into the control row, with a radial dead zone on each stick.
- `tank_light_fade.h` — the Tank / Light fade component: a peak, default 4, the seconds it fades, default 0.12, and the seconds left; the call that restarts a fade from full, per shot, and the finder of the fade under a parent.
- `tank_light_fade_system.c` — sets each faded point light to peak × left / seconds through its replace, only when it differs, then counts left down to 0.
- `tank_state.h` — the runtime-only round state row on the player's hull: the phase, menu, playing, paused, won or lost, the score, the screens' chosen item and last frame's input levels; a fresh world starts at the menu; the menu's entry point.
- `tank_state_system.c` — adds the state row at the menu to the first hull and, while playing, adds this step's shot points, sets lost at 0 lives, and else won with the goal's points once the hull reaches the goal's z.
- `tank_menu.c` — draws each phase's screen, the menu, the pause, won, lost, or the `Score N` and `Lives N` HUD, moves and presses the chosen item by keyboard, pad or click, pauses and resumes, and sets the run's paused and restart asks.
- `project.c` — the four entry points: registers all sixteen types; while playing runs the control, hull, turret, gun, shell, lives, state, spawner, enemy and fade away before the move and the scroll after, else only the state; the light fade and camera always; the interface is the menu.
