# 08 — The glyph cutoff guards one byte step of the field
folder: render/shaders
decisions: 0168, 0182, 0183, 0184, 0177

## Change
In `render/shaders/elements.slang`, `voe_render_element_cutoff(float2 texels_per_pixel)` returns,
per decision 0184,
`clamp(0.5 - 0.5 * field_per_pixel + 1.0 / 255.0, 1.0 / 512.0, 0.5)` with
`field_per_pixel = max(texels_per_pixel.x, texels_per_pixel.y) / (2.0 * VOE_RENDER_ELEMENT_FIELD_SPREAD)`.
Nothing else in the fragment stage changes.

Rewrite the comments that say 0.49 (the header paragraph at "THE OUTLINE (ADR-0182, ADR-0183)",
the one above `voe_render_element_cutoff`, and any other `0.49`): the cut sits half a screen pixel
outside the outline less one byte step of the field, because the sheet stores bytes and a rounded
texel can read up to half a step high, so an outline on pixel boundaries must not gain a row
(ADR-0184). Cite ADR-0184 beside ADR-0183. Update the `elements.slang` entry in
`render/shaders/shaders.md` to match if it says 0.49.

## Done when
- `cmake --preset debug && cmake --build --preset debug --target voe_render` exits 0.
- `ctest --test-dir build/debug -R '^render/'` passes.
- `grep -c '0\.49' render/shaders/elements.slang render/shaders/shaders.md` prints 0 for both.
