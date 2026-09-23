# 0230 — The views' seam is drawn as a dark and a light stripe
date: 2026-09-23
by: planner

## Decision
For 022, bug 01. The seam between the two scene views (the split whose children are both SCENE_VIEW leaves)
is no longer an empty gap: it is filled edge to edge by two stripes side by side along the split, each half
the seam, the top one in the palette's `inverse_ink` and the bottom one in its `inverse`. The seam keeps its
size and place, so the arrangement, the views' room and resize.h's hit band are unchanged. The walk is
handed the palette in force to draw them. Every other seam stays a gap.

## Reasoning
Both views clear to the engine's near-black whatever the theme, so a gap showing the ground vanishes in a
dark theme, and a scene's content may match any single colour. `inverse` and `inverse_ink` sit at the text's
and the ground's lightness, a pair legible by construction (0196), so at least one stripe stands out against
anything beside it; both carry the theme's one hue (0194), so no colour of its own is added. The side panels'
seams sit between surfaces and were not reported; drawing them too would change the whole editor's look for
a fault it does not have.

## Replaces
nothing
