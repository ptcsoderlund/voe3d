# assets

`assets`'s public headers: the three formats a caller reads or writes — models,
pictures and the engine's own authored text. A caller hands over the bytes and
the arena; nothing here opens a file.

- `model.h` — `.glb` files, read into vertex attributes, materials, decoded
  pictures and a node tree. Its header says why the binary form only, why no
  coordinate is ever converted while matrix layout is, which failures are
  malformed and which are unsupported, and why animation is ignored rather than
  refused.
- `image.h` — pictures, both directions: PNG and JPEG decoded to RGBA8, and
  RGBA8 encoded back as a PNG. Its header says why neither direction takes a
  path now that `platform` has one, why the output is always four channels
  whatever the file held, what the encoder writes and what it refuses, and — the
  one worth reading before touching a texture coordinate — why nothing in this
  engine turns an image the right way up, because PNG, Vulkan and glTF already
  agree that row zero is the top.
- `sectioned.h` — the engine's own authored text format: `[Section]` headers and
  `key=value` lines, handed back as text and never interpreted. Its header says
  why it is layer one of three and stops there, why comments are `//` and not
  `#`, and every question the format left open with the answer it took.
