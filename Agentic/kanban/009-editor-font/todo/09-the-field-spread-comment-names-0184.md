# 09 — The field spread comment names the byte-step guard
folder: text/src
decisions: 0168, 0184

## Change
Comment only. In `text/src/raster.h`, the sentence above `#define VOE_TEXT_FIELD_SPREAD` that says
the shader copy's glyph cutoff "measures half a screen pixel (ADR-0183)" becomes: measures half a
screen pixel less one byte step of the field (ADR-0183, ADR-0184); change both or neither.

## Done when
- `grep -n 'ADR-0184' text/src/raster.h` prints a line.
- `cmake --build --preset debug --target voe_text && ctest --test-dir build/debug -R '^text/'` exits 0.
