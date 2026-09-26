# Code

The example project's own components and systems, compiled into its game or
into the library the editor loads (0242).

- `keyboard_input.h` — the Keyboard Input component: a read-only move direction and jump from the keys.
- `keyboard_system.c` — writes every keyboard_input row from W/A/S/D and Space's edge.
- `player.h` — the Player component: a walking speed, a jump height and gravity.
- `player_system.c` — submits each player's body with the walk, the jump and gravity as its velocity.
- `coin.h` — the Coin component: a spin, default 2 rad/s.
- `coin_system.c` — spins each coin and destroys the ones a player's collider overlaps.
- `follow_camera.h` — the Follow Camera component: a target entity and a distance, default 6.
- `follow_camera_system.c` — keeps each follower a distance behind its target along its own forward.
- `project.c` — the four entry points: registers the four types, runs three systems before the move and follow after it, draws no interface.
