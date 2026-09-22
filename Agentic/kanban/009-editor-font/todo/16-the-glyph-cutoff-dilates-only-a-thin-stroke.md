# 16 — The glyph cutoff dilates only a thin stroke

folder: render/shaders
decisions: 0168, 0182, 0183, 0184, 0212, 0177

## Change
Bug 05 of 009: editor text is too heavy wherever a letter lands on few pixels — a stem measures four
screen pixels at a cap height of twenty-three in a 1920x1080 capture. The cause is
`voe_render_element_cutoff` in `render/shaders/elements.slang`, whose dilation is half a screen pixel
at every size (ADR-0184), so every stem comes out about a pixel wider than the face draws it. Only
this file changes; `draw.slang` does not.

- Add a constant beside `VOE_RENDER_ELEMENT_FIELD_SPREAD`:
  `static const float VOE_RENDER_ELEMENT_THIN_STROKE_TEXELS = 2.0;`. Its comment: the thinnest
  stroke the sheet holds, in sheet texels, at `text/src/font.c`'s `ATLAS_EM` of thirty-two texels
  to the em; Oxanium Regular's stem is about 0.08 em, two and a half texels, and the field cannot
  hold a feature under about one and a half; it is copied because render cannot include text, and it
  moves with `ATLAS_EM` and with the face.
- `voe_render_element_cutoff(float2 texels_per_pixel)` keeps its signature and returns, per ADR-0212,
  `clamp(0.5 - dilation * field_per_pixel + 1.0 / 255.0, 1.0 / 512.0, 0.5)` with `field_per_pixel`
  as it is now, `thin_stroke_pixels = VOE_RENDER_ELEMENT_THIN_STROKE_TEXELS / max(texels_per_pixel.x, texels_per_pixel.y)`
  and `dilation = clamp(0.5 * (1.25 - thin_stroke_pixels), 0.0, 0.5)`.
- Rewrite that function's comment and the header paragraph beginning "BUT THE CUT IS NOT AT THE
  FIELD'S HALF": the cut still sits outside the outline, but by only as far as a thin stroke needs
  — the face's thinnest stroke is two texels, `2 / texels_per_pixel` screen pixels, and the cut
  moves out to bring that to a pixel and a quarter, so at about one texel per pixel, which is the
  editor on an ordinary display, it does not move at all and the letters keep the face's weight;
  the margin above one pixel covers the byte-step guard, `step`'s boundary and the constant being
  the face's stem rather than this fragment's stroke; below about 1.6 texels per pixel the dilation
  grows to 0.47 pixel at four, where bug 03's strokes vanished. Keep what is still true: the byte
  step, why it is a whole byte step and not a fraction of a pixel, both clamps, hard `step`, and
  that `draw.slang`'s world text keeps the half. Cite ADR-0212 beside ADR-0182, ADR-0183, ADR-0184.
  Fix any other line in the file that says the cut is half a pixel out.
- Update the `elements.slang` entry in `render/shaders/shaders.md`: the threshold is moved outward
  only as far as a thin stroke needs and not at all where strokes are wide, and it holds copies of
  the field's spread and of the face's thinnest stroke.

## Done when
- `cmake --preset debug && cmake --build --preset debug --target voe_render` exits 0 (slangc compiles
  the shader).
- `ctest --test-dir build/debug -R '^render/'` passes, including `elements.c`'s
  `thin_stroke_keeps_a_pixel` and `aligned_stroke_keeps_its_width`.
- `grep -c 'half a screen pixel' render/shaders/elements.slang render/shaders/shaders.md` prints 0
  for both.
