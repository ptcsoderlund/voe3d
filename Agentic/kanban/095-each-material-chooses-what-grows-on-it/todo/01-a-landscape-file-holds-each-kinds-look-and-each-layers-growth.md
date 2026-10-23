# 01 — A landscape file holds each kind's look and each layer's growth
folder: assets
after: none
decisions: 0168, 0473, 0475, 0476

## Change
0476 points 1 and 2 in the `.landscape` text. Changes the public struct: 3d, game and editor
follow in their own cards.

- `assets/include/assets/landscape.h` —
  - New `voe_assets_landscape_look`: `uint16_t size` (thousandths, 0 meaning 1×, as `kind_sizes`
    was), `uint8_t size_vary` (%, 0–100), `uint8_t turn_vary` (degrees, 0–180), `uint8_t tilt`
    (%, 0–100), `bool shadowless`. Constants for the spreads' maxima and defaults (20, 180, 0) and
    `VOE_ASSETS_LANDSCAPE_GROW_MAX` 500.
  - `voe_assets_landscape`: `kind_sizes` becomes `voe_assets_landscape_look looks[KINDS]`; new
    `uint16_t grows[LAYERS][KINDS]`, layer l's density of kind k in tenths of copies per 100 m²,
    0 not grown.
  - New `voe_assets_landscape_look voe_assets_landscape_look_default(void)`: size 0, 20 %, 180°,
    0 %, casting.
  - `voe_assets_landscape_kind_size` reads `looks[kind].size`.
  - Header: the sketch gains `vary0=`, `turn0=`, `tilt0=`, `shadow0=0` under [Foliage] and
    `grow1_0=40` under [Layers]; points: a missing look key is the default; `shadow<k>` written
    only when off, `grow<l>_<k>` only above 0, look keys only when not the default; the choice
    is the layer row's, so a row's new material keeps it (0476 point 2).
  - `_flat`: no kinds, every grow 0. `_resample` carries looks and grows. `_read`'s MALFORMED
    list gains: a spread past its maximum or not a whole number, `shadow` other than 0 or 1, a
    grow key whose layer is not below layer_count or kind not below kind_count, or a density not
    1 to GROW_MAX. `_write` writes them as above.
- `assets/src/landscape_foliage.h`, `assets/src/landscape_foliage.c` — the look keys read and
  written beside `size<k>`; the grow keys read after both [Layers] and [Foliage] are known and
  written in [Layers] after the layer paths (the private header's call says where); the default
  look; kind_size from `looks`.
- `assets/src/landscape.c` — `_resample` copies `looks` and `grows`; `_flat` zeroes `grows`; the
  [Layers] writer calls the grow writer.
- `assets/assets.md` — the landscape.h entry names each kind's look and each layer's growth.
- `assets/src/src.md` — the two files' entries follow.
- `assets/tests/landscape.c` — `kind_sizes` uses become `looks[k].size`; new cases: a file with
  two kinds' looks and two layers' grows read, written and read back equal; a file with no look
  keys reads every kind at the default and writes no look line; each new MALFORMED case refused;
  a resample keeps looks and grows.

## Done when
The `assets/landscape` test passes with the new cases.
