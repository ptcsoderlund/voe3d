# 0255 — The editor's views open on the scene's camera
date: 2026-09-25
by: planner

## Decision
For 027: when a project comes in — at startup and whenever a different project replaces the
one open (New, Open) — every scene view's orbit focus is set to the position of the world's
camera entity (0223: every scene has exactly one). Yaw, pitch and distance stay the first ones
the views are made with. A world with no camera, or a camera with no transform, leaves the focus
at the origin. A Refresh or a Play that swaps the world for the same scene does not move the
views; nothing is saved (ADR-0125 still holds: the view is never authored).

## Reasoning
With every entity 100 km out (feature step 16) the views opened about the origin, 100 km from
anything, and flying there at 12 m/s takes hours: the person and `--capture` both need the views
to open where the scene is. The scene's camera always exists, is placed where the author wants
the scene seen from, and moves with the scene; the scene's bounds would need a rule for which
entities count and a fitted distance for no gain in this feature.

## Replaces
nothing. Amends the views' first focus in `editor/src/view.h` ("looking at the origin").
