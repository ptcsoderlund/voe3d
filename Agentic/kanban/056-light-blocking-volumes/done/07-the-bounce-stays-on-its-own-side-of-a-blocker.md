# 07 — The bounce stays on its own side of a blocker
folder: render
after: 06
decisions: 0168, 0326, 0327, 0347

## Change
The relight and the read gate by masks (0347 point 4). Read the headers of
`render/shaders/bounce_read.slangh`, `render/shaders/bounce_relight.slang`,
`render/shaders/lighting.slangh`, `render/shaders/bindings.slangh`, `render/src/light_blockers.h`,
and `render/tests/bounce_probes_scene.c` with the test helpers it uses.

- `render/shaders/blockers.slangh` (new part): the one test of a point against a blocker record
  (sphere first, then rows) and the mask over an array of them, matching `light_blockers.h`; header
  says who includes it and that the C test is its twin.
- `render/shaders/lighting.slangh`: card 03's mask uses that part, and the bounce read is handed
  the surface mask it already has.
- `render/shaders/bounce_read.slangh`: the read takes the surface's mask and weights a probe by
  nought unless the probe position's mask equals it, each including shader supplying its own blocker
  list (draw: the pass's binding 11 region; relight: its record). Header point.
- `render/shaders/bounce_relight.slang`: in `relight`, per texel, its hit point (probe + direction ×
  distance, pushed PUSH along its normal) has a mask; level 1's sun term is nought unless that mask
  is 0 and a lamp's unless the lamp's mask equals it; a texel adds to its probe only when the probe's
  mask equals the texel's; levels above read through the gated read at the texel. Header paragraph.
- `render/tests/blocked_bounce.c` (new), modelled on `render/tests/bounce_probes_scene.c`: the lit
  red wall bouncing onto grey ground at spacing 2; a blocker around a patch of ground beside it,
  the patch reads no red (within 2/255 of bounce off) and ground outside it reads as with no
  blocker; sun off and a bouncing white lamp lighting the wall: with a blocker holding the wall, the
  patch and the lamp the patch reads red, with the lamp moved outside it none; with blockers zeroed
  the picture equals the no-blocker one.
- `render/shaders/shaders.md`, `render/tests/tests.md`: entries for the new part and test; the
  bounce_read, lighting and relight entries name blockers. Each at most 300 characters.

## Done when
The test `render/blocked_bounce` passes, and `render/bounce_probes_scene`, `render/bounce_read`,
`render/bounce_settle` and `render/blocked_light` still pass, after the folder's build.
