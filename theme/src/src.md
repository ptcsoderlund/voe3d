# src

`theme`'s implementation. Nothing here is included from outside the folder —
`include/theme/` is the whole public surface.

- `theme.c` — the sectioned parse, then the schema over its one section: each key converted and range-checked, and the result written only once all of it passed.
