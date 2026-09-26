# Code

The coin game's own components and systems, compiled into its game or into
the library the editor loads (0242).

- `rotator.h` — the Rotator component: degrees a second about the world's Y, default 90.
- `rotator_system.c` — turns every rotator with a transform by its degrees a second.
- `project.c` — the four entry points: registers rotator, runs it before the move, draws no interface.
