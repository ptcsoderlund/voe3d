# 78 — A kind's base size is an undo step and is cooked
folder: editor
after: 76, 77
decisions: 0168, 0379, 0442, 0459, 0460

## Change
0460 points 4–5, the editor's store call, the stroke that carries a size change, and the cook.
Nothing sets a size yet (card 79).

- `editor/src/models.h`, `editor/src/models.c` — `voe_editor_models_kind_size_set(models,
  path, uint32_t index, uint16_t size)` over `voe_3d_models_landscape_kind_size_set`, nothing
  for a path that is no loaded landscape. The header comment is at 58 of 60 lines: the
  declaration's own comment carries it, the header unchanged. Save already writes the sizes
  (the writer carries them).
- `editor/src/strokes.h`, `editor/src/strokes.c` — a stroke may be a kind's base size: a
  `kind_size` flag, `kind`, `size_before`, `size_after` (thousandths, 0 meaning 1×), no rect,
  heights or copies; `voe_editor_stroke_new_kind_size(path, uint32_t kind, uint16_t before,
  uint16_t after)`; `voe_editor_stroke_apply` writes `size_before` back and `size_after`
  forward through `voe_editor_models_kind_size_set`; `_destroy` frees nothing more. Header: the
  usage block gains the call; a paragraph says a size change rides as a stroke and why (0460
  point 5). The struct's comment names the new fields. Header under 60 lines.
- `editor/src/game_tree_landscapes.c` — for a landscape with copies, its sizes as `static const
  uint16_t landscape_<n>_kind_sizes[] = { ... };` (kind_count of them, the grid's `kind_sizes`
  as they are) beside its kinds, and the table entry gains `, landscape_<n>_kind_sizes` after
  the copy count; with no copies `, NULL`. The scratch's sizing counts the sizes' line. The
  header's example and its fields' line show the sizes.
- `editor/src/src.md` — the `strokes.*`, `models.*` and `game_tree_landscapes.c` entries name
  the base size; each entry ≤ 300 characters.

## Done when
The editor builds; `grep -n "kind_size_set" editor/src/strokes.c editor/src/models.c` prints a
line for each file; `grep -n "kind_sizes" editor/src/game_tree_landscapes.c` prints a line.

## Blocked
The cards this one comes after (75, 76, 77) are still in `todo/`: the tree at 2f9442fc has no
`voe_3d_models_landscape_kind_size_set` and no `kind_sizes` in `voe_assets_landscape`, so the
editor cannot call or cook them. Doing those three cards first, then running this one again, would unblock it.
