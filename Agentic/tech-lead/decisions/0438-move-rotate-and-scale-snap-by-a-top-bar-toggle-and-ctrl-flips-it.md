# 0438 — Move, rotate and scale snap by a top-bar toggle, and Ctrl flips it for one drag
date: 2026-10-16
by: tech-lead

## Decision
The editor gains snapping for the move, rotate and scale gizmos. A Snap toggle sits in the top bar, with one
step box each for move (metres, default 0.5), rotate (degrees, default 15) and scale (default 0.1). With snap on,
a moved position lands on whole multiples of the move step on the world grid, along each axis being dragged; a
turn goes in whole steps of the rotate step; a scaled value lands on whole multiples of the scale step. Holding
Ctrl during a drag flips snap for that drag (off when on, on when off). The toggle and the steps are the
person's, kept in the editor settings file (0220), not the project. Typed Inspector values are never snapped.
There is no scale gizmo yet, so 095 adds one beside move and rotate: R steps Move → Rotate → Scale, each axis
scales alone and a centre handle scales all three together.

## Reasoning
A visible toggle says when snapping is on; Ctrl makes the one exception without a trip to the top bar.
- Ctrl only (Unity): nothing shows the state, and a long session of aligned placing means holding a key.
- Toggle only: every one-off exception is two clicks.
- Steps from where the drag began: keeps an object's odd offset, so things placed apart never line up.

## Replaces
nothing
