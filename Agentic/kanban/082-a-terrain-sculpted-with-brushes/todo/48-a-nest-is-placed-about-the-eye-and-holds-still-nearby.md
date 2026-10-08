# 48 — A nest is placed about the eye and holds still nearby
folder: 3d
after: 47
decisions: 0168, 0387, 0389

## Change
0389 point 2, as arithmetic in `3d/include/3d/bounce_grid.h` and `3d/src/bounce_grid.c`.

- `VOE_3D_BOUNCE_NESTS` 3, and the nests' spacings 16, 4 and 1 m, coarse to fine. Use a constant array or
  a function from nest index to spacing, whichever is fewer names. Nest i is render volume i + 1.
- `voe_3d_bounce_grid voe_3d_bounce_grid_nest(float spacing, const int32_t *placed, voe_math_double3 eye)`:
  - Home is the eye's cell, at `spacing` about the world origin in double, less (12, 9, 12).
  - With `placed` NULL, the cell is home.
  - Otherwise the cell is `placed`, moved on each axis only as far as brings the eye's cell within 2 cells
    of its home.
  - When that move is a whole grid or more on any axis (24, 12, 24 cells), the cell is home: nothing is
    kept, so the nest is centred again.
  - `corner` is the cell's lowest corner about `eye`. `spacing` is copied.
  - A spacing not finite or not above 0, or an eye not a number, asserts.
- `voe_3d_bounce_grid_sun` already takes any grid. Check its comment holds for a nest, and say so in one
  phrase.
- The header comment gets a paragraph: nests about the eye (0387, 0389) beside the level grid, why the bias
  is low, the hysteresis, and that a nest's cells are world-fixed like the level grid's.
- `3d/include/3d/3d.md`: the `bounce_grid.h` entry mentions nests.
- `3d/tests/bounce_grid.c`, new cases:
  - `a_nest_starts_with_the_eye_nine_cells_up`
  - `a_nest_holds_still_while_the_eye_stays_within_two_cells`
  - `a_nest_moves_one_cell_when_the_eye_crosses_three`
  - `a_nest_jumps_home_when_the_eye_leaves_it` (an eye a grid away gives home)
  - `two_eyes_in_one_cell_give_one_world_place`
- `3d/tests/tests.md`: the `bounce_grid.c` entry names them.

## Done when
`ctest --test-dir build/debug -R '^3d/bounce_grid$'` passes with the five new cases.
