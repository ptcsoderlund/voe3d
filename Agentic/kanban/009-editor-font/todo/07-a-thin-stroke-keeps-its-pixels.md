# 07 — A thin stroke keeps its pixels, an aligned one keeps its width
folder: render/tests
decisions: 0168, 0182, 0183, 0177

## Change
Two claims added to `render/tests/elements.c`, headless, using the hand-made sheet and the glyph
drawing that file already has (read its header for how a sheet is made and a glyph placed in
pixels). Each makes its own sheet: a horizontal bar, every texel's three channels equal to
`0.5 + (half_width - |y - centre|) / (2 * 4)` clamped to 0..1 and stored as a byte (4 is the spread).

- `thin_stroke_keeps_a_pixel`: a bar 2 texels tall, drawn so one screen pixel spans 4 texels and
  the bar's centre lies exactly on the boundary between two pixel rows (so no pixel centre is inside
  the outline; with the old 0.5 cut nothing would be drawn). Read back: every column across the
  glyph's width has at least one pixel of the glyph's colour.
- `aligned_stroke_keeps_its_width`: a bar 8 texels tall drawn at one texel per pixel with its edges
  on pixel boundaries. Read back: exactly 8 rows in the glyph's colour in the middle column, not 9
  (the cutoff sits under half a pixel out).

Add both to the `elements.c` entry in `render/tests/tests.md`.

## Done when
- `cmake --build --preset debug --target voe_render && ctest --test-dir build/debug -R '^render/'`
  passes with both new claims in `elements.c`.
- Reverting card 05's glyph line to `step(0.5, …)` locally makes `thin_stroke_keeps_a_pixel` fail
  (check once, then restore).
- `checks.sh --all` exits 0.

For the human, with `./build/debug/editor/voe_editor` on the sponsor's display:
1. Choose Oxanium in Preferences. Shrink the window in steps: at no height does any letter lose a
   stroke; the "T" of "Theme's own", "Close" and "Oxanium (in force)" stay whole. A stroke one pixel
   wider than another is accepted.
2. Text still shrinks smoothly with the window; edges stay hard, no grey.
3. Choose Pixel Operator and repeat 1 and 2.
4. At full screen, both fonts look no worse than before this card.
