# 27 — A PNG or WAV refused says why to the artist
folder: assets
after: 26
decisions: 0168, 0443, 0445

## Change
0445 points 2 and 5 for the PNG and WAV readers. The file or picture is the report's subject,
pushed by whoever knows it (card 23's `base/report.h`); these readers never name it.

- `assets/src/png.c` — today `fail` returns a code and prints nothing, so a refused picture is
  silent. Every refusal prints one line through `VOE_BASE_ERROR("assets", …)`, in the shape of
  0445 point 2 with `(PNG: <the format's words>)` after it:
  - a depth not read (1, 2, 4, or a palette at 16): the picture's bit depth, save it as 8-bit;
  - interlaced: save it without interlacing;
  - a side past `VOE_ASSETS_IMAGE_MAX_SIDE`: a limit, width × height and the most a side, scale it
    down; now `VOE_BASE_ERROR_UNSUPPORTED`, no longer malformed;
  - everything else (signature, a chunk past the end, CRC, header shape, palette, data that
    inflates short or long, a filter byte): the picture is damaged, save it again as PNG.
  `fail` takes the words; the header's rule paragraph says each refusal is one line.
- `assets/include/assets/image.h` — `VOE_ASSETS_IMAGE_MAX_SIDE`'s comment: refused as a limit the
  engine keeps (0443), unsupported, not malformed; `voe_assets_png_decode`'s refused list the same,
  and that every refusal prints one line in the artist's words.
- `assets/src/wav.c` — its `wav: %s` lines through the same shape with `(WAV: …)`: a sample format
  not read names it (24-bit PCM, ADPCM, …) and says export as 16-bit PCM or 32-bit float WAV; more
  than two channels is a limit, mix it to stereo; zero channels or rate, no `fmt`/`data`, a short
  chunk, data not whole frames, not RIFF/WAVE: the sound is damaged, export it again.
- `assets/include/assets/sound.h` — its refused paragraph: one line each, in the artist's words.
- `assets/src/src.md` — the `png.c` and `wav.c` entries where they say what is refused.
- `assets/tests/png.c` — new checks on `voe_base_report_error_first()` after a clear: an interlaced
  file says "interlac" and contains `(PNG: `; the existing `huge` case (`HUGE_PNG`) is now
  UNSUPPORTED and its kept line names its size and "16384"; a depth-4 file says "4-bit".
- `assets/tests/wav.c` — new check: a 24-bit PCM file's kept line says "24-bit" and "16-bit".
- `assets/tests/tests.md` — the `png.c` and `wav.c` entries name the wording checks.

## Done when
`ctest --test-dir build/debug -R '^assets/(png|wav)$'` passes with the new checks.
