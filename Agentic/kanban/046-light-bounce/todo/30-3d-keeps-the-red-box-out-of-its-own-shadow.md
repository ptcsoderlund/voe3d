# 30 — 3d keeps the red box's colour out of its own shadow
folder: 3d
after: 29
decisions: 0168, 0310, 0312, 0313, 0314

## Change
Bug 03 through 3d's calls: cards 27 and 29 keep a face's bounce on its lit side in
render; this holds it where the editor draws. Read `3d/tests/bounce_scene.c`
and `3d/tests/tests.md`.

- `3d/tests/bounce_scene.c`, case SHADOW SIDE, at both suns, in the frames
  THE TINT already draws: ground 0.25 m and 0.75 m out from the red box's
  shadowed −x face at the box's z, which the sun does not reach, reads no
  redder than the same pixel in the reference frame (no bounce): its red
  minus its green at most the reference's plus 1/255. Only the red box's
  colour is barred: the green box's lit face looks into that shadow, and
  0312 lets it light it. The header gains the case and says why the claim is
  the colour, not the brightness.
- THE TINT and NO SPOTS keep their claims at the gain card 29 chose; if THE
  TINT fails while card 29's tank captures met 0310, block with the measured
  pixels.
- `3d/tests/tests.md`: the `bounce_scene.c` entry names the shadow side.

## Done when
The tests `3d/bounce_scene`, `3d/bounce`, `3d/bounce_grid` and
`3d/shadows` pass after the folder's build.

The human, in the editor on this machine (bug 03, How to reproduce; bug 02;
feature.md How to test 1–3): the tank project at sun π and at 1, the purple
box's shadow right beside it is no brighter and no more purple than the same
shadow further out, its sunlit foot is still plainly purple, fading with
distance, and there are no dark spots.
