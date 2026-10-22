# 75 — A landscape file holds each kind's base size
folder: assets
after: none
decisions: 0168, 0442, 0459, 0460

## Change
0460 points 1–2: `[Foliage]` carries a base size per kind.

- `assets/include/assets/landscape.h` —
  - `VOE_ASSETS_LANDSCAPE_KIND_SIZE_MIN` (100) and `_MAX` (10000): thousandths, the file's
    range and the editor's slider's.
  - `voe_assets_landscape` gains, after `kind_count`, `uint16_t
    kind_sizes[VOE_ASSETS_LANDSCAPE_KINDS]`: thousandths, 0 meaning 1×; its comment says so.
  - `float voe_assets_landscape_kind_size(const voe_assets_landscape *, uint32_t kind)`: the
    kind's base size as a factor, 1 for 0; asserts kind below VOE_ASSETS_LANDSCAPE_KINDS.
  - `_flat` leaves every size 0; `_resample` carries them; `_read` and `_write` as below; their
    comments name the new MALFORMED and the omitted line.
  - Header points: the example's `[Foliage]` shows a `size0=`; a kind's size is a factor on
    every copy of it, 0 and a missing key 1×. The header comment is at 59 of 60 lines: make
    room by rewording, never past 60 (measure with
    `bash ~/.claude/skills/checks/scripts/checks.sh --header-lines <file>`).
- `assets/src/landscape_foliage.c`, `assets/src/landscape_foliage.h` — read: for each kind k
  read, an optional `size<k>`, a whole number in the range above; MALFORMED when not a whole
  number or out of range; missing leaves 0. Print: `size<k>=<n>` on the line after `kind<k>`
  when the size is neither 0 nor 1000. Header line follows.
- `assets/src/landscape.c` — `_resample` copies `kind_sizes` beside the kinds.
- `assets/tests/landscape.c` — new cases: a grid with two kinds, kind 1 at `size1=2000`, and a
  copy round-trips byte for byte, reads `kind_size` 1 for kind 0 and 2 for kind 1; a kind at
  1000 is written without its `size` line; `size0=0`, `size0=99`, `size0=10001` and
  `size0=1.5` each MALFORMED; a resample keeps the sizes; `_flat` with a kind added by hand
  gives 1.
- `assets/assets.md`, `assets/src/src.md`, `assets/tests/tests.md` — the landscape and
  landscape_foliage entries name the base size.

## Done when
Test `assets/landscape` passes with the cases above.
