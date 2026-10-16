# 55 — The cook carries a model's swap
folder: editor
after: 47, 48
decisions: 0168, 0430, 0432

## Change
0432 point 1: `voe_assets_levels` gains `swap` (`assets/include/assets/parts.h`, card 48). The
cooked parts table names every level field, so a cooked game's table says the model's swap rather
than a zeroed one.

- `editor/src/game_tree_materials.c`: the parts entry's format (line ~165) gains
  `.swap = %s` after `.strength`, the enum's name (`VOE_ASSETS_LEVELS_SWAP_NEAR`, `_NORMAL`, `_FAR`,
  `_FARTHER`) written by a small static helper over the four, shaped as `strength_name` (line ~136),
  asserting a swap past the four. The header's example (line ~24) shows it.
- `editor/src/src.md`: the `game_tree_materials.c` entry, if it names count and strength, names
  the swap too, still one sentence.

`editor/src/model_import.c` and `editor/src/levels_ask.c` copy the whole struct and need nothing;
the question's swap row is card 56's.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -n 'VOE_ASSETS_LEVELS_SWAP' editor/src/game_tree_materials.c` prints lines.
