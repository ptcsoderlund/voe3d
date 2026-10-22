# 26 — Blender's texCoord −1 and 16-bit PNGs are read
folder: assets
after: 23
decisions: 0168, 0446

## Change
0446: the two things in the bug's fir tree the reader refuses today.

- `assets/src/model_gltf_materials.c` — a texture reference's `texCoord`: a negative number reads as
  the reference being absent (the slot gets `VOE_ASSETS_MODEL_NONE`, as with no reference), with no
  line; 0 or absent as today; a whole number above 0 stays refused as another set; anything else
  stays malformed. Today it goes through `voe_assets_gltf_uint_member`, which refuses −1; read the
  member's number first. The file's header: one line on Blender's −1 (0446 point 1).
- `assets/include/assets/model.h` — the header's refused list: a negative `texCoord` reads as no
  texture (0446).
- `assets/src/png.c` — bit depth 16 in colour types 0, 2, 4 and 6: the row and filter arithmetic at
  two bytes a sample, each sample rounded to 8 bits as 0446 point 2 says on the way to RGBA8.
  `tRNS` stays read for palettes only, as today. Depths 1, 2 and 4, and palette at 16, stay
  refused as unsupported. The header's numbered rule about lengths holds for the doubled row size.
- `assets/include/assets/image.h` — `voe_assets_png_decode`'s comment: depths 8 and 16 are read,
  16 rounded to 8 (0446); the refused list keeps interlace and depths below 8.
- `assets/tests/model.c` (and `model_data.inc` if its glb builder lives there) — new check: a
  material whose base colour reference has `"texCoord": -1` reads, its base colour texture `NONE`,
  the rest of the material as written.
- `assets/tests/png.c` (and `png_data.inc` if bytes live there) — new checks: a 2 × 1 RGBA PNG at
  depth 16 with samples 0x0000, 0xFFFF, 0x807F and 0x7F80 decodes to 0, 255, 128 and 127; a 16-bit
  greyscale PNG decodes to grey RGBA with alpha 255; a depth-4 file is still refused as unsupported.
  Build the files in the test: one stored deflate block in a zlib wrapper, the CRC through
  `../src/png_crc.h` by relative path as `tests/deflate.c` reaches into `src`.
- `assets/tests/tests.md` — the `model.c` and `png.c` entries name the new cases.

## Done when
`ctest --test-dir build/debug -R '^assets/(model|png)$'` passes with the new checks.
