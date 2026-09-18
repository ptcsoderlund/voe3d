# text

A string into one mesh and one texture: the font read, the glyphs measured into
one distance-field sheet, the characters laid out. Or, for a caller placing
characters itself, where one character sits and how far the pen then moves. Not
what wears that mesh — a material and an entity are `3d`'s, and this folder names
only `render`. Not layout onto a surface either: that is `ui`'s.

- `include` — the public headers, in `include/text/`. See `include/text/text.md`.
- `src` — the implementation. See `src/src.md`.
- `tests` — one plain C program per module, found by the build. See
  `tests/tests.md`.
- `fonts` — Oxanium Regular and the licence it travels under. Neither file is
  renamed and neither is modified.
