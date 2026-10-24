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
  says why this file is flat and has no recursion in it, the rule every
  length read out of the file is checked by, and that each refusal is one line
  in the artist's words.
- `png_crc.h` — PNG's CRC-32, shared by the reader and the writer and
  implemented in `png.c`. Its header says why there is no running value to
  chain from, and which bytes of a chunk it covers.
- `png_write.c` — the writer: colour type 6, filter 0 on every row, one
  `IDAT`. Its header says why each of those is the only shape it writes, and
  where the size of every buffer it pushes comes from.
- `image_fit.c` — a picture shrunk to a largest side by an area average, one
  output row of float sums at a time, colour in linear light weighted by alpha
  and normals made unit length again.
- `image_average.h` — the area-average shrink to an exact size, internal to
  this folder, shared by the fit and the mips so the two cannot disagree.
- `image_levels.c` — a picture's level count and size, its mip chain made
  level by level, and the cooked form: head and levels written, the key read
  from the head alone, and a read with no copy that bounds every head field.
- `sectioned.c` — two passes over one line lexer. Its header says why the
  caller states no capacity here where the JSON reader demands one, and why
  there is nothing for rule 14 to worry about.
- `landscape.c` — the landscape file over the sectioned reader: shape, quoted
  layer paths and heights, and its measure-then-print writer, handing paint,
  foliage and the layers' grows to the two files below.
- `landscape_paint.h` — the paint's read, print and resample, internal to this
  folder, with the printer and source index `landscape.c` shares with it.
- `landscape_paint.c` — `[Paint]`'s rows and `v*n` runs into two RGBA8 planes,
  back out, and resampled per weight.
- `landscape_foliage.h` — the foliage's and the layers' grows' read and print,
  internal to this folder, with the quote helpers `landscape.c` shares with it.
- `landscape_foliage.c` — `[Foliage]`'s quoted kind paths and looks, `[Layers]`'
  `grow<l>_<k>` densities and `[Copies]`' six numbers a copy, walked once in
  order, and back out.
- `material.c` — the material file over the sectioned reader, with a scratch
  arena of its own, and a writer that quotes every path.
- `parts.c` — the parts file over the sectioned reader: each `material<N>` key
  to an index and its path, the optional `[Levels]` section's count,
  shares and swaps, and a writer that quotes every path.
- `json.h` — a JSON reader, internal to this folder. Its header says why it
  has an explicit stack and a depth limit rather than recursion, why the caller
  states how many values it will hold, why string spans are raw, and the
  damaged-file line and the two limits a refusal says.
- `json.c` — the state machine, and nothing tolerated: a syntax error is
  damage with its byte, too many values or too deep a limit with the most the
  engine takes.
- `model_gltf.h` — the split between the container and the description.
- `model_gltf_read.h` — what the glTF reader's parts share, internal to this
  folder: the read's state, the shared helpers and each part's one call.
- `model_gltf.c` — the shared helpers, accessors and buffer views, the version
  and extension checks, and the entry point. Its header says the rule every
  length read out of a file is checked by, and the one buffer.
- `model_gltf_materials.c` — texture references, the alpha mode, materials,
  images recorded encoded, and a picture decoded on asking. Its header says why
  a texture resolves to an image.
- `model_gltf_meshes.c` — attributes, primitives and meshes, each mesh under
  its part and each primitive under its material, triangle lists only.
- `model_gltf_nodes.c` — the node names and matrices and the node tree, each
  node under its object. Its header says how the tree is proven a tree without
  recursion; every refusal is damage.
- `model_material.c` — a glTF material to `.material` values, and its ORM
  picture merged from occlusion and metal-roughness at the nearest texel and
  multiplied by the roughness and metal factors.
- `model_glb.c` — the container: the header and the chunks, read with the
  pictures left encoded or each decoded. Its header
  carries the layout and says why a `.gltf` is refused by name, a container not
  version 2 refused, and a cut or misshapen chunk refused as damage.
- `jpeg_read.h` — what the JPEG reader's two parts share, internal to this
  folder: the decoder's state, the refusal helper and the scan's two calls. Its
  header says why the split is by stage, and that each refusal is one line in
  the artist's words.
- `jpeg.c` — the markers and their tables, chroma upsampling, the colour
  conversion and the entry point. Baseline only; its header says why
  progressive and every other frame kind is refused rather than attempted.
- `jpeg_scan.c` — the entropy-coded data: the bit reader, Huffman decode, one
  block, the inverse cosine transform and every MCU in order. Its header says
  which index walks off the end of a block and where each is bounded; damage
  met in a block is refused there.
- `wav.c` — the WAV reader: one walk over the RIFF chunks, then 16-bit or
  float samples converted to float in the caller's arena; a refusal names the
  format or the damage in one line in the artist's words.
