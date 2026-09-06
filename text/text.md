# text

A string into one mesh and one texture: the font read, the glyphs rasterised into
one atlas, the characters laid out. Not what wears that mesh — a material and an
entity are `3d`'s, and this folder names only `render`.

- `include/text/font.h` — the whole public surface: make the font, then make a
  block from a string. Its header says why one block is one draw, what the four
  things a text material has to say are, why a block is built once and never
  changes, why the licence notice travels with the file, and what this folder
  refuses by name — kerning included, and why.
- `fonts/` — Oxanium Regular and the licence it travels under. Neither file is
  renamed and neither is modified.
- `src/truetype.h` — the file format, read. Its header says which seven tables
  and no more, why format 4 is the only character map, why a failure here is a
  bug in the reader rather than a bad font, and why the composite walk is an
  explicit stack.
- `src/truetype.c` — the directory, the tables, the character map, the outlines
  and the walk over composite glyphs. Its header says why every read goes through
  one bounds-checked cursor and what a zero out of it can mean.
- `src/raster.h` — an outline into a coverage bitmap. Its header says why the
  fill rule is non-zero winding and what filling in the counters looks like, how
  the anti-aliasing is done and what it costs, and that the Y flip between font
  space and an image is here.
- `src/raster.c` — the flattening, the scanline crossings and the spans. Its
  header says why the outline is walked twice.
- `src/utf8.h` — one character at a time. Its header says why it never stands
  still and which three encodings it refuses.
- `src/utf8.c` — the decoder.
- `src/font.c` — the atlas built once, the glyph table and the layout. Its header
  is where the three scales and the three Y axes that meet in this folder are
  each pinned down, and why the atlas is white with the coverage in its alpha and
  is not premultiplied.
- `tests/truetype.c` — the reader against the font that is actually shipped,
  composite glyphs first. Its header says why a real file and not a built one,
  and why the accented characters are the test that matters. Needs no graphics
  card.
- `tests/raster.c` — the fill rule, as the one pair of cases that tells non-zero
  winding from even-odd. Its header says why an `o` on its own would pass with
  the wrong rule. Needs no graphics card.
- `tests/utf8.c` — well-formed input, every malformed shape, and that walking a
  mangled string ends. Needs no graphics card.
