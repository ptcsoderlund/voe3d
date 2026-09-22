# 05 — The element glyph cutoff keeps every stroke
folder: render/shaders
decisions: 0168, 0182, 0183, 0177

## Change
Bug 03 of 009: at window heights where text does not line up with the screen, thin strokes of
editor text lose every pixel (the "T" of "Theme's own" vanishes). The owner is the one threshold
every editor letter passes through, `colour.a *= step(0.5, voe_render_element_median(field))` in
`elements.slang`'s fragment stage. Fix it there only; `draw.slang` does not change.

- Add `static const float VOE_RENDER_ELEMENT_FIELD_SPREAD = 4.0;` with a comment saying it must equal
  `VOE_TEXT_FIELD_SPREAD` in `text/src/raster.h` (render cannot include text) and what it is: the
  field's encoded range in texels either side of the outline.
- Add `float voe_render_element_cutoff(float2 texels_per_pixel)`: returns
  `max(0.5 - 0.49 * max(texels_per_pixel.x, texels_per_pixel.y) / (2.0 * VOE_RENDER_ELEMENT_FIELD_SPREAD), 1.0 / 512.0)`
  — the field value at which a pixel counts as inside, per decision 0183.
- In the fragment stage, before the branch on the record's kind (derivatives need uniform control
  flow), compute `texels_per_pixel = fwidth(<the interpolated sheet coordinate>) * <sheet size in
  texels>`. Take the sheet size from the glyph's texture with `GetDimensions` inside the branch, or
  compute the derivative of the sheet coordinate before the branch and multiply by the size inside
  it; either way no `ddx`/`ddy`/`fwidth` sits inside the branch. The glyph branch becomes
  `step(voe_render_element_cutoff(texels_per_pixel), median)`. Still `step`: no partial coverage.
- Rewrite the header paragraph "A THRESHOLD, NOT A SMOOTHSTEP" to say the cut is hard but no longer at
  0.5: it sits 0.49 screen pixel outside the outline so no stroke loses all its pixels (ADR-0182,
  ADR-0183), and why 0.49 and not 0.5. Add to the "SAMPLED AT AN EXPLICIT LEVEL" paragraph that the one
  derivative the glyph needs is taken before the branch. Fix the in-body comment "A hard cut at the
  half, which is draw.slang's answer" to match.
- Update the `elements.slang` entry in `render/shaders/shaders.md`: the threshold is a hard cut moved
  outward by under half a pixel, and that it holds a copy of the field's spread.

## Done when
- `cmake --preset debug && cmake --build --preset debug --target voe_render` exits 0 (slangc compiles
  the shader).
- `ctest --test-dir build/debug -R '^render/'` passes: the existing glyph claims in `render/tests/elements.c`
  still hold.
- `grep -c 'ddx\|ddy\|fwidth' render/shaders/elements.slang` is at least 1 and none of them is inside
  the `if (vertex.kind == VOE_RENDER_ELEMENT_GLYPH)` block (read it).
