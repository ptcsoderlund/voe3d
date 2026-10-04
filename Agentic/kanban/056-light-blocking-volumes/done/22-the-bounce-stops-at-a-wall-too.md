# 22 — The bounce stops at a Wall too
folder: render
after: 20, 21
decisions: 0168, 0326, 0327, 0350

## Change
The bounce read and the relight gate by 0350 point 5, using card 21's `voe_render_blockers_pass`.
Read the headers of `render/shaders/blockers.slangh`, `render/shaders/bounce_read.slangh`,
`render/shaders/bounce_relight.slang`, `render/shaders/lighting.slangh`, and
`render/tests/blocked_bounce.c` with the helpers it uses.

- `render/shaders/bounce_read.slangh`: the read takes the walls and indoors words beside the
  surface's mask; a probe's weight is nought unless `_blockers_pass` with the probe's mask as
  source, the surface as p and the probe's position as s. Header point replaces 0347's.
- `render/shaders/lighting.slangh`: hands the read the region's words.
- `render/shaders/bounce_relight.slang`: at a texel's hit (pushed PUSH along its normal) level 1's
  sun by `_blockers_pass` with the record's `sun` and the sun's ray, and a lamp by it with the
  lamp's mask and position; the texel feeds its probe only when `_blockers_pass` passes with the
  probe as source and the hit as p; level k reads through the gated read. Header paragraph.
- `render/tests/blocker_kinds_bounce.c` (new), modelled on `render/tests/blocked_bounce.c`: the lit
  red wall bouncing onto grey ground at spacing 2;
  1. a Wall slab standing between the red wall and a patch of ground: once settled the patch reads
     within 2/255 of bounce off; ground on the red wall's side still reads red;
  2. an Indoors box around that patch instead: the patch reads as with no blocker within 2/255;
  3. the three words zero with a Room around the patch: blocked_bounce's matching picture.
- `render/tests/blocked_bounce.c`: where a sample now lies where a probe's segment meets the box,
  the sample is moved so it does not, and the header says why; its claims otherwise stand.
- `render/shaders/shaders.md`, `render/tests/tests.md`: the entries for what changed and the new
  test. Each at most 300 characters.

## Done when
The test `render/blocker_kinds_bounce` passes, and `render/blocked_bounce`,
`render/bounce_probes_scene`, `render/bounce_settle` and `render/blocker_kinds` still pass, after
the folder's build.
