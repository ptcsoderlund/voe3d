# 62 — A one-metre box tints the ground, and a moved one leaves no ring
folder: 3d
after: 53, 61
decisions: 0168, 0387, 0388, 0389, 0390

## Change
The proof of bug 05, in `3d/tests/bounce_scene.c` and its header comment. Spec 051's steps 2 and 3, with
a small box. Cards 56 and 59 already brought the file's world, eye, waits, budget and existing cases to
the nests, and card 61 made a removed blocker relight the nest: the world registers shape changes, every frame remembers transforms first, `settles` waits
out the fade, and the eye stands where the 1 m nest covers the box (0390). Change none of that.

- **New cases:**
  - `a_small_box_tints_the_ground_beside_it`: in place of the 2 m box, a 1 m cube, strongly purple, on the
    same ground and sun. Once settled, the ground 0.25 m out from its sunlit face has red less green above
    the reference's by at least TINT/255. 3 m out, it is less than half of that.
  - `a_moved_box_leaves_no_ring`: settled, the 1 m box is moved 2 m along x, then settled again. Every
    open-ground pixel `open_ground_is_even` reads is within EVEN/255 of the same pixel in a world where the
    box always stood at its new place.
  - `a_recoloured_box_tints_its_new_colour`: settled, the box's colour is changed through
    `voe_3d_shape_submit` to strong blue, then settled. The ground beside its lit face is bluer than before.
- **Header:** a paragraph for the three new cases.
- **`3d/tests/tests.md`:** the `bounce_scene.c` entry names the new cases.

## Done when
`ctest --test-dir build/debug -R '^3d/bounce_scene$'` passes with the three new cases.

Then the human, on Windows (0381), with the debug build of the editor:
1. Bug 05's repro: a stretched box as a floor, a sun at bounces 2, strength 2 to 5, low. Add a 1 m cube
   and colour it purple. Within about a second, the floor beside the cube's lit face turns pink, fading
   with distance. No curved edge shows anywhere on the floor.
2. Drag the cube 3 m. The pink follows, and no ring or line stays where it was.
3. Feature 082's How to test step 6: a cube on the sculpted hill tints the slope beside its lit face.
4. At 2560×1440, with the sun at bounces 1, fly over the hill at editor speed. The frame breakdown holds 60
   fps. New ground fades in without a pop.
