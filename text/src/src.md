# src

`text`'s implementation: the font file read, the outlines rasterized into one
distance-field sheet, and the layout both creates share.

- `truetype.h` — the file format, read. Its header says which seven tables and no
  more, why format 4 is the only character map, why a failure here is a bug in
  the reader rather than a bad font, and why the composite walk is an explicit
  stack.
- `truetype.c` — the directory, the tables, the character map, the outlines and
  the walk over composite glyphs. Its header says why every read goes through one
  bounds-checked cursor and what a zero out of it can mean.
- `raster.h` — an outline into a coverage bitmap, or into three signed distances
  per texel. Its header says why the fill rule is non-zero winding and what
  filling in the counters looks like, how the anti-aliasing is done and what it
  costs, why the field has three channels rather than one and where its sign
  comes from, and that the Y flip between font space and an image is here. It
  owns the spread and the corner threshold and says what each is worth.
- `raster.c` — the flattening, the scanline crossings and the spans, then the
  edge colouring and the distance loop. Its header says why the outline is walked
  twice and what the two halves share.
- `utf8.c` — the decoder.
- `font.c` — the sheet built once, the glyph table both creates and the public
  metrics read, and the one layout both creates share. Its header is where the
  three scales and the three Y axes that meet in this folder are each pinned
  down, why the sheet is a distance field uploaded as data, why it is the one
  texture in the engine that asks for a filtered sampler and why that is not
  antialiasing, why its resolution is no longer a function of the screen, and why
  nothing in it is premultiplied.
