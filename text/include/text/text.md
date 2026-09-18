# text

`text`'s public headers: the font and the two ways to use it — a whole string as
one block, or one character at a time — and the walk over a UTF-8 string a caller
placing characters itself needs.

- `font.h` — the whole public surface: make the font, then either make a block
  from a string — once at startup or again every frame — or ask where one
  character sits and place it yourself. Its header says why one block is one
  draw, what the five things a text material has to say are, why the sheet is a
  distance field and not a picture, which of the two creates is a startup
  operation and which lives inside a frame and why neither caches a string, why
  the licence notice travels with the file, and what this folder refuses by name
  — kerning included, and why. On the metrics it says why there are two text
  paths and why a third would be wrong, that everything is in ems with +y up and
  who turns that round, why the box is a low-and-high pair while the sheet
  rectangle is a corner and a size, and where the two Y directions are
  reconciled, and what a whole string's measurement is for and why it carries a
  baseline beside a size.
- `utf8.h` — one character at a time. Its header says why a caller placing
  characters itself has to walk a string with this and not with a byte loop, why
  it never stands still, and which three encodings it refuses.
