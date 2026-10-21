# 04 — The forest floor bounces sunlight into the canopy

## Seen
The ground bounces light 20 metres up into the leaves, which looks weird. Under a painted forest the
floor bounces as if it stood in open sun: foliage is left out of the bounce entirely
(`3d/src/draw_foliage.h`, "FOLIAGE IS NOT IN THE BOUNCE OR THE POINT SHADOWS", 0442 point 5), so the
trees neither shade the ground the bounce sees nor block what a probe sees. A probe sees twelve of its
grid's cells out, which on the hill's coarse level grid is a hundred metres or more.

## Expected
Two things, both:

1. The forest darkens the bounce: the trees shade the ground in the relight's sun map, so a floor under
   a canopy bounces what reaches it, not full sun. Whether the trees also block a probe's view is the
   planner's call.
2. A bounce falloff setting I can turn: how far bounced light carries, so I can shorten it by eye
   where the look calls for it.

## How to reproduce
In lumberhill, with the sun's bounces at 1 or more, paint a forest on the hill and look up into the
canopy from below: the leaves' undersides are lit by the floor as brightly as over open ground.
