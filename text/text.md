# text

A string into one mesh and one texture: the font read, the glyphs measured into
one distance-field sheet, the characters laid out. Or, for a caller placing
characters itself, where one character sits and how far the pen then moves. Not
what wears that mesh — a material and an entity are `3d`'s, and this folder names
only `render`. Not layout onto a surface either: that is `ui`'s.

- `include/text/font.h` — the whole public surface: name a `voe_text_typeface`
  and make its font, then either make a block from a string — once at startup
  or again every frame — or ask where one character sits and place it yourself.
  Its header says why one block is one draw, what the five things a text
  material has to say are, why the sheet is a distance field and not a picture,
  which of the two creates is a startup operation and which lives inside a
  frame and why neither caches a string, why an enum and not a path or bytes
  names a face and why both faces' licence notices travel with the files
  (ADR-0167), and what this folder refuses by name — kerning included, and why.
  On the metrics it says why there are two text paths and why a third would be
  wrong, that everything is in ems with +y up and who turns that round, why the
  box is a low-and-high pair while the sheet rectangle is a corner and a size,
  and where the two Y directions are reconciled, and what a whole string's
  measurement is for and why it carries a baseline beside a size.
- `include/text/utf8.h` — one character at a time. Its header says why a caller
  placing characters itself has to walk a string with this and not with a byte
  loop, why it never stands still, and which three encodings it refuses.
- `fonts/` — Oxanium Regular and Pixel Operator Regular, and the licence each
  travels under. Neither file is renamed and neither is modified.
- `src/truetype.h` — the file format, read. Its header says which seven tables
  and no more, why format 4 is the only character map, why a failure here is a
  bug in the reader rather than a bad font, why the composite walk is an
  explicit stack, and which of a composite component's transforms it reads —
  a uniform scale, for Pixel Operator's `¡` and `¿` — and which two it still
  refuses.
- `src/truetype.c` — the directory, the tables, the character map, the outlines
  and the walk over composite glyphs, offset and uniformly scaled alike. Its
  header says why every read goes through one bounds-checked cursor and what a
  zero out of it can mean.
- `src/raster.h` — an outline into a coverage bitmap, or into three signed
  distances per texel. Its header says why the fill rule is non-zero winding and
  what filling in the counters looks like, how the anti-aliasing is done and what
  it costs, why the field has three channels rather than one and where its sign
  comes from, and that the Y flip between font space and an image is here. It
  owns the spread and the corner threshold and says what each is worth.
- `src/raster.c` — the flattening, the scanline crossings and the spans, then the
  edge colouring and the distance loop. Its header says why the outline is walked
  twice and what the two halves share.
- `src/utf8.c` — the decoder.
- `src/font.c` — both embedded faces, the sheet built once per font, the glyph
  table both creates and the public metrics read, and the one layout both
  creates share. Its header is where the three scales and the three Y axes that
  meet in this folder are each pinned down, why the sheet is a distance field
  uploaded as data, why it is the one texture in the engine that asks for a
  filtered sampler and why that is not antialiasing, why each face's resolution
  is a fact of that face and not a function of the screen — and, for Pixel
  Operator, not of Oxanium's own number either — and why nothing in it is
  premultiplied.
- `tests/truetype.c` — the reader against both fonts that are actually shipped,
  composite glyphs first for each, Pixel Operator's scaled `¡` on top of that,
  and both fonts open at once. Its header says why real files and not built
  ones, why the accented characters are the test that matters, and why reading
  both at once is what a leaking static would fail at. Needs no graphics card.
- `tests/raster.c` — the fill rule, as the one pair of cases that tells non-zero
  winding from even-odd, and the field measured at one right angle. Its header
  says why an `o` on its own would pass with the wrong rule, and why the field's
  claims are made about a corner and about the median rather than about a letter
  or a channel. Needs no graphics card.
- `tests/utf8.c` — well-formed input, every malformed shape, and that walking a
  mangled string ends. Needs no graphics card.
