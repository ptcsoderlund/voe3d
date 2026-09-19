# text

A string into one mesh and one texture: the font read, the glyphs measured into
one distance-field sheet, the characters laid out. Or, for a caller placing
characters itself, where one character sits and how far the pen then moves. Not
what wears that mesh — a material and an entity are `3d`'s, and this folder names
only `render`. Not layout onto a surface either: that is `ui`'s.

- `include` — the public headers, in `include/text/`; each is listed below by path.
- `src` — the implementation: the file read, the outlines rasterised, the string
  laid out; each file is listed on `src/src.md`.
- `tests` — one plain C program per module, found by the build, none of them
  needing a graphics card; each is listed on `tests/tests.md`.
- `include/text/font.h` — the whole public surface: name a `voe_text_typeface`
  and make its font, then either make a block from a string — once at startup
  or again every frame — or ask where one character sits and place it yourself.
  Its header says why one block is one draw, what the five things a text
  material has to say are, why the sheet is a distance field and not a picture,
  which of the two creates is a startup operation and which lives inside a
  frame and why neither caches a string, why an enum and not a path or bytes
  names a face and why Oxanium's licence notice travels with the file
  (ADR-0185), and what this folder refuses by name — kerning included, and why.
  On the metrics it says why there are two text paths and why a third would be
  wrong, that everything is in ems with +y up and who turns that round, why the
  box is a low-and-high pair while the sheet rectangle is a corner and a size,
  and where the two Y directions are reconciled, and what a whole string's
  measurement is for and why it carries a baseline beside a size.
- `include/text/utf8.h` — one character at a time. Its header says why a caller
  placing characters itself has to walk a string with this and not with a byte
  loop, why it never stands still, and which three encodings it refuses.
- `fonts/` — Oxanium Regular and `OFL.txt`, the SIL Open Font License it
  travels under. Neither file is renamed and neither is modified.
