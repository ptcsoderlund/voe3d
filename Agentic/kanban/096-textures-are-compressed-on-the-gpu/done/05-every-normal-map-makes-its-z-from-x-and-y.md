# 05 — Every normal map makes its z from x and y
folder: render
after: 04
decisions: 0168, 0278, 0477

## Change
0477 point 1: a BC5 normal map holds x and y alone. Every normal map is read that way, so a map that
waits for compression draws the same as the compressed one.

- `render/shaders/draw.slang` — `voe_render_mapped_normal` takes `m.xy` from the texel's x and y,
  and `m.z = sqrt(saturate(1 − dot(m.xy, m.xy)))`. The blue byte is never read. The comment above it
  says why. Painted landscape layers blend their texels and pass them through this same function
  (`render/shaders/terrain_layers.slangh`). Check that they do, and change nothing there if so.
- `render/tests/surface_maps.c` — a new case: a flat normal map whose blue is 0 reads as none, the
  same as the flat one.
- `render/shaders/shaders.md`, `render/tests/tests.md` — entries, if their words change.

## Done when
The `render/surface_maps` test passes.
