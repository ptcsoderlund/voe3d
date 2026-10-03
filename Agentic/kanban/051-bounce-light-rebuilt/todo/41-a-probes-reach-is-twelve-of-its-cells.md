# 41 — A probe's reach is twelve of its volume's cells
folder: render
after: 40
decisions: 0168, 0326, 0332

## Change
0332 point 4, for bug 03 of 051: a grid stretched to 8 m keeps probes 8 m apart that must still
see past their neighbours, so the reach scales with the volume's spacing: 24 m at 2 m.
- `render/include/render/device.h`: `VOE_RENDER_BOUNCE_REACH`'s comment: the reach at
  `VOE_RENDER_BOUNCE_SPACING`; a volume's reach is that × its spacing / the finest, twelve of its
  cells. The capture pass's and `voe_render_frame_draw`'s comments that name the reach say "the
  volume's reach".
- `render/src/bounce_capture.c`: each probe's light range and the normal's w where nothing is hit
  are the begun volume's reach. Header to match.
- `render/shaders/draw.slang`: the capture's `VOE_RENDER_BOUNCE_REACH` constant goes; the reach is
  12 × the bounce record's `spacing`, one named constant for the twelve.
- `render/shaders/bounce_relight.slang`: `REACH` goes the same way, from the relight record's
  `spacing`; the no-hit test reads it.
- `render/src/draw.c`: its header's reach phrase to match (the cull already reads the range).
- `render/tests/bounce_capture.c`: a case: a volume at spacing 4 whose first captured probe stands
  about 30 m from a cube: the cube draws once into the capture pass (it drew none at spacing 2,
  the existing far case). Header paragraph.
- `render/tests/bounce_probes_scene.c`: the red wall's lit-side claim (ground within 2 m of the lit
  face redder than ground 10 m off) holds again with the grid at spacing 4 and its cell fitted
  about the scene at that spacing. Header paragraph.
- `render/shaders/shaders.md`, `render/src/src.md`, `render/tests/tests.md`: entries.

## Done when
`ctest --test-dir build/debug -R "^render/bounce_"` passes and
`! grep -n "24\.0" render/shaders/draw.slang render/shaders/bounce_relight.slang` exits 0.
