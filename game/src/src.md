# src

`game`'s implementation, one file per public header except `scene.h`, which a
project's cooked `scene.c` defines.

- `world.c` — the eight registrations and their capacities.
- `frame.c` — the systems and the one window pass, in order.
- `run.c` — the run's steps and their refusals; the only file naming `voe_game_scene_build`.
