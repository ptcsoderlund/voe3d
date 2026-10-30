# 106 — LODs swap calmly

## What
A model's LODs never sit on top of each other: whatever swap values I type, each LOD shows over its own
stretch of distance, and the Swap distance column shows the distance actually used. A tree or a model
swaps to a coarser LOD as I move away, and only swaps back once I have come a bit nearer again, so
creeping back and forth at one distance does not flip it between two LODs. This holds for a placed
model and for painted foliage, in the editor, in Play and in a shipped build (0495).

## Why
LODs that flip back and forth at a boundary draw the eye, and settings like ×0.5, ×0.5 should not
quietly hide a LOD.

## How to test
1. Open my tree's "LODs of <file>" and set its swaps to ×0.2, ×0.5, ×0.5, ×1, ×0.8. Each LOD's Swap
   distance is farther than the one above it, by at least a quarter. No two are the same.
2. In the editor, look at one tree with its LOD showing in the Inspector. Fly slowly away until it
   swaps to the next LOD. Creep back a little: it keeps the coarser LOD. Come about a tenth nearer: it
   swaps back, dissolving as before.
3. Hover at one distance near a swap and nudge the camera back and forth: the tree does not flicker
   between LODs. Do the same at the edge of the painted forest: the forest does not shimmer.
4. With both scene views showing the forest from different places, each view's trees swap by its own
   camera.
5. Press Play and fly the same path: the same calm swaps. Ship and run the build: the same.
6. Open a project saved before this: models look as before, except a LOD that had been hidden by
   overlapping swaps now shows over its own stretch.
