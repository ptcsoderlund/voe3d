# 43 — Moving the camera leaves the bounce as it was
folder: 3d
after: 42
decisions: 0168, 0328, 0331, 0332

## Change
The standing test for bug 03 of 051, through the calls the editor and the game make, and the
human's proof in the editor. No code outside the test changes; if a case fails, the fault is in
`3d/src/bounce_grid.c` or `3d/src/draw_bounce.c` (card 42), or render's relight-needed compare
(`render/src/bounce_probes.h`, card 39): report it blocked, naming which claim.
- `3d/tests/bounce_scene.c`, after SETTLING and TURN (read its header first), two cases:
  - MOVE: settled, the eye moved 6 m along −x and 3 m up for a few probing frames, each true (no
    capture pass opens, nothing is relit), then moved back: the lit-side, shadow-foot and five
    even-ground pixels each within 1/255 of their values before the move.
  - FAR: the eye 40 m further back along +z, still looking at the box, for a few probing frames,
    each true; the ground 0.25 m out from the box's lit face, projected to its pixel from there,
    has its red less its green above the bounces-0 reference's at the same eye by at least half of
    TINT/255: distance from the camera does not take the bounce away (0331).
  Header paragraphs for both, naming bug 03 and 0331.
- `3d/tests/tests.md`: the `bounce_scene.c` entry names moving and far.

## Done when
- `ctest --test-dir build/debug -R "^3d/bounce_scene$"` passes.
- The human, on a hardware card: `voe_editor` opens `examples/tank_game`, the sun at Bounces 1 or
  more; in a scene view, the green tank and the two pillars beside it with the tall tan block close
  on the right (as in `03-before.png`, kept in commit a9d40fbe: `git show a9d40fbe:Agentic/kanban/051-bounce-light-rebuilt/bugs/03-before.png > <scratch>.png`); moving the camera a little to the left and back, and
  far away and back, the shaded sides of the tank and pillars keep their brightness, and stay so
  after a few seconds at each place; moving the camera never makes the picture busy (no fan, no
  stutter), as when it stands still.
