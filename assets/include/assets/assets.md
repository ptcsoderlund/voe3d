# assets

The public headers, one entry each; the fuller account of every one of these,
and the questions each header answers, stays on `assets/assets.md`.

- `model.h` — `.glb` files, read into vertex attributes, materials, pictures
  decoded or left encoded, and a node tree.
- `image.h` — pictures, both directions: PNG and JPEG decoded to RGBA8,
  RGBA8 encoded back as a PNG, and a picture shrunk to a largest side.
- `sectioned.h` — the engine's own authored text format: `[Section]` headers and
  `key=value` lines, handed back as text and never interpreted.
- `landscape.h` — a `.landscape` file's text to a grid of up to 2048 cells of
  heights, its paint and its foliage copies, and back, and a grid resampled to
  another count.
- `material.h` — a `.material` file's text to a shader, its factors and three
  map paths, and back.
- `model_material.h` — a glTF material as a `.material` file's values, and the
  ORM picture Import textures writes for it; opens no file.
- `parts.h` — a `.parts` file's text to the `.material` each glTF material of a
  `.glb` wears, and back.
- `sound.h` — sounds: 16-bit PCM and 32-bit float WAV decoded to interleaved
  float.
