# src

`text`'s implementation: the file format read, the outlines turned into pixels,
and the string measured and laid out. Nothing here is included from outside the
folder — `include/text/` is the whole public surface.

The seam runs by stage. `truetype.*` knows the byte layout of a `.ttf` and
nothing about pixels; `raster.*` knows about pixels and nothing about files; and
`font.c` is the only file where the two meet and where this folder's three scales
and three Y axes are reconciled.

- `truetype.h` — the file format's own surface: which seven tables are read, why
  format 4 is the only character map, and why the composite walk is an explicit
  stack.
- `truetype.c` — the directory, the tables, the character map, the outlines and
  the walk over composite glyphs, every read through one bounds-checked cursor.
- `raster.h` — an outline into a coverage bitmap or into three signed distances
  per texel: the non-zero fill rule, the anti-aliasing, the spread and the corner
  threshold, and the Y flip between font space and an image.
- `raster.c` — the flattening, the scanline crossings and the spans, then the
  edge colouring and the distance loop, on one flattener walked twice.
- `utf8.c` — the decoder: one character forward, with every malformed shape
  turned into one replacement character.
- `font.c` — the sheet built once, the glyph table both creates and the public
  metrics read, and the one layout they share; the file where the three scales
  and the three Y axes are pinned down.
