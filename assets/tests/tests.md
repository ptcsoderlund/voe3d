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
- `model.c` — a hand-built `.glb` in and its arrays out, the two textures
  over one picture that the deduplication depends on, and one byte of a working
  file changed at a time. Its header says why the files are built rather than
  exported.
- `model_data.inc` — those files, as bytes.
- `jpeg.c` — greyscale, 4:4:4 and 4:2:0 round-tripped against the pixels
  that went in, plus the same three failures and the progressive refusal. Its
  header says where the files came from and why a quantiser of one is what keeps
  a decoder tested against its own encoder honest.
- `jpeg_data.inc` — those files, as bytes.
