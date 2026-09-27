# 0279 — The Assets panel's border drags, and its height is kept
date: 2026-09-27
by: planner

## Decision
For 036 bug 01. The seam between the Scene list and the Assets panel is a border like 0226's: the
column split keeps holding Assets (its second child) at a length in millimetres, default
`VOE_EDITOR_DOCK_ASSETS_TALL` (70 mm); a drag writes that length, the Scene list and the Assets
panel each keep at least `VOE_EDITOR_DOCK_PANEL_MIN` (30 mm), a double-click puts back 70 mm, and it
is lit as 0231/0232 say. It is kept in `<settings>/voe3d/editor_settings` as one more line,
`assets_tall`, `%.3f` mm, read when above nought and at most 1000. The dock node's `fixed` flag,
which only this seam used, goes.

## Reasoning
The person asked for it, and every other border already works this way; a held length and not a
share, so the panel keeps its height as the window changes, as the side panels keep their widths.
No seam is left that needs `fixed`, and a field no one sets is a question for every reader.

## Replaces
Amends 0277 point 6: the Assets panel is dragged.
