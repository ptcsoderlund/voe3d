# 01 — A landscape file holds foliage
folder: assets
after: none
decisions: 0168, 0442

## Change
0442 point 1: a `.landscape` names foliage kinds and holds every copy.

- `assets/include/assets/landscape.h` —
  - `VOE_ASSETS_LANDSCAPE_KINDS` (16), `VOE_ASSETS_LANDSCAPE_COPIES` (65536) and
    `VOE_ASSETS_LANDSCAPE_KIND_PATH` (128, as a model row's path).
  - `voe_assets_landscape_copy`: `int32_t x, z` (millimetres, grid's own space), `uint16_t turn`
    (1/65536 turn about +Y), `uint16_t scale` (thousandths, 1–65535), `uint8_t kind`, `uint8_t
    lean` (0–255), in that order, so a cooked initializer `{x, z, turn, scale, kind, lean}`
    fills it. Compare copies field by field, never with memcmp (padding).
  - `voe_assets_landscape` gains, at its end, `kinds[VOE_ASSETS_LANDSCAPE_KINDS]
    [VOE_ASSETS_LANDSCAPE_KIND_PATH]`, `kind_count`, `voe_assets_landscape_copy *copies` and
    `copy_count`. `_flat` leaves none (copies NULL); `_resample` carries kinds and copies
    unchanged (copied onto its arena).
  - `_read`: `[Foliage]` `kind0=`… quoted paths to the first missing key; `[Copies]` keys
    walked once in file order (as `landscape_paint.c` walks rows, never looked up by name), the
    i-th named exactly `c<i>`, its value six whole numbers as above. MALFORMED: an unquoted or
    too long kind path, a key out of order or misnamed, a wrong count of numbers, a kind not
    below kind_count, turn, scale or lean out of range, scale 0. UNSUPPORTED past
    VOE_ASSETS_LANDSCAPE_COPIES copies. No `[Copies]` is none.
  - `_write`: `[Foliage]` when kind_count > 0, `[Copies]` when copy_count > 0, one line a copy.
  - Header points: the two sections and the example extended, why no height (stands on the
    ground), why keys are walked. The header is at 49 of 60 lines: keep it under the cap.
- `assets/src/landscape_foliage.c` and `assets/src/landscape_foliage.h` — new, internal, beside
  `landscape_paint.*` and shaped as them: the two sections' read and print, each with its header.
  `assets/src/landscape.c` calls them.
- `assets/tests/landscape.c` — new cases: a 4-cell grid with two kinds and three copies (one
  negative x, one lean 255, one scale 65535) round-trips byte for byte and reads back each
  field; a file with no `[Copies]` reads copy_count 0; `c1` before `c0`, a kind 2 with two
  kinds, a scale 0 and a row of five numbers each MALFORMED; a resample keeps the copies.
- `assets/assets.md`, `assets/src/src.md`, `assets/tests/tests.md` — landscape entries mention
  foliage; entries for the new files.

## Done when
Test `assets/landscape` passes with the cases above.
