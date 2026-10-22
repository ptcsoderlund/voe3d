# 08 — The Frame panel shows each picture's counts
folder: editor
after: 07
decisions: 0168, 0358, 0465

## Change
0465 point 1, the showing: under Draws, two rows per picture drawn — its name with drawn and
skipped, then its shadows' drawn and skipped — taken with the quarter-second refresh.

- `editor/src/frame_breakdown.h`, `editor/src/frame_breakdown.c` —
  - `voe_editor_frame_breakdown` gains the pictures' counts as taken (`voe_editor_picture_counts`,
    `editor/src/view.h`, up to VOE_EDITOR_VIEWS + 1) and their labels, sized for a name and two
    uint32_t's.
  - `voe_editor_frame_breakdown_take` gains `const voe_editor_views *views`: at each refresh it
    copies `views->pictures` and `picture_count` with the triangles and draws; a showing never
    measured shows them too (they need no timestamps).
  - `_draw`: per picture a row `Camera` (the preview) or `View N` (index + 1) with
    `<drawn> drawn, <skipped> skipped`, then a row `  shadows` with `cast` and `cast_skipped`
    the same way. The header's first paragraph, its constraints and its node and element counts
    grow by those rows at their most.
- `editor/src/interface.h` — the budget that counts the Frame panel's nodes and elements grows by
  the same, in a paragraph beside the Draws row's.
- `editor/src/main.c` — the `voe_editor_frame_breakdown_take` call hands `&views`.
- `editor/src/src.md` — the `frame_breakdown.*` entries follow.

## Done when
The editor builds, and `grep -n "skipped" editor/src/frame_breakdown.c` prints a line.

Human (How to test 1–5): open the hill with the Frame panel; note View 1's drawn and skipped and
the view pass's time. Turn to the sky, then an empty corner: drawn falls, skipped rises, the pass
time drops. Fly and turn fast past things at the edge: nothing pops or flickers. Sun low, a tall
thing just behind or beside the picture: its shadow lies on the ground in view. Open the second
view and select the scene camera: View 2 and Camera each show their own counts.
