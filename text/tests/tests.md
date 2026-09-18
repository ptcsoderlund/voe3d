# tests

`text`'s own tests: plain C programs with an ordinary `main()`, zero for pass,
found by the build and registered nowhere. None of them needs a graphics card —
everything this folder computes is computed on the CPU, so every claim below is
checked by reading numbers and pixels straight out of an arena.

- `truetype.c` — that the reader gets the right numbers out of the font that is
  actually shipped, composite glyphs first, because a reader that handles only
  simple ones renders every accented character as a blank.
- `raster.c` — that the fill rule is non-zero winding and not even-odd, as the
  one pair of cases that tells them apart, and that the distance field measures a
  right angle correctly at its corner and along its median.
- `utf8.c` — that well-formed input decodes to the right character, that every
  malformed shape is refused, and that walking a mangled string always ends.
