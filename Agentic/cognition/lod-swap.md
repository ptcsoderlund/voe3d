# LOD swap

## The bounds
- Every LOD bound is made in one place, the model store: the error bound, then the bounds made non-increasing. The cooked file carries errors and swaps, not bounds. [observed · planner-106 · 2026-10-30]
- So a change to the bounds reaches load, cook and Play alike. [inferred · planner-106 · 2026-10-30]
- The "LODs of <file>" panel measures a model's bounding radius the same way the store does, so the panel and the store agree on the radius a swap distance is figured from. [observed · planner-106 · 2026-10-30]
- 106's three open points were taken as recommended ("B, A, B": a timed dissolve, Swap distance in metres, a lossless LOD's bound capped at s = 1), with no reason of the sponsor's own. See 0500. [assumed · tech-lead · 2026-10-30]

## The dissolve
- Foliage dissolves between levels too: a band's level-before copies are dithered the opposite way, drawn by the camera and never cast as shadows. [observed · planner-106 · 2026-10-30]
- A timed dissolve needs no shader change: an object's dither (and a foliage copy's) already carries a fraction in (0,1] and its complement in [−1,0). [observed · planner-106 · 2026-10-30]

## The memory
- A view's LOD memory may be megabytes: editor and game arenas grow by blocks and a push never fails. [observed · planner-106 · 2026-10-30]
- A copy past the memory's room loses the swap-back hysteresis too, not only the dissolve. See 0501. [assumed · planner-106 · 2026-10-30]
- Shadows choose through the same memory as the camera pass; a second choose in one frame answers as the first, so it does not matter whether shadows or the camera draw first. [assumed · planner-106 · 2026-10-30]
- The Inspector's LOD row reads the view's memory, and before a view's first frame falls back to the stateless distance LOD. [assumed · planner-106 · 2026-10-30]

## Where it draws
- Play and a shipped build draw through one frame path, so a swap change reaches both; the dev build hands no models to the frame and draws no model LODs. [observed · planner-106 · 2026-10-30]

## The editor panel
- The "LODs of <file>" dialog puts its "Swap distance" header over the × number box; no distance is shown anywhere in it. [observed · planner-106 · 2026-10-30]
- The default lens that Swap distance's metres are read under is a 60° vertical view; until 106 nothing named it. See 0501. [observed · planner-106 · 2026-10-30]
