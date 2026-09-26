# src

`game`'s implementation, one file per public header except `scene.h`, which a
project's cooked `scene.c` defines.

- `world.c` — the eleven registrations, their capacities and the room for a project's types.
- `frame.c` — the world step's drains in order, and the frame: that step and the one window pass.
- `steps.c` — one fixed step's calls in order (systems, world step, move, transforms, after-the-move systems, world step again), and the bank the steps are drawn from.
- `project.c` — the 32 replace keys, a project type's registration and the drain of its replaces.
- `run.c` — the run's steps and their refusals; the only file naming `voe_game_scene_build` and the project's entry points.
