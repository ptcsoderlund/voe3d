# 02 — The bounce scene's harness moves to a file of its own
folder: 3d
after: 01
decisions: 0168

## Change
`3d/tests/bounce_scene.c` is 712 lines and cards 03–05 need its world and its frame for a second test
program. Split it by function; no case changes.

- New `3d/tests/bounce_world.inc` (test `.inc` files are compiled into the program that includes them,
  as `model_data.inc` is). Move into it, unchanged in behaviour:
  - the world builder: floor, sun, the bounce settings and the box. The box's centre, size and colour
    become parameters, so a later case can place a 1 m purple cube or a red slab;
  - the reference world (the same world without the box) and its builder;
  - the frame: one draw of the world through `voe_3d_draw_system_shadows` and the passes after it;
  - the settle: DUMMIES empty passes, then up to BOUND pairs until the patch holds, then FADE frames;
  - the readback and the projection of a world point to a pixel.
  Each function keeps or gains a header comment saying what it does, what it returns and what it
  leaves for the caller to free. The file's own header says what it holds and which programs include it.
- `3d/tests/bounce_scene.c`: includes the `.inc` and keeps only its cases and `main`. Its header loses the
  paragraphs that described the harness and points to the `.inc` for them.
- `3d/tests/tests.md`: an entry for `bounce_world.inc`, and the `bounce_scene.c` entry says the harness is
  there.

The frame and settle constants (SIDE, BOUND, FADE, PASSES, DUMMIES, SCRATCH) and the tolerances TINT and
SHADOW move with the harness; a static the moved functions share moves with them.

## Done when
After `cmake --build --preset debug --target voe_test_3d_bounce_scene`,
`ctest --test-dir build/debug -R '^3d/bounce_scene$'` passes with the same cases as before.
