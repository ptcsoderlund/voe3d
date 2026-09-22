# 06 — The field spread names its shader copy
folder: text/src
decisions: 0168, 0183

## Change
Comment only. In `text/src/raster.h`, in the comment above `#define VOE_TEXT_FIELD_SPREAD`, add one
sentence: `render/shaders/elements.slang` holds a copy, `VOE_RENDER_ELEMENT_FIELD_SPREAD`, from which
its glyph cutoff measures half a screen pixel (ADR-0183); change both or neither.

## Done when
- `grep -n VOE_RENDER_ELEMENT_FIELD_SPREAD text/src/raster.h` prints a line.
- `cmake --build --preset debug --target voe_text && ctest --test-dir build/debug -R '^text/'` exits 0.
