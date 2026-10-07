# assets

The public headers, one entry each; the fuller account of every one of these,
and the questions each header answers, stays on `assets/assets.md`.

- `model.h` — `.glb` files, read into vertex attributes, materials, decoded
  pictures and a node tree.
- `image.h` — pictures, both directions: PNG and JPEG decoded to RGBA8, and
  RGBA8 encoded back as a PNG.
- `sectioned.h` — the engine's own authored text format: `[Section]` headers and
  `key=value` lines, handed back as text and never interpreted.
- `landscape.h` — a `.landscape` file's text to a grid of heights, and back.
- `sound.h` — sounds: 16-bit PCM and 32-bit float WAV decoded to interleaved
  float.
