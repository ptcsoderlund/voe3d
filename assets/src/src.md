# src

`assets`'s implementation: the readers and the one writer behind the four
public headers, and the compressor and decompressor PNG shares with them.
Nothing here opens a file, so a decoder takes a buffer and an encoder hands one
back. A reader is here to pick which file to open: what each one owns, and which
of them is internal to this folder rather than reachable from outside it.

- `inflate.h` — DEFLATE inside a zlib wrapper, internal to this folder. Its
  header says why the caller states the output size instead of the decompressor
  discovering it, which is the whole of how a small hostile file is stopped from
  becoming a large allocation.
- `inflate.c` — the decompressor. Its header says which three things a
  compressed stream can attack and where each is bounded, and why an incomplete
  Huffman code is accepted where an over-subscribed one is not.
- `deflate.h` — the compressor, internal to this folder and the mirror of
  `inflate.h`. Its header says why it produces a zlib stream rather than a
  bare DEFLATE one, why it cannot fail, and where the worst-case output size the
  caller's arena is asked for comes from.
- `deflate.c` — one block, fixed Huffman codes, and a greedy match finder
  over a hashed chain. Its header carries the fixed code and the length and
  distance tables, and says which three rules keep a changed matcher legal.
- `png.c` — the reader: signature, chunks, CRC, filters, palette. Its header
  says why this file is flat and has no recursion in it, and the rule every
  length read out of the file is checked by.
- `png_crc.h` — PNG's CRC-32, shared by the reader and the writer and
  implemented in `png.c`. Its header says why there is no running value to
  chain from, and which bytes of a chunk it covers.
- `png_write.c` — the writer: colour type 6, filter 0 on every row, one
  `IDAT`. Its header says why each of those is the only shape it writes, and
  where the size of every buffer it pushes comes from.
- `sectioned.c` — two passes over one line lexer. Its header says why the
  caller states no capacity here where the JSON reader demands one, and why
  there is nothing for rule 14 to worry about.
- `json.h` — a JSON reader, internal to this folder. Its header says why it
  has an explicit stack and a depth limit rather than recursion, why the caller
  states how many values it will hold, and why string spans are raw.
- `json.c` — the state machine, and nothing tolerated.
- `model_gltf.h` — the split between the container and the description.
- `model_gltf.c` — accessors, buffer views, materials, images, meshes and the
  node tree. Its header says the rule every length read out of a file is checked
  by, and why the node graph is proven to be a tree here rather than in the
  importer.
- `model_glb.c` — the container: the header and the chunks. Its header
  carries the layout and says why a `.gltf` is refused by name.
- `jpeg.c` — markers, Huffman tables, the inverse cosine transform, chroma
  upsampling and the colour conversion. Baseline only; its header says why
  progressive is refused rather than attempted, and which index in the format is
  the one that walks off the end of a block.
- `wav.c` — the WAV reader: one walk over the RIFF chunks, then 16-bit or
  float samples converted to float in the caller's arena.
