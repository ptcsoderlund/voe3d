# 56 — The levels question asks when they swap
folder: editor
after: 53, 54, 55
decisions: 0168, 0430, 0432

## Change
0432 point 1: the "Levels of <file>" question (`editor/src/levels_ask.h`) gains a third row,
"Swap", under Strength: Near, Normal, Far, Farther, the model's lit, read from and saved to its
`.parts` with the count and strength (`voe_assets_levels.swap`, `assets/include/assets/parts.h`).

- `editor/src/levels_ask.h`: `VOE_EDITOR_LEVELS_ASK_SWAPS` 4, in `voe_assets_levels_swap`'s order;
  the struct gains the swap buttons' nodes (and a row's, if the strength row has one). Header: the
  panel shows when the levels swap too (0432 point 1: ½, 1, 2, 4 times the distance), and a press
  on a swap only changes the shown values; `_draw`'s and `_read`'s comments name the swap.
- `editor/src/levels_ask.c`: the draw adds the row as the strength row is drawn (line ~150), the
  chosen swap lit; the read sets `ask->levels.swap` from a fired swap button as it sets the strength
  (line ~188) and answers NONE. Nothing else changes: open and save carry the whole struct.
- `editor/src/interface.h`: THE LEVELS QUESTION paragraph (line ~350) counts the row as it counts
  the others: the row and its label, two nodes; four buttons, each a button and a label, eight;
  ten nodes. "Swap", four elements; each button's border and fill, eight; "Near", "Normal", "Far",
  "Farther", twenty; thirty-two elements. The sums 2598 and 51744, and `VOE_EDITOR_INTERFACE_NODES`
  and `_ELEMENTS` raised to them (line ~388).
- `editor/src/src.md`: the `levels_ask.h` and `levels_ask.c` entries name the swap in a phrase,
  each still one sentence.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -n 'swap' editor/src/levels_ask.c` prints lines. The human, in the lumberhill project with
the terrain switched off:
1. Right-click `buildings/wooden_stable.glb` in Assets → Make levels: a Swap row shows Normal lit.
   Levels 4, Strong, Normal, Make.
2. Place the stable, select it and fly slowly away: as the Inspector's level climbs, each change
   dissolves over a short way and is not seen as a pop (feature How to test step 2).
3. Make levels with Swap Far: each level is reached at about twice the distance it was; with Near,
   at about half. Reopen the question: the last choice is lit.
4. The feature's How to test, steps 1 to 6, still hold.
