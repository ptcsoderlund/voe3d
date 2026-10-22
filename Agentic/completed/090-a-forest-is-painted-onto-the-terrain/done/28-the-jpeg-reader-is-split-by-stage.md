# 28 — The JPEG reader is split by stage
folder: assets
after: 27
decisions: 0168

## Change
`assets/src/jpeg.c` is 836 lines and card 29 changes its refusals throughout. Split it by what each
part does, moving code without changing it, so card 29's coder reads only what it changes.

- New `assets/src/jpeg_read.h` — internal to this folder, as `model_gltf_read.h` is for glTF:
  `struct huffman`, `struct component`, `struct decoder`, the `fail` helper (renamed with the
  `voe_assets_jpeg_` prefix, made non-static), and the one call each part makes into the other.
  Header comment: what the parts share and why the split is by stage.
- New `assets/src/jpeg_scan.c` — the entropy-coded data: the bit reader, `extend`, the Huffman
  decode, one block, `inverse_dct` with `ZIGZAG` and `COS_TABLE`, the scan header and every MCU in
  order. Header comment: the hostile run length and where it is bounded (moved from `jpeg.c`'s
  header), and no recursion.
- `assets/src/jpeg.c` — keeps the markers, the colour conversion and upsample, and
  `voe_assets_jpeg_decode`; its header keeps baseline only and progressive refused, and points to
  `jpeg_scan.c` for the scan.
- `assets/src/src.md` — the `jpeg.c` entry says what stays; new entries for `jpeg_read.h` and
  `jpeg_scan.c`.
- No change to `assets/include/assets/image.h`, to behaviour, or to any message.

## Done when
`ctest --test-dir build/debug -R '^assets/jpeg$'` passes, `wc -l assets/src/jpeg.c
assets/src/jpeg_scan.c` shows each under 600, and
`bash ~/.claude/skills/checks/scripts/checks.sh --header-lines assets/src/jpeg_scan.c` and the
same for `assets/src/jpeg_read.h` print no "over cap".
