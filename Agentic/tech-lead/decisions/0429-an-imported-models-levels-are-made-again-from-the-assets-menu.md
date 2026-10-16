# 0429 — An imported model's levels are made again from the Assets menu
date: 2026-10-15
by: planner

## Decision
For 089 bug 02.

1. A model imported before 089 needs no re-import: the loader already makes its levels file the first
   time the model is loaded (0426 point 2).
2. To make a model's levels where a person can see it, the Assets panel's right-button menu over a
   `.glb`'s row holds **Make levels**, after Import textures. It runs the same splash wait as Import's
   prepare ("Preparing <file>", 0426 point 3), and makes and writes the levels file even when the one
   there is fresh.
3. `voe_game_models_prepare` takes `bool remake` for it: false keeps a fresh file, as Import and the
   loader do; true makes the file whatever is there.
4. The model in the store is not loaded again: the making is the same for the same bytes, so the levels
   it draws are already the ones just written. Replaced by 0431.

## Reasoning
A re-import (0403) rewrites the model's materials and drops the map paths a person assigned, a high
price for testing levels. The levels are a cache, so the action that matters is making them again in
sight; the splash wait and the prepare already exist, and one flag and one menu row reuse both.

## Replaces
Nothing.
