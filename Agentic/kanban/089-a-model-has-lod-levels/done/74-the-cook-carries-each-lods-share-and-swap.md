# 74 — The cook carries each LOD's share and swap
folder: editor
after: 70, 71
decisions: 0168, 0435, 0436

## Change
Card 70 replaced `voe_assets_levels.swap` with `shares[]` and `swaps[]` and removed the
`voe_assets_levels_swap` enum. This card brings `editor` back to building; the per-LOD rows
come in card 75.

- `editor/src/game_tree_materials.c`:
  - `swap_name` goes. `parts_entry` writes `.levels = { .count, .strength, .shares = { … },
    .swaps = { … } }`, all VOE_ASSETS_LEVELS of each, every float `%a` with an `f` as the
    material floats are, so the game reads exactly what the `.parts` read gave.
  - The header comment's example entry shows `.shares` and `.swaps` in place of `.swap`.
- `editor/src/levels_ask.h` and `editor/src/levels_ask.c`: the Swap row goes —
  `VOE_EDITOR_LEVELS_ASK_SWAPS`, `swap_buttons`, `SWAP_NAMES`, and its draw, read and open
  lines. `_save` still copies the whole `ask->levels`, so the shares and swaps a `.parts`
  held survive a save. Both header comments stop naming Near, Normal, Far, Farther and the
  swap row; `levels_ask.h` says the per-LOD rows follow (0435 point 4).
- `editor/src/interface.h` is left for card 75, which counts the question again.
- `editor/src/src.md`: entries that name the
  swap row or swap words change.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -c 'SWAP_\|levels.swap\b\|swap_name' editor/src/game_tree_materials.c editor/src/levels_ask.c editor/src/levels_ask.h`
prints 0 for each.
