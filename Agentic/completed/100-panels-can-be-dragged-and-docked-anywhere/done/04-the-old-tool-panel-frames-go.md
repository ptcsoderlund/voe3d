# 04 — The tool panels' old frames go
folder: editor
after: 03
decisions: 0168, 0439, 0491

## Change
Rule 10: since card 03 the walk draws the four tool panels' frame, header and ×, so their own frames
and × are drawn by nothing.

- `editor/src/frame_breakdown.h` and `.c`: `voe_editor_frame_breakdown_draw`,
  `voe_editor_frame_breakdown_clicks_read` and `close_button` go. Header: the usage sketch and the
  "anchored panel at a point the caller gives" become the contents call in a dock panel; the NODES and
  ELEMENTS constraint drops the panel, title row and ×.
- `editor/src/errors.h` and `.c`: `voe_editor_errors_draw`, `voe_editor_errors_clicks_read` and
  `close_button` go; the header's sketch and constraint likewise.
- `editor/src/project_panel.h` and `.c`: `voe_editor_project_panel_draw`, `close_button` and the
  result's `closed` go; the header's "anchored panel over the dock" and sketch likewise.
- `editor/src/landscape_panel.h` and `.c`: `voe_editor_landscape_panel_draw`, `close_button` and the
  result's `closed` go; the header likewise.
- `editor/src/interface_read.c` and `.h`, `editor/src/interface.c`: every read of what went
  (the breakdown's and Errors' clicks reads, Project's and Landscape's `closed`) goes.
- `editor/src/interface.h`: the budget's numbers stay (an over-count is safe); the paragraphs that
  counted those title rows say the walk's headers carry them now.
- `editor/src/src.md`: the eight entries.

## Done when
- `! grep -q close_button editor/src/frame_breakdown.h editor/src/errors.h editor/src/project_panel.h editor/src/landscape_panel.h`
  exits 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
