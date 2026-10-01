# 19 — A lit face on a probe plane still tints the ground beside it
folder: render
after: none
decisions: 0168, 0307, 0308, 0309

## Change
Card 16's blocked case: in `render/bounce_scene`, OFF THE ORIGIN reads a
red/green lift of 0.031 where 0.05 is asked, because the wall's red face lies
on a probe plane (centres at odd world metres) and that probe sees none of it.
Decision 0309 is the fix: the gather measures each VPL from half a cell behind
its surface. Read `render/shaders/bounce.slang`, `render/shaders/shaders.md`
and the header comment of `render/tests/bounce_scene.c`.

- `render/shaders/bounce.slang`, `gather`: before the distance and direction
  toward a probe, move the VPL's position back along its normal, normalized,
  by half the push block's `spacing`. Flux, the cosine, the 1 m² floor and the
  1 mm skip stay, measured from the moved position. A VPL whose normal is too
  short to normalize is not moved. The header comment's formula gains the
  move and cites ADR-0309.
- `render/shaders/shaders.md`: the `bounce.slang` entry says the gather's
  sources sit half a cell behind their surfaces.
- `render/tests/bounce_scene.c`: no change to the case or its 0.05; the wall
  stays at x = −71 on purpose (a face on a probe plane). The header comment
  says so in a phrase.

If OFF THE ORIGIN still fails with the move in place, block with the measured
pixels near and far.

## Done when
The tests `render/bounce_scene`, `render/bounce`, `render/bounce_grid`,
`render/bounce_map` and `render/bounce_schedule` pass after the folder's
build.
