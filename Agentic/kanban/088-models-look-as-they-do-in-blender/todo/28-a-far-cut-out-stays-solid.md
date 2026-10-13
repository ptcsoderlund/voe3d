# 28 — A far cut-out stays solid
folder: render
after: 27
decisions: 0168, 0359, 0421

## Change
0421 point 3, in the shader alone.

- `render/shaders/draw.slang`: in the cutout branch, before the compare with `alpha_cutoff`, the
  coverage is multiplied by 1 + 0.25 × max(0, the base colour texture's level of detail at `base_uv`)
  (Slang's level-of-detail query on that texture). A distance-field record keeps its cut as now. The
  ALPHA paragraph of the header says why (mips average thin opaque texels below the cut, so leaves
  thin away; the factor puts back what averaging took, Ben Golus' method), and that it is approximate.
- `render/shaders/shaders.md`: the `draw.slang` entry mentions it if it fits in 300 characters.
- `render/tests/cutout_far.c` (new, headless; read `render/tests/mips.c` for a SMOOTH texture made and
  read at a distance): a 256² COLOUR SMOOTH texture of one-texel stripes, alpha 1 and 0 alternating, on a
  cutout record, cutoff 0.5. Drawn so it fills the picture, about half its pixels are drawn; drawn so
  small the texture reads at level 4 or more, at least a third of its pixels are drawn. Name the counts
  and the bounds in the test.
- `render/tests/tests.md`: an entry (at most 300 characters).

## Done when
The test `render/cutout_far` passes after the folder's build.
