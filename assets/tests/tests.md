# tests

One plain C program per `assets` module, found by the build, each checking that
module's promises from outside. Nothing here opens a file: the bytes a test
reads are compiled in beside it as a `_data.inc`, listed under the test it
belongs to.

- `inflate.c` — round trips against streams zlib produced, and three
  streams assembled by hand because no encoder emits them: a back-reference
  reaching behind the start of the output, an overlapping run, and every prefix
  of a good stream.
- `inflate_data.inc` — those streams, as bytes.
- `deflate.c` — every case round-tripped through `../src/inflate.c`, from one
  byte to 200 kB, at the longest match and the farthest distance the format has.
  Its header says why a round trip alone would pass with the matcher switched off
  and what the size checks do about it.
- `png.c` — real encoder files with every filter, 16-bit and 4-bit files built
  here, each refusal read for its code and its kept line's words, and the
  writer round-tripped through the reader.
- `png_data.inc` — those files, as bytes.
- `image_fit.c` — the sizes a largest side gives, a constant picture kept
  constant for each kind, colour averaged in linear light and by alpha, two
  leaning normals averaged to a unit one, and data averaged as stored.
- `image_levels.c` — chain lengths as Vulkan counts them, a constant's mips
  kept constant for each kind, a checker halved to linear grey, and the cooked
  form round-tripped as RGBA8, BC7 and BC5, refused when broken, short, of an
  unknown format or the old version, and its key read alone.
- `image_bc7.c` — a hand-built block decoded to its texels, and one colour,
  two colours, a gradient and a cut-out's alpha round-tripped within bounds,
  the same texels always the same bytes.
- `image_blocks.c` — chains' sizes and block counts, a colour chain as BC7
  and a bump map as BC5 decoded near their texels, runs of 7 blocks matching
  one call, and a 5×3 picture's edge blocks.
- `json.c` — a document read back as its values, every shape of broken input
  a tolerant reader would let past, and too many values or too deep refused as
  limits.
- `sectioned.c` — the principal's sketch read back as its text, one test
  per decision the format left open, and every shape of broken line.
- `landscape.c` — grids, heights, layers, paint, foliage and kinds' base sizes
  round-tripped and resampled, and each refusal with its category.
- `landscape_blend.c` — a kind's `blend<k>` and `blendheight<k>` read, written
  only off the default and read back, and each out-of-range value refused.
- `material.c` — an empty section read as the defaults, a full file
  round-tripped field for field and written again byte for byte, and each bad
  value refused MALFORMED.
- `parts.c` — a parts file's entries and levels, shares and swaps included,
  round-tripped and written again byte for byte, defaults read for what is
  absent, and bad input refused MALFORMED.
- `model.c` — a hand-built `.glb` read back into its arrays, its fields read
  or defaulted, a working file corrupted a byte at a time, each refusal's
  kept line read for its subject, and the encoded read matching the decoded
  one.
- `model_material.c` — built glTF materials, no file, to their `.material`
  values; and the ORM picture from metal-roughness, occlusion, both or
  neither, its green and blue the products by the roughness and metal factors.
- `model_data.inc` — those files, as bytes.
- `jpeg.c` — greyscale, 4:4:4 and 4:2:0 round-tripped against the pixels
  that went in, and each refusal read for its code and its kept line's words.
  Its header says where the files came from.
- `jpeg_data.inc` — those files, as bytes.
- `wav.c` — WAV files built byte by byte: mono 16-bit, stereo float, an
  extensible header and a skipped `LIST` decoded, each refusal with its
  category, and a 24-bit file's kept line naming 24-bit and 16-bit.
