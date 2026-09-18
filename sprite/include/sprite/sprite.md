# sprite

`sprite`'s public headers: the one quad every sprite is drawn on, and the sheet
that becomes one material per cell.

- `quad.h` — the unit quad, uploaded once and shared by every sprite. Its header
  says why nothing about a sprite may live in its mesh, why there are two faces
  in the same place, why the back one is mirrored, and where billboarding went.
- `sheet.h` — a grid of cells, and one material per frame. Its header says why a
  frame is a material and not a mesh, which way the cells are ordered, what a
  sheet's texture has to have been created as, and what this folder is
  deliberately not.
