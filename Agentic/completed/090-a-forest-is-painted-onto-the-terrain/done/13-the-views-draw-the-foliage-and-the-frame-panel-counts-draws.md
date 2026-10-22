# 13 — The views draw the foliage, and the Frame panel counts draws
folder: editor
after: 12
decisions: 0168, 0358, 0442

## Change
0442 point 5 in the editor's preview and views, the device's room for it, and a Draws row in
the Frame panel so "far fewer draws than trees" can be read (How to test 2).

- `editor/src/view_passes.c` — in the preview and in each shown view, after the frame's
  `models` is set and before the blockers, `voe_3d_draw_system_foliage(world, gpu, &frame,
  arena)`, its result ignored as the blockers' is, called before the arena mark is taken so
  the groups survive the rewind after the pass begins; the frame handed to
  `voe_3d_draw_system_run` (`drawn`, built anew) carries `foliage` and `foliage_count` from
  it. A comment says why it comes before the shadows and the mark.
- `editor/src/view_passes.h` — `VOE_EDITOR_CAPACITIES` gains `.instances =
  VOE_3D_FOLIAGE_INSTANCES`, and its objects VOE_3D_FOLIAGE_GROUPS per view and for the
  preview, in the pass and in each cascade of each of VOE_RENDER_DIRECTIONAL_LIGHTS lights.
  The header is at 58 of 60 lines: the reasoning goes in the comment above the macro.
- `editor/src/frame_breakdown.h`, `editor/src/frame_breakdown.c` — after Triangles, a "Draws"
  row: `voe_render_frame_draw_count`, taken with the triangles, its label sized for a
  uint32_t's 10 digits (`VOE_EDITOR_FRAME_DRAWS`). The header's first paragraph and its node
  and element counts grow by the row (3 nodes; "Draws" and 10 digits, 15 elements).
- `editor/src/interface.h` — the budget that counts the Frame panel's nodes and elements
  grows by the same.
- `editor/src/src.md` — entries for `view_passes.*` and `frame_breakdown.*` follow.

## Done when
The editor builds, and `grep -n "voe_3d_draw_system_foliage" editor/src/view_passes.c` and
`grep -n "frame_draw_count" editor/src/frame_breakdown.c` each print a line.
