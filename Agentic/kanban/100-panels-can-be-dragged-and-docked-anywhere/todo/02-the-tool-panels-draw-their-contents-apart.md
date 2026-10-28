# 02 — The four tool panels draw their contents apart from their frame
folder: editor
after: 01
decisions: 0168, 0439, 0491

## Change
So the dock can put Frame, Errors, Project and Landscape in a leaf or a floating box under its own
header (card 03), each gets a call that draws only what is inside it: no panel, no title row, no ×,
no anchor, no scroll area. Each existing `_draw` keeps today's look by drawing its panel and title
row, then the new call; card 04 retires the old ones.

- `editor/src/frame_breakdown.h` and `.c`:
  `void voe_editor_frame_breakdown_contents_draw(voe_ui_context *ui, voe_editor_frame_breakdown *breakdown)`
  — the pass rows through the last picture's rows. Asserts when not showing, as the draw does.
- `editor/src/errors.h` and `.c`:
  `void voe_editor_errors_contents_draw(voe_ui_context *ui, const voe_editor_errors *errors)` — the
  kept lines as labels. `voe_editor_errors_draw` keeps its own scroll area round this call.
- `editor/src/project_panel.h` and `.c`:
  `void voe_editor_project_panel_contents_draw(voe_ui_context *ui, voe_editor_project_panel *panel, voe_authoring_project_window window)`
  — Width, Height and the choice, recording every control as the draw did.
- `editor/src/landscape_panel.h` and `.c`:
  `void voe_editor_landscape_panel_contents_draw(voe_ui_context *ui, voe_editor_landscape_panel *panel)`
  — a row with the file's name first (the dock's header says only "Landscape"), then Size and Cells,
  recording every control.
- Each header: a paragraph that the contents are drawn apart so the dock places the panel (0439,
  0491); the usage sketch shows the new call. Each file's node and element counts are unchanged.
- `editor/src/src.md`: the four `.h` entries name the contents call.

## Done when
- `grep -q _contents_draw` exits 0 on each of `editor/src/frame_breakdown.h`, `errors.h`,
  `project_panel.h` and `landscape_panel.h`.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
