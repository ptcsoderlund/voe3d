# 18 — The sun reddens the ground by a red box, through 3d
folder: 3d
after: 17
decisions: 0168, 0307, 0308
read: feature.md

## Change
Bug 01 end to end: the screenshot's scene through the calls the editor and
game make, not a hand-fitted grid. Read `3d/tests/bounce.c` and
`3d/tests/shadows.c` (a world, a sun, the shadows call then the view's pass,
pixel lookup), `3d/include/3d/bounce_grid.h`, `3d/src/bounce_grid.c`,
`3d/src/draw_bounce.h` / `.c`, the shadows call's and `voe_3d_frame`'s
comments in `3d/include/3d/draw_system.h`, and `3d/tests/tests.md`.

- `3d/tests/bounce_scene.c`, new, header comment, headless:
  - THE SCENE as `examples/tank_game/main.scene` lights it: beige ground (a
    flat box 60 m wide, top at y 0, base about 0.8, 0.7, 0.5), a red box
    (base 1, 0, 0) 2 m a side at (12, 1, −6), a green one 6 m from it; a sun
    of intensity 9, colour (0.95, 0.71, 0.71), down at 45° onto the red
    box's lit face; fill 0.09. An eye 8 m up and 14 m back from the red box,
    looking at its foot.
  - Ten frames of: frame begin, `voe_3d_draw_system_shadows` with
    `frame.target` the window, the view's pass and draw, frame end.
  - THE TINT: ground 1 m out from the red box's lit face has red over green
    greater than ground 8 m from both boxes by at least 0.03.
  - NO SPOTS: a 7 × 7 lattice of ground points about the boxes, sunlit and
    shadowed, every channel at least the same pixel in a reference frame
    whose shadows call names a second target (so the window's pass reads no
    bounce), less 1/255.
- If a case fails, the 3d owners are `bounce_grid.c` (the fit: the grid
  about the boxes, its light view holding them) and `draw_bounce.c` (the
  pass, casters and update, all about the frame's eye); fix it there and say
  it in the file's header comment. A fault in render (cards 16, 17's cases
  pass but this fails through render) is a block with the measured pixels.
- `3d/tests/tests.md`: the entry for `bounce_scene.c`.

## Done when
The test `3d/bounce_scene` passes, and `3d/bounce`, `3d/bounce_grid` and
`3d/shadows` still pass, after the folder's build.

The human, in the editor on Windows (bug 01, How to reproduce): the scene
of the screenshot shows the ground at the red box's foot visibly redder
than open ground, and no gray spots anywhere.
