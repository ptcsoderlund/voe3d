# 0387 — The bounce is finer near the camera, in nested world-fixed grids
date: 2026-10-08
by: tech-lead

## Decision
For 082 bug 05, amending 0331: the bounce gets the LOD system 0331 deferred. Over the level grid of 0331, which
stays as it is, two or three finer probe grids are nested around the camera, finest nearest. Their cells are fixed
to the world: moving the camera changes which cells are fine, never where a cell sits, and a cell already in a grid
is never captured or relit again because the camera moved. Each grid hands over to the next coarser one across a
blend band, so no edge, line or ring can be seen, standing or moving (0328). A cell entering a finer grid is
captured and lit under a fixed per-frame budget (0386), and its detail fades in when ready: it may arrive late, it
never causes a hitch. A camera that stands still costs only the read, as before. The finest grid is fine enough
that a 1 m box in sunlight colours the ground in front of its lit face (051 step 3) on any level, the 082 hill
included. The workload is pinned to the sun at Bounces 1: flying over the hill at editor speed holds 0388's 60 fps at
1440p with it on; more bounces and lamps may arrive more slowly but never hitch. Far from the camera the
bounce is as coarse as the level grid makes it; small things' colour is a near-camera detail. The planner chooses
the grid count, spacing, band widths and budget. The faint curved edge in bug 05's floor is fixed under the same
bug.

## Reasoning
The sponsor's hill needs small things to colour the ground, and the level grid cannot show anything smaller than
its 64 m cells there. 0331 rejected nested camera grids because the camera would then cost work and the joins
could show; the sponsor now accepts paced work while moving, as long as it stays smooth, and the blend band answers
the joins, the way terrain and model LOD hide theirs.
- A cheap local bleed from each small lit thing: cheapest, any level size, but a cheat that sees no walls and that
  this grid would replace.
- Fine grids fixed around each small thing: right everywhere, but memory and capture grow with the number of
  things, and a forest needs a cap.
- Wait for model LOD (088): the hill would ship without the tint, which the sponsor ruled out.

## Replaces
Nothing. Amends 0331: the camera now picks which world cells are fine; the level grid, its world-fixed placement
and "a still camera costs only the read" stay.
