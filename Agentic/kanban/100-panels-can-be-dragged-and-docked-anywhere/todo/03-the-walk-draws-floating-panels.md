# 03 — The walk draws the tool panels and every floating panel
folder: editor
after: 02
decisions: 0168, 0439, 0491, 0363

## Change
0491 points 2, 3, 5: Frame, Project, Errors and Landscape become dock panels drawn by the walk,
floating at their default boxes; any floating panel is drawn as a box over the dock.

- `editor/src/dock.h`:
  - `voe_editor_dock_contents` — pointers to the frame breakdown, Project panel, Errors and
    Landscape panel, and the project's window: what the four tool panels draw from.
  - `voe_editor_panel voe_editor_closable_panel(voe_editor_closable which)` — the panel a closable
    is drawn as; PANEL_COUNT for BOTTOM_VIEW.
  - `voe_editor_dock_walk` and `voe_editor_panel_draw` gain `voe_editor_dock_contents *contents`.
  - `voe_editor_dock_closes`: `close` widened to `[VOE_EDITOR_CLOSABLE_COUNT]`; gains
    `voe_ui_node header[VOE_EDITOR_CLOSABLE_COUNT]`, each header row drawn, VOE_UI_NODE_NONE else.
  - `voe_editor_dock_closes_read` answers for every closable.
  - Header: the walk's paragraph says it draws floating boxes too.
- `editor/src/dock.c`: `voe_editor_closable_panel`.
- `editor/src/dock_walk.c`: every laid leaf with a closable, the four included, gets today's header;
  `voe_editor_panel_draw`'s four cases call card 02's contents calls. After the tree, each closable
  floating and not closed, in rising `raised`: an anchored raised panel inside the dock's container at
  `voe_editor_dock_float_shown` less the area's top left, holding the same header and a scroll area
  with `voe_editor_panel_draw` in it. Header comment: the floating boxes paragraph.
- `editor/src/interface.c`: fills the contents and passes them to the walk; no longer draws the four
  as overlays; only the browser, Preferences and today's lists and questions hand the dock a pointer
  with `over` false — Project, Errors and Landscape no longer do (0491 point 5).
- `editor/src/frame_pointer.c`: the borders' `allowed` no longer goes false for Project, Errors or
  Landscape showing. A field of `frame_pointer.h` this leaves unused goes, with its fill in `main.c`.
- `editor/src/interface_read.c` and `.h`: a fired × of PROJECT, ERRORS or FRAME goes through
  `voe_editor_panels_toggle` as a docked one's does; LANDSCAPE through
  `voe_editor_landscape_panel_hide`. The old title-row × reads stay until card 04; they never fire.
- `editor/src/interface.h`: count what the walk adds — four headers, four anchored panels and four
  scroll areas — on top, in a paragraph like the others, and raise the three numbers. The paragraphs
  on Project, Errors, Landscape and the breakdown say they are dock panels now (0491).
- `editor/src/src.md`: the `dock.h`, `dock_walk.c`, `interface.h` entries.

## Done when
- `grep -q voe_editor_dock_contents editor/src/interface.c` and
  `! grep -q voe_editor_frame_breakdown_draw editor/src/interface.c` exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
