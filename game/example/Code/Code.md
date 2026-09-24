# Code

The example project's own components and systems, compiled into its game or
into the library the editor loads (0242).

- `keyboard_input.h` — the Keyboard Input component: a read-only move direction from the keys.
- `keyboard_system.c` — writes every keyboard_input row from W/A/S/D.
- `player.h` — the Player component: a walking speed, default 2.
- `player_system.c` — moves each player's transform on XZ by its input and speed.
- `follow_camera.h` — the Follow Camera component: a target entity and a distance, default 6.
- `follow_camera_system.c` — keeps each follower a distance behind its target along its own forward.
- `project.c` — the two entry points: registers the three types and runs the three systems in order.
