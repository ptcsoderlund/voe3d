# 30 — The editor's views have room for the relight's sun map
folder: editor
after: 28, 29
decisions: 0168, 0328, 0329
read: feature.md

## Change
0329 point 3, as card 19 did for the capture passes, and the human's proof of bug 01.
- `editor/src/view_passes.h`: in `VOE_EDITOR_CAPACITIES`, each `(VOE_RENDER_SHADOW_CASCADES + 1 +
  VOE_RENDER_BOUNCE_CAPTURE_PASSES)` in `.passes` and `.objects` gains one for the bounce shadow
  pass. The reasoning at the top of the file (the `passes` and per-caster sentences) names it.
- `editor/src/view_passes.c`: the header's sentence on each view's bounce: its probe volume stands
  on the view's eye, and its relight has a sun map of its own.

## Done when
- `grep -qi "bounce shadow\|sun map" editor/src/view_passes.h` exits 0.
- The human, on a hardware card, in the editor and in Play: bug 01's How to reproduce, a house
  with a doorway and a sun at Bounces 1: standing in the doorway and only turning, the room looks
  the same at every angle and never flickers; turned away, move a box elsewhere, turn back: the
  room is as it was. Then `## How to test` steps 2 and 8 of `feature.md` again in
  `examples/tank_game`, watching the top of the screen: if the bounce visibly ends about 20 m ahead,
  report it (0329's known cost).
