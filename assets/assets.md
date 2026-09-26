# assets

Files on the way in (glTF, PNG, JPEG, WAV, sectioned text), and a PNG on the
way out: a decoder takes a buffer and an encoder hands one back, since
`platform` owns files (ADR-0023: written, not fetched). Nothing here knows about Vulkan, a scene or an entity.

- `include` — the public headers, in `include/assets/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/assets/model.h` — `.glb` files, read into vertex attributes,
  materials, decoded pictures and a node tree. Its header says why binary only,
  why no coordinate is converted, and which failures are malformed.
- `include/assets/image.h` — pictures, both directions: PNG and JPEG decoded to
  RGBA8, and RGBA8 encoded back as a PNG. Its header says why output is always
  four channels and why nothing turns an image the right way up.
- `include/assets/sectioned.h` — the engine's own authored text format:
  `[Section]` headers and `key=value` lines, handed back as text and never
  interpreted. Its header says why it is layer one of three and why comments
  are `//`.
- `include/assets/sound.h` — sounds: 16-bit PCM and 32-bit float WAV decoded
  to interleaved float. Its header says what is unsupported, what is malformed,
  and why only WAV.
