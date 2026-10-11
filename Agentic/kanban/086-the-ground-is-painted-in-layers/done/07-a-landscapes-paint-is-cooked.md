# 07 — A landscape's paint is cooked
folder: authoring
after: 01
decisions: 0168, 0379, 0402

## Change
0402 point 8, the cook's half: the paint the game compiles in, beside the heights array.

- `authoring/include/authoring/landscape_cook.h` — `voe_authoring_landscape_cook_paint(const
  voe_assets_landscape *landscape, const char *name, voe_base_arena *arena)` returning
  `voe_authoring_text`: `static const uint8_t <name>[<n>] = { ... };\n`, n being one plane's
  (cells + 1)² · 4 bytes when layer_count ≤ 4, both planes' when more, in the planes' order.
  Asserts `paint` not NULL, layer_count above nought and `name` a C identifier. Header points:
  what is cooked and why one plane suffices up to four layers, that a game table entry with no
  layers names no paint (the caller's), whole bytes so the game reads the saved weights exactly.
  The layer paths are the table's, written by the game tree; this writes no path.
- `authoring/src/landscape_cook.c` — the function, the heights' cook unchanged.
- `authoring/tests/landscape_cook.c` — a 4-cell grid with two layers and one painted point cooks
  to 100 bytes whose painted point's four bytes are as set; five layers cook 200.
- `authoring/authoring.md`, `authoring/src/src.md`, `authoring/tests/tests.md` — the
  landscape cook's entries mention the paint.

## Done when
Test `authoring/landscape_cook` passes.
