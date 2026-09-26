# Code

The coin game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `rotator.h` — the Rotator component: degrees a second about the world's Y, default 90.
- `rotator_system.c` — turns every rotator with a transform by its degrees a second.
- `player.h` — the Player component: the sponsor's numbers for the walk, the jump, the camera and the score.
- `player_system.c` — registers player with its defaults under "Player".
- `coin.h` — the Coin component, its points, and the runtime-only coin_taken that hides a taken coin.
- `coin_system.c` — registers coin under "Coin" and coin_taken with no menu.
- `project.c` — the four entry points: registers rotator, player and coin, runs it before the move, draws no interface.
