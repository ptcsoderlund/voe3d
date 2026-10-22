# 29 — A JPEG refused says why to the artist
folder: assets
after: 28
decisions: 0168, 0443, 0445

## Change
0445 points 2 and 5 for the JPEG reader, as card 27 did for PNG (read `assets/src/png.c`'s `fail`
for the shape). The picture or file is the report's subject, pushed by whoever knows it; the
reader never names it.

- `assets/src/jpeg_read.h`, `assets/src/jpeg.c`, `assets/src/jpeg_scan.c` — every refusal prints one
  line through `VOE_BASE_ERROR("assets", …)`, then `(JPEG: <the format's words>)`:
  - progressive, arithmetic-coded, lossless or 12-bit: what it is, save it as a baseline JPEG
    (untick Progressive) or as PNG;
  - a side past `VOE_ASSETS_IMAGE_MAX_SIDE`: a limit, width × height and the most a side, scale it
    down, `VOE_BASE_ERROR_UNSUPPORTED`;
  - a component count or sampling not read: what it is, save it again as an RGB or greyscale JPEG,
    or as PNG;
  - every other refusal (a marker past the end, a table index, a run past a block, a bad Huffman
    code, a missing table or frame): the picture is damaged, save it again.
  Where `fail` already printed a line, it prints this one instead; one line per refusal.
- `assets/include/assets/image.h` — `voe_assets_jpeg_decode`'s refused list: one line each, in the
  artist's words; the side limit unsupported.
- `assets/src/src.md` — the JPEG entries, where they say what is refused.
- `assets/tests/jpeg.c` — new checks on `voe_base_report_error_first()` after a clear: a
  progressive (SOF2) file says "baseline" and contains `(JPEG: `; a file cut short in its scan
  says to save it again. Old wording checks updated.
- `assets/tests/tests.md` — the `jpeg.c` entry names the wording checks.

## Done when
`ctest --test-dir build/debug -R '^assets/jpeg$'` passes with the new checks.
