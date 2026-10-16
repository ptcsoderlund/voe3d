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
- `png.c` — real files from a real encoder, one filter per row, the three failures card 017 named
  plus a corrupt CRC and the two variants refused rather than broken, then the writer round-tripped
  through the reader. Its header says why reading its own output is not the whole proof.
- `png_data.inc` — those files, as bytes.
- `json.c` — a document read back as its values, and every shape of broken
  input a tolerant reader would let past.
- `sectioned.c` — the principal's sketch read back as its text, one test
  per decision the format left open, and every shape of broken line.
- `landscape.c` — a flat and a 2048-cell grid round-tripped, heights rounded
  to the millimetre, a resample keeping a plane and its corners, layers and
  paint round-tripped byte for byte and resampled to sums of 255, and each
  refusal with its category.
- `material.c` — an empty section read as the defaults, a full file
  round-tripped field for field and written again byte for byte, and each bad
  value refused MALFORMED.
- `parts.c` — a parts file's entries and levels, shares and swaps included,
  round-tripped and written again byte for byte, defaults read for what is
  absent, and bad input refused MALFORMED.
- `model.c` — a hand-built `.glb` read back into its arrays, its fields read
  or defaulted, and a working file corrupted a byte at a time.
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
  extensible header and a skipped `LIST` decoded, and each refusal with its
  category.
