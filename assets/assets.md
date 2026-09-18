# assets

Files on the way in, and a PNG on the way out: bytes from somewhere, engine data
out, and pictures back to bytes. Every reader and writer here is written rather
than fetched (ADR-0023), and nothing in this folder opens a file — `platform`
owns files — so a decoder takes a buffer and an encoder hands one back, and the
caller says where the bytes came from or where they go.

Nothing here knows about Vulkan, a scene or an entity. A decoded picture is
pixels in an arena; what happens to them next is `render`'s.

- `include` — the public headers, in `include/assets/`; each is listed below by path.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See `tests/tests.md`.
- `include/assets/model.h` — `.glb` files, read into vertex attributes,
  materials, decoded pictures and a node tree. Its header says why the binary
  form only, why no coordinate is ever converted while matrix layout is, which
  failures are malformed and which are unsupported, and why animation is ignored
  rather than refused.
- `include/assets/image.h` — pictures, both directions: PNG and JPEG decoded to
  RGBA8, and RGBA8 encoded back as a PNG. Its header says why neither direction
  takes a path now that `platform` has one, why the output is always four
  channels whatever the file held, what the encoder writes and what it refuses,
  and — the one worth reading before touching a texture coordinate — why nothing
  in this engine turns an image the right way up, because PNG, Vulkan and glTF
  already agree that row zero is the top.
- `include/assets/sectioned.h` — the engine's own authored text format:
  `[Section]` headers and `key=value` lines, handed back as text and never
  interpreted. Its header says why it is layer one of three and stops there,
  why comments are `//` and not `#`, and every question the format left open
  with the answer it took.
