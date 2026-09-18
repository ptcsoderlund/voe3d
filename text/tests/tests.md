# tests

One plain C program per module, found by the build, checking the reader against
the font that is actually shipped and the raster's two rules. None of them needs
a graphics card.

- `truetype.c` — the reader against the font that is actually shipped, composite
  glyphs first. Its header says why a real file and not a built one, and why the
  accented characters are the test that matters.
- `raster.c` — the fill rule, as the one pair of cases that tells non-zero
  winding from even-odd, and the field measured at one right angle. Its header
  says why an `o` on its own would pass with the wrong rule, and why the field's
  claims are made about a corner and about the median rather than about a letter
  or a channel.
- `utf8.c` — well-formed input, every malformed shape, and that walking a mangled
  string ends.
