# 05 — The levels pop, and I cannot choose when they swap

## Seen
It's popping. I think I need more control over when the different ones should swap.

Found while looking: a level is used once its error is under about a pixel on screen (bound 0.002 × radius ÷
error, `3d/src/models_levels.c`), for every model alike, with no setting and no blend between levels. The error
is only how far the surface moved, so a level that smooths hard edges (Medium, Strong) also changes the shading,
and that pops even when the shape stays within a pixel.

## Expected
I want to choose, per model, when its levels swap (next to the count and strength), and the swap should not pop.

## How to reproduce
1. Place a model with made levels and fly slowly away from it: each change of level is seen as a pop.
