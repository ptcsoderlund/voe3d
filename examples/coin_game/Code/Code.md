# Code

The coin game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `rotator.h` — the Rotator component: degrees a second about the world's Y, default 90.
- `rotator_system.c` — turns every rotator with a transform by its degrees a second.
- `player.h` — the Player component: the sponsor's numbers for the walk, the jump, the camera and the score; the runtime-only player_state.
- `player_system.c` — registers player and player_state; walks, jumps and restarts players, and keeps the camera behind the first.
- `coin.h` — the Coin component, its points, and the runtime-only coin_taken that hides a taken coin.
- `coin_system.c` — registers coin under "Coin" and coin_taken with no menu; takes a coin a player's overlap finds and puts it back after a restart.
- `game_state.h` — the run's runtime-only game_state row: phase, dropped score, restarts; the derived score and coins left.
- `game_system.c` — registers game_state, makes its row on the first player, drops the score each second and ends the run.
- `project.c` — the four entry points: registers rotator, player, coin and game_state, runs game, player, coin, rotator before the move and player_camera after it, draws no interface.
