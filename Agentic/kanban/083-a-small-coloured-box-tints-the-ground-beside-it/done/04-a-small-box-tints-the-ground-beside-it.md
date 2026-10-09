# 04 — A small box tints the ground beside it, in 3d's frame
folder: 3d
after: 03
decisions: 0168, 0387, 0389, 0390

## Change
The feature's own picture, as cases in `3d/tests/bounce_tint.c` (card 03), built with
`bounce_world.inc`. The world: flat floor, sun low, one bounce, strength 2, as `feature.md`'s How to test
step 1; a 1 m purple cube (linear colour about (0.5, 0.1, 0.6)) with its lit face toward the sun. Each
case settles before it reads, compares against the same world without the box, and prints what it read
on failure.

- `a_small_box_tints_the_ground_beside_it`: the ground 0.25 m out from the lit face is redder and bluer
  than the reference by at least TINT/255 and no greener; 3 m out by less than half TINT/255; the ground
  at the foot of the shaded face within SHADOW/255 of the reference in every channel. Then strength 5:
  the 0.25 m tint at least the strength-2 one plus 1/255. Then back to 2: within 1/255 of the first read
  (step 6).
- `a_slab_reddens_the_box_face`: at strength 5, a red slab (2 × 1 × 0.2 m, about 1.5 m from the lit face,
  not between the face and the sun) added and settled; the middle of the cube's lit face redder than
  before the slab by at least TINT/255.
- `a_moved_box_leaves_no_ring`: the cube moved 1 m along its lit face's normal and settled; the ground
  0.25 m out from the new face tinted as above; the ground where the old 0.25 m point was, and eight
  points on a 2 m circle about the old place, within EVEN/255 of the reference (step 5, step 4: no ring
  or blotch). EVEN moves into `bounce_world.inc` with TINT if it is not there yet.
- `a_recoloured_box_tints_its_new_colour`: the cube recoloured green through `voe_3d_shape_submit`
  (header of `3d/include/3d/shape_system.h`) and settled; the 0.25 m ground greener than the reference
  by at least TINT/255 and redder by less than half TINT/255.

If a case fails, the fault is in what cards 01 and 03 left: find it from the headers of
`3d/src/draw_bounce.c`, `3d/src/draw_bounce.h`, `3d/src/bounce_grid.c` and `3d/src/draw_shadows.c`, read
the body of the one at fault and fix it there, correcting its header. A fault in render: block the card
naming the function. Do not loosen a tolerance past the feature's picture; if one has to move, say why in
the case's header paragraph.

Header of `bounce_tint.c`: one paragraph per case, the How to test step it stands for. `3d/tests/tests.md`:
the `bounce_tint.c` entry lists the cases.

## Done when
After `cmake --build --preset debug --target voe_test_3d_bounce_tint`,
`ctest --test-dir build/debug -R '^3d/bounce_tint$'` passes with the four cases above and card 03's.
