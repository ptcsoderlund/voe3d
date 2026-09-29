# 0285 — Every drag shows a ghost, and the ghost says when it cannot be dropped
date: 2026-09-29
by: tech-lead

## Decision
Every drag and drop in the editor, from any panel (Scene list, Assets panel, and any added later),
shows 0282's ghost from the moment the drag starts: a raised panel with the dragged thing's name
following the pointer. While the pointer is over a place a release would do nothing (a panel or
row that does not take this kind of thing), the ghost is drawn dimmed, like 0282's dimmed row, and
shows a second line, "Can't drop here". Over a place that takes it, the ghost is drawn normally
and the target shows its own highlight as before. A release over a refusing place drops nothing.
The accept/refuse answer comes from the same function the release uses, as in 0282.

## Reasoning
The sponsor wants one drag behaviour everywhere and to know before letting go whether a drop will
land. Dimming reuses 0282's pushed theme and the monochrome palette (0194/0196) has no warning
colour, so words carry the refusal. Rejected: a "not allowed" pointer shape (0227 names only
arrow and resize shapes, and a platform change for one hint); dimming alone with no words (too
easy to miss on a small ghost); a ghost only on some panels (what the sponsor reported as a bug).

## Replaces
nothing. Extends 0282 from the Scene list to every drag.
