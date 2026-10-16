# 75 — Each LOD has a row with its share and swap
folder: editor
after: 73, 74
decisions: 0168, 0435, 0436

## Change
0436 point 8: "LODs of <file>" gains a row per LOD from LOD 2 on, with its share, its swap
and the triangles it really has.

- `editor/src/levels_ask.h`:
  - `voe_editor_levels_ask` gains: `uint32_t triangles[VOE_ASSETS_LEVELS]`, `known` (how
    many levels it holds), `bool own`, `bool fresh`; one `voe_ui_node` per row for the share
    box and the swap box; and per row text buffers for its name ("LOD 2"), share ("50 %"),
    swap ("×1") and triangles (the count, or "—"), sized for the longest each can be.
  - Header comment: the rows replace the Swap row's place; what a share and a swap are
    (0435), tenths, the share kept below the one before and later ones pushed down, swaps
    0.1 to 10; triangles are the file's, "—" when not known; own levels hide the LODs and
    Strength rows and show their share as a label. `_draw`'s and `_read`'s comments name
    the rows.
- `editor/src/levels_ask.c`:
  - `_open`: after the `.parts` read, `voe_game_models_levels_triangles`
    (`game/include/game/models.h`) fills `triangles`, `known`, `own`, `fresh`; `known` 0
    when it fails. Scratch rewound as now.
  - `_draw`: own hides the LODs and Strength rows. Rows: made, k from 1 to `levels.count`
    − 1; own, k from 1 to `known` − 1. Each: the name label; the share as a number box
    (`voe_ui_number_begin(ui, "share", k, shares[k], 1.0)` with its figure, then
    `voe_ui_end`), or for own a label of triangles[k] ÷ triangles[0] × 100 to a tenth; the
    swap as a number box (`"swap"`, k, swaps[k], 0.1); the triangles label, triangles[k]
    when k < `known`, else "—". Format with `%g` so 50 shows "50 %" and 12.5 "12.5 %".
  - `_read`: `voe_ui_number_action(ui, node).changed` on a share box sets `shares[k]` to
    its value rounded to a tenth, clamped to at least 0.1 × (VOE_ASSETS_LEVELS − k) and at
    most `shares[k − 1]` − 0.1; then each later share at or above the one before it is set
    to that one − 0.1. A changed swap box sets `swaps[k]` rounded to a tenth, clamped to
    `VOE_ASSETS_LEVELS_SWAP_LEAST`..`_MOST`. Both stay NONE.
  - The top comment names the rows, not "the chosen ... swap lit".
- `editor/src/interface.h`: THE LEVELS QUESTION paragraph (near line 350) counted again:
  the Swap row's label and its four buttons gone; at most five rows, each the row, its
  name label, two number boxes each with its figure, and its triangles label; elements at
  each text buffer's size, each box's one fill (ui/widgets.h: no border); and, while one
  box is open for typing, a caret, VOE_UI_FIELD_CAPACITY characters and "not a number",
  one at a time, as the Project panel's paragraph counts them. The sums that follow and
  `VOE_EDITOR_INTERFACE_NODES` and `_ELEMENTS` move by the difference.
- `editor/src/src.md`: the `levels_ask.h` and `levels_ask.c` entries name the rows.

## Done when
`voe_editor` builds in the folder's check with no warning, and both
`grep -c 'voe_game_models_levels_triangles' editor/src/levels_ask.c` and
`grep -c 'VOE_ASSETS_LEVELS_SWAP_LEAST' editor/src/levels_ask.c` print at least 1.
