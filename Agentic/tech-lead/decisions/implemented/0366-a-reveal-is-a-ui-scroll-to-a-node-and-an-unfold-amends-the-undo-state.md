# 0366 — A reveal is a `ui` scroll to a node, and its unfold amends the undo state
date: 2026-10-05
by: planner

## Decision
For 061, how 0355 is built:
1. **`ui` gains a scroll by the program**: `voe_ui_scroll_centre(ui, area, node, axes)`, called after
   `voe_ui_frame_end`, sets the area's remembered offset so the node's centre meets the area's visible
   centre on each named axis where the node is not wholly seen; it lands in the next frame's layout and
   is clamped there, like any scroll. A node already wholly seen moves nothing.
2. **The editor keeps `revealed`, the selection the Scene list last showed.** Each frame, after the
   clicks are read, a live selection other than `revealed` that has an identity is revealed; a row click
   in the list sets both, so the list's own selection never moves the list. A new entity whose rows are
   still queued waits a frame. A selection with no row drawn (the Scene panel closed) is given up.
3. **The unfold is the identity's replace intent with `folded` false** for every folded ancestor, walked
   up by `voe_scene_parent_get` capped at `VOE_SCENE_PARENT_DEPTH_MAX`. The scroll waits for the frame
   after, when the row is drawn. Unfolds are counted apart from `structural`: they mark the project
   unsaved, but are not an edit for undo.
4. **Undo amends instead of recording.** `voe_editor_undo_revealed` says a reveal reached the project;
   the next settle at rest rewrites the state the world is at with the scene's text, pushing nothing and
   keeping what could be redone. So the next edit's step differs from it only by the edit.

## Reasoning
Scrolling to a node needs the area's remembered offset, which only `ui` owns (rule 3), and centring
needs this frame's rectangles, which `ui` already reads back after the frame. A handle compare catches
every outside path at once (view click, Add, Duplicate, drop, undo's re-found selection carries a new
handle) with no call added to each. Rejected: a reveal call at each selecting site (five places, and a
sixth forgets it); marking the reveal an edit (0355 says no undo step); leaving undo's state untouched
(the next edit's step would carry the unfold, and undoing it would fold the parents again).

## Replaces
Nothing. Carries out 0355; lifts `ui`'s "no scrolling by a program" (widgets.h).
