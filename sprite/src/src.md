# src

`sprite`'s implementation: one file per public header. Nothing here is included
from outside the folder — `include/sprite/` is the whole public surface.

Neither file holds any state. `quad.c` is the numbers one quad is made of,
uploaded once at startup; `sheet.c` is arithmetic over a grid, plus the loop that
uploads a material for every frame of it.

- `quad.c` — those vertices and indices, and the upload. Its header says where
  the winding was copied from and why the arrays are not public.
- `sheet.c` — the grid division and the loop that uploads it.
