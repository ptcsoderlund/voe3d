# 01 — A program can scroll a node into the middle of a scroll area
folder: ui
after: none
decisions: 0168, 0366

## Change
Lift "no scrolling by a program" from `ui` with one call (0366 point 1).

- `ui/include/ui/widgets.h`: add, in the scroll area section after `voe_ui_scroll_begin`,
  `void voe_ui_scroll_centre(voe_ui_context *ui, voe_ui_node area, voe_ui_node node,
  voe_ui_scroll_axes axes);`. Its comment says: called in the window after `voe_ui_frame_end`
  where rectangles are readable, `area` a node `voe_ui_scroll_begin` handed back this frame and
  `node` one inside it; on each axis in `axes` that the area scrolls and where `node`'s
  rectangle is not wholly within the area's visible rectangle, the remembered offset is set so
  the node's centre meets the visible centre; already wholly seen moves nothing; it lands in the
  next frame's layout, clamped there, like the pointer's scroll; an area not called next frame
  forgets it as any offset. Rewrite the paragraph "No scrolling by a program and no scrolling to
  a node yet" so it says what is still missing (scrolling by a length, focus reaching an area).
  Refused as `voe_ui_node_rect` is for NONE or a node not made this frame (assert).
- `ui/src/scroll.c`: the definition. Find the area's slot in this frame's areas by its node,
  read the node's rectangle and the area's visible one, compute the new offset per axis as
  current offset plus (node centre − visible centre), clamp it with the existing
  `clamp_offset`/`scroll_range`, and write it into `scroll_memory` as `scroll_by` does. Not
  through `scroll_by`: nothing passes outward. Update the file's header where it lists the ways
  an offset moves.
- `ui/src/src.md`: the `scroll.c` entry names the centring as one of the moves.
- `ui/ui.md`: the `include/ui/widgets.h` entry mentions scrolling a node into the middle.
- `ui/tests/scroll.c`: add cases, each its own function called from `main`:
  - a Y-scrolling area 50 mm tall over twenty 10 mm boxes, no gap or pad; after the first frame, centring box 15
    gives next frame's `voe_ui_node_scroll` the hand-worked value (box centre 155 − 25 = 130);
  - centring box 19 clamps to the end (measured less arranged);
  - centring box 1, wholly seen at offset 0, leaves the offset 0;
  - an axis not in `axes` is left as it was.
- `ui/tests/tests.md`: the `scroll.c` entry names the centring.

## Done when
`ctest --test-dir build/debug -R "^ui/scroll$"` exits 0 with the new cases in `ui/tests/scroll.c`.
