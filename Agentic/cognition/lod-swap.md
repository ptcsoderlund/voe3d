# LOD swap

## `3d/src/models_levels.c`
- Every LOD bound is made here and only here, in the store: `error_bound`, then the bounds are made non-increasing. The cooked file carries errors and swaps, not bounds. [observed · planner-106 · 2026-10-30]
- So a change to the bounds here reaches load, cook and Play alike. [inferred · planner-106 · 2026-10-30]

## `3d/src/draw_foliage.h`
- Foliage dissolves between levels too: a band's level-before copies are `rest` groups dithered the opposite way, drawn by the camera and never cast as shadows. [observed · planner-106 · 2026-10-30]

## `editor/src/levels_ask.c`
- The "LODs of <file>" dialog puts its "Swap distance" header over the × number box; no distance is shown anywhere in it. [observed · planner-106 · 2026-10-30]
