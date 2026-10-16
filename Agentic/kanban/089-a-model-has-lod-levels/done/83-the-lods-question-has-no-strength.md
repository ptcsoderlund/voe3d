# 83 — The LODs question has no Strength
folder: editor
after: 80, 81, 82
decisions: 0168, 0435, 0437

## Change
0437 point 1. Card 81 removed `voe_assets_levels.strength` and its enum; this card brings
`editor` back to building and takes the Strength row out of "LODs of <file>". The column
headers and the swap line come in card 84.

- `editor/src/game_tree_materials.c`: `strength_name` goes; `parts_entry` writes no
  `.strength`. The header comment's example entry loses it.
- `editor/src/levels_ask.h`:
  - `VOE_EDITOR_LEVELS_ASK_STRENGTHS` and `strength_buttons` go.
  - Header comment: no Strength; own levels hide only the LODs row. `_draw`'s, `_read`'s
    and `voe_editor_levels_ask_swaps_only`'s comments drop strength.
- `editor/src/levels_ask.c`:
  - `STRENGTH_NAMES` goes, with the Strength row's draw (its label and three buttons),
    its read loop, and any line in `_open` or `_close` that walks the strength buttons.
  - `voe_editor_levels_ask_swaps_only` compares the count and the shares only.
  - The top comment drops strength.
- `editor/src/interface.h`: THE LEVELS QUESTION paragraph (near line 350) counted again:
  the Strength row and its label gone, three fewer buttons (Gentle, Medium, Strong, each a
  button and a label; each button's border and fill; their texts), and "Strength" gone
  from the labels. The sums that follow and `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS`
  move by the difference.
- `editor/src/src.md`: the `levels_ask.h` entry, as card 80 left it, and the
  `levels_ask.c` and `game_tree_materials.c` entries, wherever they name strength or
  Gentle, Medium or Strong; each stays under 270 characters.

## Done when
`voe_editor` builds in the folder's check with no warning,
`grep -ci strength editor/src/levels_ask.c editor/src/levels_ask.h` prints 0 for each,
and `grep -cE 'strength_name|levels\.strength|LEVELS_(GENTLE|MEDIUM|STRONG)' editor/src/game_tree_materials.c`
prints 0.
