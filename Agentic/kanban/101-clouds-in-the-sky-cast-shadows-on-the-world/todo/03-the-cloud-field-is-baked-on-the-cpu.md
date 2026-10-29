# 03 — The cloud field is baked on the CPU
folder: render
after: 02
decisions: 0168, 0492

## Change
0492 point 4, the bytes only: two tiling noise fields, made the same every run, pure CPU like
`sky.h`. Read the headers of `render/src/sky.h` and `render/src/light_bins.h` (pure CPU files of
this folder) and `render/tests/light_bins.c` (a test of one).

- `render/src/clouds_noise.h` (new): `VOE_RENDER_CLOUDS_WEATHER_SIZE` 256 and
  `VOE_RENDER_CLOUDS_DETAIL_SIZE` 64; `void voe_render_clouds_weather(uint8_t *texels)` filling
  256 × 256 bytes, row by row, and `void voe_render_clouds_detail(uint8_t *texels)` filling 64 ×
  64 × 64, x fastest then y then z. Header points: the weather is the clouds' shapes over
  `VOE_RENDER_CLOUDS_PERIOD` (Perlin fBm whose lattice periods divide the size, so it tiles); the
  detail repeats every 256 m (inverted Worley fBm, cells wrapped, so it tiles), dividing the
  period so a wrapped origin shows no seam; no state, no randomness beyond a fixed hash, so every
  run and every machine gets the same bytes; why baked (0492 point 4); the twin reads are in
  `shaders/clouds.slangh` (a later card).
- `render/src/clouds_noise.c` (new): the two calls, with its header.
- `render/tests/clouds_noise.c` (new), no graphics card: each field made twice is the same bytes;
  each tiles, the largest step across each axis's wrap no larger than the largest step between
  neighbours inside; the weather's darkest texel is under 64 and its brightest over 192; the
  detail's likewise under 64 and over 192.
- `render/src/src.md`, `render/tests/tests.md`: an entry each, at most 300 characters.

## Done when
The test `render/clouds_noise` passes after the folder's build.
