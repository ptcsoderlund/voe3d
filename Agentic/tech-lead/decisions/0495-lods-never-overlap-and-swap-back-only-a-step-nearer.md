# 0495 — LODs never overlap, and swap back only a step nearer
date: 2026-10-30
by: tech-lead

## Decision
1. **No overlap.** Every LOD a model has is shown over some stretch of distance. A LOD swaps in at
   least 1.25 times as far away as the LOD before it does. That is the width of 0432's dissolve band
   (down to 0.8 of a bound), so one dissolve never runs into the next. Swap multipliers that would put
   two LODs closer, such as ×0.5 followed by ×0.5, are spread to that gap wherever the swaps are read:
   at a load, a cook and in "LODs of <file>". The panel's Swap distance column shows the distance
   actually used. This tightens 0435 point 2's "the bounds stay in order", which allowed two LODs the
   same bound.
2. **Swapping out and back in happen at different distances.** A copy moving away swaps to the coarser
   LOD at that LOD's bound, as now. It swaps back to the finer LOD only once it is 10% nearer than that
   bound. A copy standing in that band keeps the LOD it has. So the choice keeps a little state: the
   LOD each model copy and foliage copy last showed, kept per view because each editor view has its own
   eye. A copy seen for the first time takes the LOD its distance gives, with no band. The dissolve
   still runs across its band whenever a swap happens. Shadows still draw the camera's LOD, and bounce
   captures still draw the last (0425).
3. **Order.** This is work order 106, built after 104. The hill becomes 107 and stays 0.3's last
   work order.

## Reasoning
The sponsor wants LODs that do not flicker at a boundary and settings that cannot hide a LOD. A
stateless choice changes LOD every time a copy crosses its bound, and the dissolve band only softens
that change. A gap of one band keeps two dissolves apart. Alternatives: a wider dissolve band (still
stateless, but each swap takes longer and costs more fill); refusing overlapping multipliers in the
panel only (a file written by hand or before this would still overlap); hysteresis without the gap
(LODs could still sit on top of each other). A LOD index per copy per view is small next to what the
gather already works out for every copy each frame.

## Replaces
Amends 0425 point 3 (the choice is no longer stateless), 0432 point 3 (the band now runs at a swap
the state decides), 0435 point 2 (bounds at least 1.25× apart, not merely in order), 0494 point 3 and 0479 (the
hill renumbered 105 → 107).
