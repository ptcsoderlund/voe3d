# 0355 — The Scene list reveals a selection made elsewhere
date: 2026-10-04
by: tech-lead

## Decision
Whenever the selection changes from anywhere but the Scene list itself (a click in a scene view, Add entity,
Duplicate, a drop from Assets, undo or redo), the Scene list unfolds every folded parent above the selected
entity, then scrolls so its row is seen, centring it, but only if the row was out of sight. The unfold is
real: the parents stay unfolded and the next save keeps them so (0302's `folded`). Revealing is not an undo
step of its own; undo goes back to the last edit, not to the folds.

## Reasoning
The sponsor's call (2026-10-04). A selection you cannot find in the list is half a selection; doing it for
every outside change is the same work as doing it for view clicks only, and is what one expects. Rejected:
a temporary unfold that closes again when the selection moves, jumpy and a second kind of fold; making each
reveal an undo step, which would fill undo with steps nobody asked for; scrolling even when the row is seen,
which moves the list under the eye for nothing.

## Replaces
nothing. Extends 0302.
