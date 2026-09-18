# sprite

A 2D sprite: the one quad every sprite is drawn on, and the materials that read
one cell each out of a sheet. Not billboarding, not animation, and not a
component of its own — a sprite is an ordinary entity wearing this quad and one
of these materials.

- `include` — the public headers, in `include/sprite/`; each is listed below by path.
- `src` — the implementation: the quad's numbers and the sheet's arithmetic; each
  file is listed on `src/src.md`.
- `tests` — one plain C program per module, found by the build; each is listed on
  `tests/tests.md`.
- `include/sprite/quad.h` — the unit quad, uploaded once and shared by every
  sprite. Its header says why nothing about a sprite may live in its mesh, why
  there are two faces in the same place, why the back one is mirrored, and where
  billboarding went.
- `include/sprite/sheet.h` — a grid of cells, and one material per frame. Its
  header says why a frame is a material and not a mesh, which way the cells are
  ordered, what a sheet's texture has to have been created as, and what this
  folder is deliberately not.
