# 84 — The LOD rows have headers and a swap line
folder: editor
after: 83
decisions: 0168, 0435, 0437

## Change
0437 points 5 and 6 in "LODs of <file>". Read `voe_ui_sizing` and `VOE_UI_SIZE_FIXED` in
`ui/include/ui/layout.h` and `voe_ui_label_role` in `ui/include/ui/widgets.h`, their
comments only.

- `editor/src/levels_ask.c`, `_draw`:
  - When there is at least one LOD row, a header row before the first: an empty name
    cell, then "Triangles kept", "Swap distance", "Triangles", made or own alike.
  - Four column widths in millimetres, one named constant each, wide enough for the wider
    of the header and the longest value its buffer holds in `levels_ask.h`. The header
    row and every LOD row give their four cells those widths, so a header sits over its
    values. A label or number box that takes no size of its own sits in a fixed-width row.
  - After the last LOD row, one label in `VOE_UI_TEXT_ROLE_SECONDARY`: ×2 swaps the LOD
    in twice as far away and looks better, ×0.5 twice as close and saves triangles.
  - The top comment names the header row and the swap line.
- `editor/src/levels_ask.h`: the header comment's A LOD'S ROW paragraph says the
  columns have headers at fixed widths and the line under the rows explains the swap;
  `_draw`'s comment names both. A new field only if a text must outlive the call that
  drew it; string literals need none.
- `editor/src/interface.h`: THE LEVELS QUESTION paragraph (near line 350) counted again:
  the header row, its four cells and three labels; each LOD row's cell rows if any were
  added; the swap line's label. Elements: the three headers' and the line's characters.
  The sums that follow and `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` move by the
  difference.
- `editor/src/src.md`: the `levels_ask.c` entry names the headers and the swap line, under
  270 characters.

## Done when
`voe_editor` builds in the folder's check with no warning, and
`grep -c 'Triangles kept' editor/src/levels_ask.c` and
`grep -c 'Swap distance' editor/src/levels_ask.c` print at least 1.

The human's, in the editor on a project with the stable and a `.glb` with its own `_LOD1`
and `_LOD2`: every step of `## How to test` in
`Agentic/kanban/089-a-model-has-lod-levels/bugs/09-the-lod-rows-say-nothing-and-strength-is-left-over.planned.md`,
1 to 3, and `## How to test` 1 to 6 in `Agentic/kanban/089-a-model-has-lod-levels/feature.md`
still hold.
