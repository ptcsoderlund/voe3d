# 07 — A landscape's copies are cooked
folder: authoring
after: 01
decisions: 0168, 0442

## Change
0442 point 1, the cook's half: the copies the game compiles in, beside the heights and paint.

- `authoring/include/authoring/landscape_cook.h` —
  `voe_authoring_landscape_cook_copies(const voe_assets_landscape *landscape, const char *name,
  voe_base_arena *arena)` returning `voe_authoring_text`: `static const
  voe_assets_landscape_copy <name>[<n>] = {\n` then one `{x, z, turn, scale, kind, lean},` line
  a copy in the landscape's order, then `};\n`. Asserts copy_count above nought and `name` a C
  identifier. Header points: the copies cooked whole so the game stands each where the editor
  did; the kind paths are the table's, written by the game tree, not here.
- `authoring/src/landscape_cook.c` — the function; the heights' and paint's cooks unchanged.
- `authoring/tests/landscape_cook.c` — a grid with two copies, one of negative x and z, cooks
  to exactly the expected text.
- `authoring/authoring.md`, `authoring/src/src.md`, `authoring/tests/tests.md` — the landscape
  cook's entries mention the copies.

## Done when
Test `authoring/landscape_cook` passes.
