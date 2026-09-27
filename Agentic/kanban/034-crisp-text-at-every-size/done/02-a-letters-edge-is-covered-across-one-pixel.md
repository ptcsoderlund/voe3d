# 02 — A letter's edge is covered across one pixel
folder: render
decisions: 0168, 0269, 0212, 0183, 0069

## Change
034: small text is grainy because an interface glyph's pixel is either in or out. Replace the
hard cut with 0269's coverage. `render/src` does not change; the blend is already premultiplied.

- `render/shaders/elements.slang`: `voe_render_element_cutoff` becomes
  `float voe_render_element_coverage(float median, float2 texels_per_pixel)`, returning 0269's
  ramp: `field_per_pixel` and `dilation` exactly as now, `edge`, `low`, `high`, the linear ramp.
  The glyph branch multiplies alpha by it instead of `step(...)`. Rewrite the header paragraphs
  "A THRESHOLD, NOT A SMOOTHSTEP" and "BUT THE CUT IS NOT AT THE FIELD'S HALF", the function's
  comment and the in-body comments ("A hard cut…", "Inside or outside, and nothing in between")
  to say: coverage is a box filter over one screen pixel read from the distance (0269); the edge
  still moves out only for a thin stroke (0212) so small text keeps its ink; why nought texels
  stay uncovered; large text at about one texel per pixel is nearly hard; world text in
  `draw.slang` keeps its hard cut. Drop what only the byte-step guard needed. The derivative
  paragraph stands with "cutoff" read as "coverage".
- `render/shaders/draw.slang`: only the two comments saying the engine has no antialiasing
  anywhere (the header's "THE STEP IS HARD…" and the body's "A HARD CUT AT THE OUTLINE…") — world
  text is hard, interface text is covered (0269). No code change.
- `render/include/render/device.h`: the comments that say a glyph's field is thresholded (around
  the field sampling note, the GLYPH kind, and a glyph with no sheet) say it is turned into a
  coverage; a white sheet is still fully covered, so no sheet still draws a solid rectangle.
  Comments only.
- `render/shaders/shaders.md`: the `elements.slang` entry — coverage across one pixel around an
  edge that moves out for a thin stroke, not a cut at a threshold.
- `render/tests/glyphs.c` (from card 01): the stroke claims measure ink instead of counting rows.
  Add a helper turning a read byte back into linear coverage (decode sRGB if the target's format
  encodes it; `read_back`'s comment or `element_scene.h` says which). Claims, each with a header
  comment citing 0269:
  - `thin_stroke_keeps_its_ink` (replaces `thin_stroke_keeps_a_pixel`): the half-pixel bar at four
    texels a pixel sums to at least one pixel of coverage in every column.
  - `aligned_stroke_stays_sharp` (replaces `aligned_stroke_keeps_its_width`): the eight-texel bar
    at one texel a pixel has rows 4–11 at least 0.95 covered and rows 3 and 12 at most 0.05.
  - `a_wide_stroke_keeps_its_width` (replaces `a_wide_stroke_gains_no_row`): the four-texel bar a
    quarter pixel off sums to 4 ± 0.15 pixels per column.
  - new `an_edge_between_pixel_centres_is_partly_covered`: in that same bar, some row lies
    strictly between 0.2 and 0.8 coverage.
  The other glyph claims keep their meaning; where one counted pixels on an edge that is now
  partly covered, count pixels at least a pixel away from it instead, never loosen anything else.
  Update the bar-sheet paragraph and the `glyphs.c` entry in `render/tests/tests.md`.

## Done when
- `cmake --preset debug && cmake --build --preset debug --target voe_render $(ninja -C build/debug -t targets all | grep -oE "^voe_test_render_[A-Za-z0-9_]+")` exits 0.
- `ctest --test-dir build/debug -R '^render/'` passes, including the four claims above.
- `grep -c 'voe_render_element_cutoff\|step(' render/shaders/elements.slang` prints 0.
