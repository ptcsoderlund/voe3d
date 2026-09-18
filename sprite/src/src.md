# src

`sprite`'s implementation: one file per header above it — the quad's vertices
and the grid division.

- `quad.c` — those vertices and indices, and the upload. Its header says where
  the winding was copied from and why the arrays are not public.
- `sheet.c` — the grid division and the loop that uploads it.
