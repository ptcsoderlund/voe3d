# 0500 — A swap dissolves over a quarter second, and Swap distance is metres
date: 2026-10-30
by: tech-lead

## Decision
Settles what 0495 left open for work order 106:

1. **A swap dissolves over time, not over a band of distance.** Each model copy and foliage copy keeps,
   per view, the LOD it shows and the moment it last swapped. At a swap, in either direction, the old
   and the new LOD dissolve into each other over 0.25 s, at whatever distance the swap happened. A copy
   that has finished its dissolve draws one LOD whole, so a copy standing still is never stippled. A
   copy creeping back inside the 10% shows the coarser LOD whole; once it is 10% nearer it swaps back and
   dissolves then. A copy seen for the first time takes its distance's LOD with no dissolve. Shadows and
   held parts still draw one LOD whole, the kept one.
2. **The 1.25 gap stays, for a stretch per LOD.** Bounds are still spread so each LOD swaps in at least
   1.25 times as far away as the one before, so every LOD shows over some stretch of distance.
3. **Swap distance shows metres.** The panel's Swap distance column shows the distance in metres at which
   that LOD swaps in, for the model at scale 1 seen through a scene camera's default lens, after
   spreading. The × multiplier box stays as the input, under its own "Swap" header. A LOD with no fresh
   file shows "—".
4. **Spreading is applied when read.** The spread bounds are worked out at load, at cook and in "LODs of
   <file>", and never written back: `.parts` and the levels file keep the multipliers as typed, so an old
   project reads exactly what it saved.
5. **A lossless LOD swaps in once the model stops filling the screen.** Every bound is capped at the
   model's radius filling half the screen height (s = 1) before spreading. A LOD with no error then
   swaps in at that size, and the LODs before it each get their own stretch nearer than that. Own LODs on
   the 2^-k fallback are spread like any other.

## Reasoning
A dissolve over time is the only choice where a copy keeps the coarser LOD whole and later swaps back
dissolving, as 106's test 2 asks, and it removes the stipple 0432 accepted on a copy at rest. A
multiplier scales its own LOD's error, so only a distance lets two rows be compared. An unbounded
lossless LOD would hide every LOD before it, against 0495 point 1. Alternatives: the band kept where
0432 put it (a copy creeping back half-dissolves while the panel names the coarser LOD); the band
following the direction of travel (more state, and the picture depends on the path taken); Swap
distance as a ratio to LOD 2 (independent of lens and scale, but less concrete); a lossless LOD always
allowed, its predecessors shown as never used.

## Replaces
Amends 0432 point 3 (the dissolve runs over 0.25 s at a swap, not across a band in s) and 0495 points 1
and 2 (the 1.25 gap is a stretch per LOD, not one band apart; the dissolve runs over time at each swap).
