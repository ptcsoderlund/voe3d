# assets

Files on the way in, and a PNG on the way out: bytes from somewhere, engine data
out, and pictures back to bytes. Every reader and writer here is written rather
than fetched (ADR-0023), and nothing in this folder opens a file — `platform`
owns files — so a decoder takes a buffer and an encoder hands one back, and the
caller says where the bytes came from or where they go.

Nothing here knows about Vulkan, a scene or an entity. A decoded picture is
pixels in an arena; what happens to them next is `render`'s.

- `include` — the public headers, in `include/assets/`. See
  `include/assets/assets.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
