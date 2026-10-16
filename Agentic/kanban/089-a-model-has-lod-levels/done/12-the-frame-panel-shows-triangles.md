# 12 — The Frame panel shows the frame's triangles
folder: editor
after: 02, 08, 11
decisions: 0168, 0425

## Change
0425 point 7: under the passes and Total, a "Triangles" row with the frame's count from
`voe_render_frame_triangle_count` (`render/include/render/device.h`), refreshed with the times.

- `editor/src/frame_breakdown.h`: the panel gains `uint64_t triangles` and its label buffer
  (`char triangles_label[...]`, room for 20 digits and a NUL, named by a define beside
  `VOE_EDITOR_FRAME_MILLISECONDS`). The header's first paragraph names the row; its NODES and
  ELEMENTS counts gain the row and two labels, three nodes, and "Triangles" and at most 20 digits,
  29 elements, so 203 nodes and 3003 elements. The take's comment says the count is taken with
  the times and lags as they do.
- `editor/src/frame_breakdown.c`: `_take` reads the count when it copies a measurement; `_draw`
  adds the row after Total, as Total's row is drawn, the number as a plain integer.
- `editor/src/interface.h`: a paragraph after the last budget paragraph (the one ending at the
  `VOE_EDITOR_INTERFACE_NODES` define): the Triangles row adds three nodes and 29 elements, with
  the sums; `VOE_EDITOR_INTERFACE_NODES` 2553 and `VOE_EDITOR_INTERFACE_ELEMENTS` 51365.

## Done when
`voe_editor` builds in the folder's check with no warning. The human: place twenty of an
imported tree near the camera, open the Frame panel and read Triangles; fly far away and read it
again: the far count is a small fraction of the near one (How to test step 5).
