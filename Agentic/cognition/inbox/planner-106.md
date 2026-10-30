## Learned
- Every LOD bound is made in one place, the store's `3d/src/models_levels.c` (error_bound, then made non-increasing); the cooked file carries errors and swaps, not bounds, so a change there reaches load, cook and Play alike.
- Foliage dissolves too: a band's level-before copies are `rest` groups dithered the other way, drawn by the camera and never cast (`3d/src/draw_foliage.h`).
- "LODs of <file>" is `editor/src/levels_ask.c`; its "Swap distance" header sits over the × number box, with no distance shown anywhere.
