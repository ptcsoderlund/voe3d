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
- `png.c` — real files from a real encoder, one filter per row, 16-bit RGBA and grey built here
  and rounded to 8 bits, the three failures card 017 named plus a corrupt CRC, interlace and depth 4
  refused rather than broken, every refusal's kept line read for `(PNG: ` and the words for interlace,
  4-bit and the huge size's 16384, then the writer round-tripped through the reader.
- `png_data.inc` — those files, as bytes.
- `json.c` — a document read back as its values, and every shape of broken
  input a tolerant reader would let past.
- `sectioned.c` — the principal's sketch read back as its text, one test
  per decision the format left open, and every shape of broken line.
- `landscape.c` — grids, heights, layers, paint and foliage round-tripped and
  resampled, and each refusal with its category.
- `material.c` — an empty section read as the defaults, a full file
  round-tripped field for field and written again byte for byte, and each bad
  value refused MALFORMED.
- `parts.c` — a parts file's entries and levels, shares and swaps included,
  round-tripped and written again byte for byte, defaults read for what is
  absent, and bad input refused MALFORMED.
- `model.c` — a hand-built `.glb` read back into its arrays, its fields read
  or defaulted, a base colour texture at `texCoord` −1 read as none, and a
  working file corrupted a byte at a time.
- `model_material.c` — built glTF materials, no file, to their `.material`
  values: opaque, MASK, BLEND, unlit, double-sided, emission and repeat; and
  the ORM picture from metal-roughness alone, with a larger occlusion picture,
  from occlusion alone, and from neither.
- `model_data.inc` — those files, as bytes.
- `jpeg.c` — greyscale, 4:4:4 and 4:2:0 round-tripped against the pixels
  that went in, plus the same three failures and the progressive refusal. Its
  header says where the files came from and why a quantiser of one is what keeps
  a decoder tested against its own encoder honest.
- `jpeg_data.inc` — those files, as bytes.
- `wav.c` — WAV files built byte by byte: mono 16-bit, stereo float, an
  extensible header and a skipped `LIST` decoded, each refusal with its
  category, and a 24-bit file's kept line naming 24-bit and 16-bit.
