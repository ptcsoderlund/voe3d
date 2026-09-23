# 04 — A border is dragged, double-clicked and remembered
folder: editor
decisions: 0168, 0226, 0227

## Change
The borders take the pointer: the shape over them, the drag, the double-click, and the write when it ends.

- `editor/src/resize.h`, `editor/src/resize.c` — new.
  - `VOE_EDITOR_RESIZE_REACH` 0.5f (mm either side of a seam), `VOE_EDITOR_RESIZE_DOUBLE` 0.4 (s).
  - A border is a held split's node index, `VOE_EDITOR_DOCK_NODES` for the top bar's lower edge, or
    `UINT32_MAX` for none. `voe_editor_resize`: the border held, the grab's offset from its edge, the
    last press's border and time, and last frame's button level. Zeroed-then-`held = UINT32_MAX` is at
    rest; say how a caller makes one.
  - `voe_editor_resize_result { voe_platform_cursor cursor; bool taken; bool ended; }`.
  - `voe_editor_resize_result voe_editor_resize_frame(voe_editor_resize *resize, voe_editor_dock_root
    *root, voe_editor_topbar *bar, bool allowed, double now)`: arranges the root's tree below
    `voe_editor_topbar_high(bar, root->size.y)`; hovering is a border whose band holds the pointer while
    `allowed`, `over` and the button up; a press edge while hovering holds it, unless it is the same
    border pressed within `DOUBLE`, which sets that size back (the node's length in
    `voe_editor_dock_default()`, the bar's `wanted` nought) and ends; while held, the pointer less the
    grab is the new edge, written as the node's `length` in the arrangement's `least..most` or the bar's
    `wanted` in `voe_editor_topbar_least(bar)..root->size.y - VOE_EDITOR_DOCK_VIEW_ROOM`; the release
    ends. `taken` while hovering or held; `cursor` LEFT_RIGHT on a ROW split's seam, UP_DOWN on a
    COLUMN's or the bar's, ARROW otherwise.
  - `[[nodiscard]] bool voe_editor_resize_remember(const voe_editor_dock_tree *tree, const
    voe_editor_topbar *bar)`: the three sizes written through `voe_editor_settings_write`.
  - Header points: the editor's own hit test before `ui`'s, because the seam is a gap `ui` draws nothing
    in; why a press must start over a border with the button up; the double-click timed here (0226).
- `editor/src/main.c` — one `voe_editor_resize` for the root. Each frame, after the root's pointer is
  filled and before pick, gizmo and the interface: call `voe_editor_resize_frame` with `allowed` false
  while the browser, Preferences, the colour picker or the open dropdown shows, and `now` from
  `voe_platform_clock_now()`. While `taken`, the root's pointer goes on with `over` and `down` false and
  pick and gizmo see no left button. With a window, `voe_platform_input_cursor` gets `cursor` every
  frame. On `ended`, `voe_editor_resize_remember`; false puts base/report.h's first kept error in the
  session's notice through `voe_editor_notice_from_report`. Header: one line on the borders.
- `editor/src/src.md` — `resize.h` and `resize.c` entries.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and `voe_editor --capture` as in card
03 still writes its picture with no file and with one.
