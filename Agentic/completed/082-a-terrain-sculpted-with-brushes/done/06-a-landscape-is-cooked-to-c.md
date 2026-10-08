# 06 — A landscape is cooked to C
folder: authoring
after: 01
decisions: 0168, 0379

## Change
A landscape's heights as C source a game tree compiles in, since a game reads no project text (0236,
0379 point 7).

- New `authoring/include/authoring/landscape_cook.h` / `authoring/src/landscape_cook.c` —
  `voe_authoring_text voe_authoring_landscape_cook(const voe_assets_landscape *landscape,
  const char *name, voe_base_arena *arena)`: `static const int32_t <name>[<(cells + 1)²>] = { … };`,
  whole millimetres rounded as the file writes them (assets/landscape.h), a line break every few numbers.
  Cannot fail; asserts `name` is a C identifier. Header points: what is cooked and why millimetres; the
  table naming the arrays is the game tree's (editor), so this knows no game type.
- New `authoring/tests/landscape_cook.c` — `cook_names_the_array_and_its_length`,
  `cook_writes_whole_millimetres`.
- `authoring/authoring.md` gains the `landscape_cook.h` entry; `src/src.md` and `tests/tests.md` theirs.

## Done when
`ctest --test-dir build/debug -R '^authoring/landscape_cook'` passes with the two tests above.
