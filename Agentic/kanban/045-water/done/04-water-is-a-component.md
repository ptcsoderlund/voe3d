# 04 — Water is a component
folder: 3d
after: 03
decisions: 0168, 0305

## Change
0305 point 5. Read `3d/include/3d/emitter_component.h` and
`3d/src/emitter_component.c` (the pattern: FIELDS macro, key, replace
intent, runtime row, register) and `3d/tests/emitter_component.c`.

- `3d/include/3d/water_component.h`, new:
  - `VOE_3D_WATER_FIELDS`: width, length, wave_height, wave_length, deep
    (FLOAT32), colour, sky (COLOUR); `voe_3d_water` described from it;
  - its key, menu "Rendering / Water", needing a transform; the defaults
    of 0305 point 5 (colour about (0.02, 0.09, 0.10), sky about
    (0.55, 0.70, 0.85), linear);
  - the replace intent and its submit;
  - `voe_3d_waves`, the runtime-only row: `double seconds`;
  - `voe_3d_water_register(voe_ecs_world *world, uint32_t capacity)`, both
    tables and the intent queue; row accessors as the emitter's;
  - header points: what each field is in metres or linear colour, that
    waves are drawn and never move the plane, the clock row's owner.
- `3d/src/water_component.c`, new: the table, the description, the
  registration.
- `3d/tests/water_component.c`, new: registered, the defaults, a
  description that names every field with its kind, a replace queued.
- `3d/include/3d/3d.md`, `3d/src/src.md`, `3d/tests/tests.md`: one entry
  each.

## Done when
The test `3d/water_component` passes after the folder's build.
