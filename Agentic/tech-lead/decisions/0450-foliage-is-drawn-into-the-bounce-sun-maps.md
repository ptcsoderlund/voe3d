# 0450 — Foliage is drawn into the bounce sun maps
date: 2026-10-21
by: tech-lead

## Decision
For 090 bug 04, amending 0442 point 5. A landscape's foliage copies are drawn into every bounce sun map,
the shadow pictures from which the probes are relit, just as they are drawn into the view's cascades: the
same instanced groups, at the levels the frame chose for them. So the ground under a forest is in its trees'
shade in the bounce as well as in the view, and the probes in and under the canopy are relit by a dark floor,
not a sunlit one. Foliage stays out of the probes' captures and the point shadows, as 0442 says: the bounce
does not see the trees themselves, only their shade. The bounce keeps no falloff with height: light off a
wide floor does not fade with distance, and what darkens a real canopy is its own shade.

## Reasoning
- With no trees in the sun maps, the bounce treats the forest floor as sunlit grass: leaves high and low take
  the same strong green bounce, and the floor in the trees' cast shadows bounces as if they were not there.
- 0442's reason for leaving foliage out covered captures and point shadows only, whose instance index counts
  faces; a sun map is one view like a cascade and draws the groups as they are.
- Rejected: foliage in captures too, so leaves block each other's bounce (a draw per copy or a new instance
  scheme; kept in `ideas.md` until the hill shows it is needed); a falloff of bounce with height (not how
  light off a floor behaves, and it would darken tall things in open fields too); leaving it.

## Replaces
Nothing. Amends 0442 point 5: foliage is in the bounce sun maps; captures and point shadows unchanged.
