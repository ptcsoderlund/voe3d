# src

`text`'s implementation: the file format read, the outlines turned into pixels, and the string
measured and laid out. Nothing here is included from outside the folder — `include/text/` is the
whole public surface.

- `truetype.h` — the file format's own surface: which seven tables are read, why
  format 4 is the only character map, why the composite walk is an explicit
  stack, and which composite transforms it reads — a uniform scale — and which
  it still refuses.
- `truetype.c` — the directory, the tables, the character map, the outlines and
  the walk over composite glyphs, offset and uniformly scaled alike, every read
  through one bounds-checked cursor.
- `raster.h` — an outline into a coverage bitmap or into three signed distances
  per texel: the non-zero fill rule, the anti-aliasing, the spread and the corner
  threshold, and the Y flip between font space and an image.
- `raster.c` — the flattening, the scanline crossings and the spans, then the
  edge colouring and the distance loop, on one flattener walked twice.
- `utf8.c` — the decoder: one character forward, with every malformed shape
  turned into one replacement character.
- `font.c` — the one embedded face, Oxanium: its sheet of Latin-1 plus a short list of symbols,
  the glyph table, the public metrics and the one layout they share.
