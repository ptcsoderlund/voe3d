# 17 — The sheet's numbers name their shader copies

folder: text/src
decisions: 0168, 0212

## Change
Comments only; no code in this folder changes. Card 16 changed what the shader's glyph cutoff does
with the two numbers this folder owns, and added a copy of a second one.

- `text/src/raster.h`, in the comment above `#define VOE_TEXT_FIELD_SPREAD`: the sentence that says
  the shader's copy is what its glyph cutoff "measures half a screen pixel less one byte step of the
  field (ADR-0183, ADR-0184)" now says the cutoff measures from it only as far as a thin stroke needs,
  less one byte step (ADR-0183, ADR-0184, ADR-0212); change both or neither stands as it is.
- `text/src/font.c`, in the comment above `#define ATLAS_EM` (the paragraph beginning "THIRTY-TWO FOR
  OXANIUM"): add that `render/shaders/elements.slang` holds `VOE_RENDER_ELEMENT_THIN_STROKE_TEXELS`,
  the thinnest stroke in texels at this em, and that its glyph cutoff dilates a stroke only until
  that many texels reach a pixel and a quarter on screen (ADR-0212) — so changing this number, or
  the face, changes that one.

## Done when
- `grep -n 'ADR-0212' text/src/raster.h text/src/font.c` prints a line for each file.
- `grep -n 'VOE_RENDER_ELEMENT_THIN_STROKE_TEXELS' text/src/font.c` prints a line.
- `cmake --build --preset debug --target voe_text && ctest --test-dir build/debug -R '^text/'` exits 0.
- `git diff --stat text/` shows `raster.h` and `font.c` only.
