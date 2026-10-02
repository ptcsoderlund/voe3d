# 16 — The editor's views have room for the capture passes
folder: editor
after: 12, 15
decisions: 0168, 0326
read: feature.md

## Change
0326 points 3 and 8, as card 15 did for the game. The Inspector needs nothing: Bounces is a
dropdown of "0" to "3" and Bounce strength a number on the sun and on a point light (card 01).
- `editor/src/view_passes.h`: in `VOE_EDITOR_CAPACITIES`, each `(VOE_RENDER_SHADOW_CASCADES + 2)`
  in `.passes` and `.objects` becomes `(VOE_RENDER_SHADOW_CASCADES + 1 +
  VOE_RENDER_BOUNCE_CAPTURE_PASSES)`: the point-shadow pass and the capture passes, no bounce
  map pass. The reasoning at the top of the file (lines naming the bounce map and the bounce
  pass) says the capture passes instead.
- `editor/src/view_passes.c`: the header's sentence on each view's bounce: its target's probe
  volume, begun, captured and relit by the shadows call.
- `editor/src/capture.h`: the `--frames` sentence: more frames let the probe volume build and
  capture its probes.

## Done when
- `grep -q "VOE_RENDER_BOUNCE_CAPTURE_PASSES" editor/src/view_passes.h` exits 0 and
  `! grep -n "bounce map\|bounce pass" editor/src/view_passes.h` exits 0.
- The human, on a hardware card, in the editor and in Play: `## How to test` steps 1–10 of
  `feature.md`, step 1's before shots taken on a build of `main`.
