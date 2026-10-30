# LOD swap

## The bounds
- Every LOD bound is made in one place, the model store: the error bound, then the bounds made non-increasing. The cooked file carries errors and swaps, not bounds. [observed · planner-106 · 2026-10-30]
- So a change to the bounds reaches load, cook and Play alike. [inferred · planner-106 · 2026-10-30]
- A model may come with no levels: a cooked file's level count is checked only from above, and making bounds for zero levels writes nothing. [observed · accept 106 · 2026-10-30]
- The "LODs of <file>" panel measures a model's bounding radius the same way the store does, so the panel and the store agree on the radius a swap distance is figured from. [observed · planner-106 · 2026-10-30]
- 106's three open points were taken as recommended ("B, A, B": a timed dissolve, Swap distance in metres, a lossless LOD's bound capped at s = 1), with no reason of the sponsor's own. See 0500. [assumed · tech-lead · 2026-10-30]

## The dissolve
- Foliage dissolves between levels too: a band's level-before copies are dithered the opposite way, drawn by the camera and never cast as shadows. [observed · planner-106 · 2026-10-30]
- A timed dissolve needs no shader change: an object's dither (and a foliage copy's) already carries a fraction in (0,1] and its complement in [−1,0). [observed · planner-106 · 2026-10-30]
- On a swap's own frame the incoming LOD's share is exactly 0 (the dissolve starts at that frame's clock), but a dither of 0 means whole. So on that frame a model row draws the old LOD alone and whole, its shadow from a separate draw of the kept LOD, and the dissolve shows from the next frame; a foliage copy instead gives the incoming LOD a dither just above 0, below the pattern's least threshold of 1/32, so it covers no pixel yet still casts, since shadow passes read no dither. The stateless choice never answers below its band's floor. [observed · coder-106-06 · 2026-10-30]

## The memory
- A view's LOD memory is 12 MiB (2^17 slots of 24 bytes, the swap moment kept as a double); editor and game arenas grow by blocks and a push never fails. The editor holds five: one per view room, made even for rooms not in use, and one for the preview, 60 MiB in all. See 0501. [observed · coder-106-10 · 2026-10-30]
- The clock a memory is begun on must never go back. [observed · accept 106 · 2026-10-30]
- A copy past the memory's room loses the swap-back hysteresis too, not only the dissolve. See 0501. [observed · accept 106 · 2026-10-30]
- A landscape row is never chosen through a memory; only other model rows and foliage copies are kept, and a single-level model row is kept too. [observed · coder-106-08 · 2026-10-30]
- Foliage chooses its LOD after its tier, so a copy left out before that (an unseen copy of a shadowless kind, even in the cast tier) touches no memory and is first sight when next seen. See 0501. [observed · accept 106 · 2026-10-30]
- A kept LOD the model no longer has (a reload, or two models' keys colliding) is taken as first sight, so a draw never reaches a level the model lacks. [observed · accept 106 · 2026-10-30]
- Shadows choose before the camera pass, through the same memory; a second choose in one frame answers as the first, so the order does not matter. [observed · coder-106-06 · 2026-10-30]
- Asking a memory for a copy answers the choice as it stood when made: last frame's at last frame's clock, this frame's at this frame's. Asking a memory never begun is an error, so each editor view begins its memory once when made; a view not yet drawn, or not laid out, then answers "not found". [observed · coder-106-11 · 2026-10-30]
- A preview that draws nothing in a frame (a blind camera) counts as not drawn, so its copies are first sight next time. [observed · accept 106 · 2026-10-30]

## Where it draws
- Play and a shipped build draw through one frame path, so a swap change reaches both; the dev build hands no models to the frame and draws no model LODs. [observed · planner-106 · 2026-10-30]

## The editor
- The Inspector's LOD row shows the view's kept level, and falls back to the stateless distance LOD only when neither of the memory's last two frames chose the row. [observed · coder-106-11 · 2026-10-30]
- The "LODs of <file>" Swap distance column shows the distance in use in metres, one decimal and the unit ("99999.9 m"); a longer distance sticks out of the column. A summary that is not fresh shows "—" on every row. See 0500. [observed · accept 106 · 2026-10-30]
- A model's level summary takes its errors from the fresh file, own LOD files included (they store measured errors); its triangles still come from the bakes. [observed · accept 106 · 2026-10-30]
- The default lens Swap distance's metres are read under is a 60° vertical view, the camera component's registered default; a camera added to a fresh world gets it, while adding a camera with a lens never reads it. See 0501. [observed · accept 106 · 2026-10-30]
