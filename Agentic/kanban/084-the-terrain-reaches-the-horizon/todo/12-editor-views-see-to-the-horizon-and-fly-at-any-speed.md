# 12 — Editor views see to the horizon and fly at a speed the wheel sets
folder: editor
after: 11
decisions: 0168, 0233, 0396
read: feature.md

## Change
0396 point 7.

- `editor/src/view.c`: `FAR_PLANE` 16000 m; the header says why (an 8 km landscape's far corner).
  The fly moves at the view's own speed, starting at `FLY_METRES_PER_SECOND`; Shift still triples.
- `editor/src/view.h`: `voe_editor_view` gains its fly speed; `voe_editor_fly_keys` gains the
  wheel's notches this frame; `voe_editor_views_fly`'s contract: while flying, each notch up
  multiplies the flying view's speed by 1.25 and down divides, clamped to 0.5 .. 1000 m/s, kept
  for that view's next fly.
- `editor/src/frame_pointer.h` / `.c`: the input gains the wheel's notches; `fly` passes them in
  the keys. A flying view takes the wheel; else it goes where it went before.
- `editor/src/main.c` (around line 560): the poll's wheel is handed to the pointer input. The
  comment that a wheel over a scene view is dropped says: unless it is flying.
- `editor/src/src.md`: the view entry names the wheel.

## Done when
The folder's checks build `voe_editor`, and `grep -n 'FAR_PLANE 16000' editor/src/view.c` matches.

Human, feature.md's How to test, on the laptop (the coder does none of these):
1. Create a landscape, set Size 4096 and Cells 2048; sculpt a hill in the middle and ridges near
   the edges.
2. Fly to the hilltop (wheel up for speed) and look round: the edge ridges show, no gaps.
3. Fly slowly from the edge to the hill, low: no jump or crawl as detail changes.
4. Fly fast over it all: smooth; the frame breakdown lists `view window: terrain`.
5. Sculpt near the edge and in the middle: as in 082.
6. Set the scene camera's far plane to 6000 m in the Inspector; Play, then ship: the same view.
