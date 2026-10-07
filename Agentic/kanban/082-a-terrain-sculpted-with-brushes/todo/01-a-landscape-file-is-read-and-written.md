# 01 — A landscape file is read and written
folder: assets
after: none
decisions: 0168, 0379

## Change
A `.landscape` file's text to a grid of heights and back (0379 point 1). Pure CPU, no file opened.

- New `assets/include/assets/landscape.h`:
  - `VOE_ASSETS_LANDSCAPE_CELLS` (512, what Create writes), `_SIZE_MIN` (16), `_SIZE_MAX` (8192),
    `_SIZE_DEFAULT` (256).
  - `voe_assets_landscape` — `float size` (metres a side), `uint32_t cells` (per side), `float *heights`
    ((cells + 1)² metres, row-major, row r at z = −size/2 + r·size/cells, column c at x likewise).
  - `voe_assets_landscape voe_assets_landscape_flat(float size, uint32_t cells, voe_base_arena *arena)` —
    every height 0; asserts size in range and cells a multiple of 4 from 4 to 512.
  - `[[nodiscard]] bool voe_assets_landscape_read(const char *text, size_t size, voe_base_arena *arena,
    voe_assets_landscape *out, voe_base_error *error)` — read with `assets/sectioned.h`: `[Landscape]`
    `size=` and `cells=`, `[Heights]` `row0=` … `row<cells>=`, each `cells + 1` whole millimetres
    separated by blanks. MALFORMED for text the sectioned reader refuses, a missing key or row, a row
    with too few or too many numbers or one not a whole number; UNSUPPORTED for size out of range or
    cells not a multiple of 4 from 4 to 512. A line on stderr names what, as the sectioned reader does.
    `out` untouched on failure.
  - `voe_assets_landscape_text` — `const char *text; size_t size`.
  - `voe_assets_landscape_text voe_assets_landscape_write(const voe_assets_landscape *landscape,
    voe_base_arena *arena)` — the text the reader takes, each height rounded to the nearest millimetre;
    cannot fail.
  - Header points: what the file is and where each height lies; why one file and no picture beside it
    (0379 point 1); millimetres so the text is exact and short; the game never reads it (0236), the cook
    does; the arena holds the result and working memory.
- New `assets/src/landscape.c` — carries those out.
- New `assets/tests/landscape.c` — `flat_written_reads_back_the_same`,
  `heights_round_to_the_millimetre`, `a_short_row_is_malformed`, `a_missing_row_is_malformed`,
  `cells_not_a_multiple_of_four_is_unsupported`, `a_size_out_of_range_is_unsupported`.
- `assets/assets.md` gains the `landscape.h` entry; `assets/src/src.md` and `assets/tests/tests.md` their
  entries; each a phrase.

## Done when
`ctest --test-dir build/debug -R '^assets/landscape'` passes with the six tests above.
