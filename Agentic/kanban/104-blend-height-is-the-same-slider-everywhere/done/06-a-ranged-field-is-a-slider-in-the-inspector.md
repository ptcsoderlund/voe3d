# 06 — A ranged field is a slider in the Inspector
folder: editor
after: 05
decisions: 0168, 0196, 0494, 0498

## Change
A described FLOAT32 or FLOAT64 field of rank 0 whose struct gives it a range
(`voe_base_ranges_find`, base/describe.h) is drawn as its title, a slider over the range and a
figure, instead of a number box. Every other field is drawn as today.

- `editor/src/inspector_value.h` / `.c`: `void figure(char *out, size_t size, double value,
  const char *unit)`: the value at two decimals, then a space and the unit when it is not empty
  ("0.50 m", "0.35"). The one formatter for a ranged value; card 07's foliage row uses it too.
- `editor/src/inspector.h`: `voe_editor_inspector_control` gains `const voe_base_field_range
  *range`, NULL for every control but a slider; its comment says a set range makes the control a
  slider read through `voe_ui_slider_action` over that range.
- `editor/src/inspector_rows.c`, in the field row: for such a field, editable, the title label,
  then `voe_ui_slider` (ui/slider.h) keyed as the field's number box is, 40 mm wide like the Foliage
  section's sliders, handed the stored value clamped into the range first, so a value outside it
  shows at the nearer end and is not rewritten until moved; then a label with the figure, formatted
  into a buffer in the frame's arena (labels are not copied). The control is recorded with `range`
  set and `writes` the field's kind. Not editable: the title and the figure as labels. Mind the node
  count per control the panel assumes (inspector.h: three nodes to a control; a slider is three).
- `editor/src/inspector_edit.c`: a control with `range` is read through `voe_ui_slider_action` with
  its least and greatest instead of the number box's read; on `changed` the value goes out as the
  replace intent exactly as a dragged number does, with `held` treated as the number box's so a drag
  is live and one undo step, as a dragged box is today.

Header points: `inspector_rows.h` — a real field with a range is a slider with its figure, the
range and unit the description's (0494, 0498); `inspector_edit.h` — a slider's read and that a value
outside the range is not written until moved. `editor/src/src.md`: the `inspector_rows.h`,
`inspector_rows.c`, `inspector_value.h` and `inspector_edit.c` entries say the slider and the figure.

## Done when
`grep -q voe_ui_slider_action editor/src/inspector_edit.c` and `grep -q voe_ui_slider
editor/src/inspector_rows.c` exit 0, and the editor builds.

The human's, in the editor on the hill: a model with Blend with ground on shows Blend height as a
slider with "0.50 m" that stops at 0.05 m and 10.00 m, the blend following the drag; Fade is a slider
0–1; a transform's position is still number boxes; Ctrl+Z puts a dragged value back.
