# sprite

The public headers, one entry each; the fuller account of both of them, and the
questions each answers, stays on `sprite/sprite.md`.

- `quad.h` — the one unit quad every sprite is drawn on, uploaded once and shared
  by every sprite entity in the world.
- `sheet.h` — a grid of equal cells, and the one material per frame that reads a
  cell of it.
