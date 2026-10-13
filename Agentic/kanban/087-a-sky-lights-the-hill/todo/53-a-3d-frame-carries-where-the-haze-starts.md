# 53 — A 3d frame carries where the haze starts
folder: 3d
after: 51, 52
decisions: 0168, 0414

## Change
0414 point 3: the row's `haze_start` reaches the pass. Read the comment above
`voe_3d_draw_system_sky` in `3d/include/3d/draw_system.h` and the header of `3d/tests/sky.c`.

- `3d/src/draw_system.c`: `voe_3d_draw_system_sky` copies the row's `haze_start` into the record
  as it copies `haze`.
- `3d/include/3d/draw_system.h`: that comment names `haze_start` among the fields copied.
- `3d/tests/sky.c`: the row of values none of them the default gains a `haze_start` that is not
  100 (say 250), and the record's `haze_start` is checked equal to it with the other fields. The
  header's "each sky field is the row's" covers it; say so if the header lists fields.
- `3d/tests/tests.md` (`sky.c`) and `3d/include/3d/3d.md` if either names the sky's fields: say so,
  each one sentence of at most 300 characters.

## Done when
The test `3d/sky` passes after the folder's build.

Human, in the editor on lumberhill_project with the sky on: set haze to 0 and note the ground, the
cube and the slab; raise haze (the scene's saved 50, then 500), with haze start at its 100 m
default: nothing within 100 m changes, and far ridges still fade into the horizon colour (feature
step 7). Set haze start to 0: the old fade from the eye returns.
