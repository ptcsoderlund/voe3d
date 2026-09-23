# 0229 — The views stay stacked, and their share is the split's `fraction`
date: 2026-09-23
by: planner

## Decision
For 022. The two scene views stay a COLUMN split, one above the other, as the default tree has them since
card 064; the feature's "left" and "right" read as "top" and "bottom", and the pointer over their border is
the up-down shape. The share 0228 names is that split's existing `fraction`, the first (top) view's part of
what the split divides after its seam. When laid out it is clamped so each view keeps
`VOE_EDITOR_DOCK_VIEW_ROOM` along the split; the clamp is on what is shown, and the `fraction` a person set
is kept. A drag writes it through `dock`'s own call; a double-click puts back the default tree's 0.5.

It is kept in `<settings>/voe3d/editor_settings` (0226) as one more line, `view_share`, `%.3f`, read only
when finite and strictly between nought and one.

## Reasoning
The feature asks for a draggable border, not a new arrangement; turning the views side by side would change
every picture's shape and 013's "top view" without being asked. `fraction` is already the tree's number for
a split that holds no child, so a share needs no new field, and a `dock` call that writes it keeps the
tree's state inside `dock`.

## Replaces
nothing
