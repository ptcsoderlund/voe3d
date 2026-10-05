# Code

The example project's own component and system, compiled into its game or
into the library the editor loads (0242).

- `day_night.h` — the Day Night component: a light's intensity and shadows by day and by night, a period, default 20 s, and a read-only clock.
- `day_night_system.c` — moves each clock on and submits the light's intensity and shadows when the time of day changes them.
- `project.c` — the four entry points: registers Day Night, runs it before the move, nothing after it, draws no interface.
