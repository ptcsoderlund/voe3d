# assets

Files on the way in (glTF, PNG, JPEG, WAV, sectioned text), and a PNG on the
way out: a decoder takes a buffer and an encoder hands one back, since
`platform` owns files (ADR-0023: written, not fetched). Nothing here knows about Vulkan, a scene or an entity.

- `include` — the public headers, in `include/assets/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/assets/model.h` — `.glb` files, read into vertex attributes,
  materials, pictures decoded or left encoded, and a node tree. Its header says
  why binary only,
  why no coordinate is converted, and which failures are malformed.
- `include/assets/image.h` — pictures, both directions: PNG and JPEG decoded to
  RGBA8, RGBA8 encoded back as a PNG, a picture shrunk to a largest side, its
  mip chain, its BC7 or BC5 blocks and its cooked form. Its header says why
  BC7 and BC5 and why nothing turns an image the right way up.
- `include/assets/sectioned.h` — the engine's own authored text format:
  `[Section]` headers and `key=value` lines, handed back as text and never
  interpreted. Its header says why it is layer one of three and why comments
  are `//`.
- `include/assets/landscape.h` — a `.landscape` file's text to a grid of
  heights, up to eight material layers, each point's paint weights, the
  foliage kinds with each kind's look, each layer's growth of each kind and the
  copies, and back, and a grid resampled bilinearly.
- `include/assets/material.h` — a `.material` file's text to a shader, its
  factors, repeat and three map paths, and back. Its header says the defaults
  and why a missing key reads as one.
- `include/assets/parts.h` — a `.parts` file's text to the `.material` each
  glTF material of a `.glb` wears and the model's levels, and back. Its header says why the keys are
  indices.
- `include/assets/model_material.h` — a glTF material as a `.material`
  file's values, and the ORM picture Import textures writes for it. Its header
  says why BLEND becomes a cut-out and only a uniform scale becomes repeat.
- `include/assets/sound.h` — sounds: 16-bit PCM and 32-bit float WAV decoded
  to interleaved float. Its header says what is unsupported, what is malformed,
  and why only WAV.
