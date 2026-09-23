# 04 — The top bar is as tall as its text needs
folder: editor
decisions: 0168, 0219, 0225

## Change
The bar is laid out at the height its content measured the frame before, as 0225 says.

- `editor/src/topbar.h`
  - `VOE_EDITOR_TOPBAR_HIGH` stays, meaning the first frame's height, before a measure.
  - `voe_editor_topbar` gains `voe_ui_node panel` (recorded by the draw) and `float high`: the height the
    bar is laid out at, nought meaning `VOE_EDITOR_TOPBAR_HIGH`.
  - `float voe_editor_topbar_high(const voe_editor_topbar *bar)` — the height to lay the bar out at now.
  - `void voe_editor_topbar_measure(const voe_ui_context *ui, voe_editor_topbar *bar)` — after
    `voe_ui_frame_end`, sets `high` to the panel's `voe_ui_node_measured` y; a refused node leaves it.
  - Header: "A FIXED HEIGHT" becomes the bar sized by what its buttons and labels measured last frame, and
    why (0225); the interface still decides where it sits.
- `editor/src/topbar.c` — the panel is fixed at `voe_editor_topbar_high(bar)`; the row inside takes its
  natural height (no fixed height across), so the panel's measure is its content's. The two new functions.
- `editor/src/interface.c` — every use of `VOE_EDITOR_TOPBAR_HIGH` becomes `voe_editor_topbar_high` of the
  bar being drawn; `voe_editor_topbar_measure` is called every frame in the read window, whatever the
  browser or Preferences show.
- `editor/src/interface.h` — the paragraph EACH ROOT GETS THE TOP BAR says the bar takes its measured
  height off the top.
- `editor/src/src.md` — the `topbar.h` entry says the bar fits its content.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. With `XDG_CONFIG_HOME` at a scratch
folder whose `voe3d/theme_scalars` holds `1.000 1.000 2.000 near_black`, a capture at 1280x720 shows the
bar's buttons whole, their borders below the letters; a capture holding `0.500` shows a bar shorter than
without the line.
