# 05 — The water system keeps the clock
folder: 3d
after: 04
decisions: 0168, 0305

## Change
0305 point 5. Read `3d/include/3d/water_component.h` (card 04),
`3d/include/3d/emitter_system.h` and `3d/src/emitter_system.c` (draining a
replace, adding and dropping a runtime row).

- `3d/include/3d/water_system.h`, new:
  `void voe_3d_water_system_run(voe_ecs_world *world, float seconds)`.
  Header points, in a run's order: drains the replace intents (a dead
  entity or one with no water is dropped); adds a waves row to each water
  lacking one, at 0, and drops rows whose water is gone; advances each
  clock by `seconds`, wrapped below 60 in double; 0 seconds only drains;
  runs once a frame where the emitters run.
- `3d/src/water_system.c`, new: the run.
- `3d/tests/water_system.c`, new: a water gains a waves row after one run;
  the clock after 3 × 0.5 s is 1.5; 61 s of steps wrap it to 1 within
  1e-9; a replace changes the row's colour; a removed water's waves row is
  gone after the next run.
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: one entry
  each.

## Done when
The test `3d/water_system` passes after the folder's build.
