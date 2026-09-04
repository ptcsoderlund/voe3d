# assets

Files on the way in: bytes from somewhere, engine data out. Every reader here is
written rather than fetched (ADR-0023), and nothing in this folder opens a file —
`platform` owns files and has no API for them yet, so a decoder takes a buffer
and the caller says where the bytes came from.

Nothing here knows about Vulkan, a scene or an entity. A decoded picture is
pixels in an arena; what happens to them next is `render`'s.

- `include/assets/model.h` — `.glb` files, read into vertex attributes,
  materials, decoded pictures and a node tree. Its header says why the binary
  form only, why no coordinate is ever converted while matrix layout is, which
  failures are malformed and which are unsupported, and why animation is ignored
  rather than refused.
- `include/assets/image.h` — pictures, decoded to RGBA8. Its header says why a
  decoder takes bytes rather than a path, why the output is always four channels
  whatever the file held, and — the one worth reading before touching a texture
  coordinate — why nothing in this engine turns an image the right way up,
  because PNG, Vulkan and glTF already agree that row zero is the top.
- `src/inflate.h` — DEFLATE inside a zlib wrapper, internal to this folder. Its
  header says why the caller states the output size instead of the decompressor
  discovering it, which is the whole of how a small hostile file is stopped from
  becoming a large allocation.
- `src/inflate.c` — the decompressor. Its header says which three things a
  compressed stream can attack and where each is bounded, and why an incomplete
  Huffman code is accepted where an over-subscribed one is not.
- `src/png.c` — signature, chunks, CRC, filters, palette. Its header says why
  this file is flat and has no recursion in it, and the rule every length read
  out of the file is checked by.
- `src/json.h` — a JSON reader, internal to this folder. Its header says why it
  has an explicit stack and a depth limit rather than recursion, why the caller
  states how many values it will hold, and why string spans are raw.
- `src/json.c` — the state machine, and nothing tolerated.
- `src/model_gltf.h` — the split between the container and the description.
- `src/model_gltf.c` — accessors, buffer views, materials, images, meshes and the
  node tree. Its header says the rule every length read out of a file is checked
  by, and why the node graph is proven to be a tree here rather than in the
  importer.
- `src/model_glb.c` — the container: the header and the chunks. Its header
  carries the layout and says why a `.gltf` is refused by name.
- `src/jpeg.c` — markers, Huffman tables, the inverse cosine transform, chroma
  upsampling and the colour conversion. Baseline only; its header says why
  progressive is refused rather than attempted, and which index in the format is
  the one that walks off the end of a block.
- `tests/inflate.c` — round trips against streams zlib produced, and three
  streams assembled by hand because no encoder emits them: a back-reference
  reaching behind the start of the output, an overlapping run, and every prefix
  of a good stream.
- `tests/png.c` — real files from a real encoder, one filter per row, against the
  three failures card 017 named — truncated, wrong magic, absurd declared size —
  plus a corrupt CRC and the two variants that are refused rather than broken.
- `tests/json.c` — a document read back as its values, and every shape of broken
  input a tolerant reader would let past.
- `tests/model.c` — a hand-built `.glb` in and its arrays out, the two textures
  over one picture that the deduplication depends on, and one byte of a working
  file changed at a time. Its header says why the files are built rather than
  exported.
- `tests/model_data.inc` — those files, as bytes.
- `tests/jpeg.c` — greyscale, 4:4:4 and 4:2:0 round-tripped against the pixels
  that went in, plus the same three failures and the progressive refusal. Its
  header says where the files came from and why a quantiser of one is what keeps
  a decoder tested against its own encoder honest.
