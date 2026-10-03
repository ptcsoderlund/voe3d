# 0331 — The bounce grid covers the level, not the camera
date: 2026-10-03
by: tech-lead

## Decision
For 051 bug 03, amending 0329 point 1 and 0326's placement: the bounce's probe grid is fixed to the world and
sized to cover the scene, the things that can be lit, not the camera. Where the camera stands or looks never moves
the grid, never makes a probe be captured or relit again, and never changes the bounce on anything: a thing far
from the camera gets the same bounce as one near it. Only a light or thing that moves or changes refreshes the
bounce, and only around itself (051 point 8). Once that is done, a still scene costs only the read (0317), however
the camera moves. A larger level gets the same grid stretched coarser, not darkened or cut off. Finer bounce near
the camera on very large levels waits for a LOD system, which will also make far things cheaper to capture. It is
not part of 051. The planner chooses the probe count, the spacing and how the grid fits a scene that grows or
shrinks while editing, within 0318's low end.

## Reasoning
0329 stood a 24-cell grid (about 48 m) on the eye, so things more than about 24 m away get no bounce and go
dark, and moving the camera moves that edge (bug 03, against 0328). The sponsor also does not want the camera to
cost anything: relighting is slow, and if every camera move recaptured probes it would never settle. A grid fixed
to the level answers both.
- Nested grids on the camera, fine near and coarse far: reach any distance, but every camera move recaptures
  their edges and the joins can show.
- Keep the 24 m grid on the eye and give far things a flat fill: cheapest, but far things look flat and the
  edge can still show.

## Replaces
Nothing. Amends 0329 point 1 (placement by the eye) and 0326's placement and scrolling. 0329's own sun map stays,
fitted to the grid.
