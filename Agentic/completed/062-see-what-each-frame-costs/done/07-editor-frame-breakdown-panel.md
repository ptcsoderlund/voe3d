# 07 — The editor's frame breakdown panel
folder: editor
after: 03
decisions: 0168, 0358, 0367

## Change
New `editor/src/frame_breakdown.h` and `editor/src/frame_breakdown.c`, shaped like
`editor/src/errors.h` (read it for the pattern: struct, show, hide, draw, clicks read).

- `voe_editor_frame_breakdown`: zeroed is hidden; `showing`; the shown passes as
  `voe_render_pass_time` (`render/include/render/device.h`, timing section), at most
  `VOE_EDITOR_FRAME_PASSES` (64); the shown total; the lines kept for the labels (a label's text is
  drawn after the call); the seconds since the last refresh; the × node.
- `void voe_editor_frame_breakdown_take(voe_editor_frame_breakdown *breakdown,
  const voe_render_device *gpu, float seconds)`: while showing, at most four times a second, copies
  `voe_render_frame_pass_times` and `voe_render_frame_gpu_time` into the shown lines, so numbers
  are readable rather than flickering; a frame with no measurement keeps the last.
- `_show`, `_hide`.
- `void voe_editor_frame_breakdown_draw(voe_ui_context *ui, voe_editor_frame_breakdown *breakdown,
  voe_math_float2 at)`: an anchored panel at `at`, a title row "Frame" with its × (as errors.c
  draws its title row), one row per pass, name left and milliseconds to two places right, then a
  total row; "No GPU timings" when none were ever taken.
- `bool voe_editor_frame_breakdown_clicks_read(const voe_ui_context *ui,
  const voe_editor_frame_breakdown *breakdown)`: the × fired.
- Header: what it shows and why it lags (0358, render's comment), the refresh rate and why, its
  node and element cost (rows × nodes and characters, the figures card 08 adds to the budget).
- `editor/src/src.md`: entries for both files.

Nothing calls it yet; card 08 wires it in.

## Done when
- `cmake --build --preset debug --target voe_editor` exits 0.
