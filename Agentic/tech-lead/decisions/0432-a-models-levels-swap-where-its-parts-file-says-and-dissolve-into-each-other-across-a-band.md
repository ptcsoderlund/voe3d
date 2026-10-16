# 0432 — A model's levels swap where its parts file says, and dissolve into each other across a band
date: 2026-10-15
by: planner

## Decision
For 089 bug 05.

1. **Swap, per model.** `.parts`' `[Levels]` (0430 point 4) gains `swap`, one of `near`, `normal`,
   `far`, `farther`, default `normal`: the distance each level swaps in at is ½, 1, 2 or 4 times what
   its error allows, so every bound of level 1 on (0425 points 3 and 4, made and own alike) is divided
   by 0.5, 1, 2 or 4. The levels question (0430 point 6) asks it in a row "Swap" under Strength; the
   cook carries it in the parts table.
2. **Carried in the levels file**, as a shipped game reads no `.parts`: levels files go to version 3,
   holding after the part count the swap's distance multiplier as a float, stale when not finite and
   above nought. The loader makes the file with the `.parts`' swap as it does count and strength; the
   store divides the bounds by the file's multiplier, 1 when there is no fresh file. Changing it is a
   Make levels (0429, 0431).
3. **Two levels dissolve across a band, with no state.** For a chosen level k ≥ 1 with a finite bound
   b_k, its band is `s` from b_k down to low = the larger of 0.8 × b_k and b_(k+1) (when level k+1
   exists). Inside it, `shown` = (b_k − s) ÷ (b_k − low): level k is drawn on that share of the
   model's pixels and level k − 1 on the rest, the two picked by one 4 × 4 ordered pattern on the
   screen, so each pixel is drawn by exactly one of them. Below low, or for level 0, an infinite bound,
   an eye inside the sphere or w not above nought, level k is drawn whole. `voe_3d_model_level` keeps
   answering the chosen k.
4. **Only the camera's at-once draws dissolve**: a model part drawn solid or cut-out in the world
   layer. Held parts (blended, fading 0336, overlay), shadows, the bounce, pick, outline and bounds
   draw the chosen level whole, as 0425 point 5 has them.
5. **`render`'s object record gains `dither`**, a float and three reserved words at its end (256 to
   272 bytes): 0 draws whole, d in (0, 1] keeps the pixels whose pattern threshold is under d, d in
   [−1, 0) those whose threshold is at or above −d; the mesh fragment stage discards the rest beside
   the cut-out's discard. A zeroed record is unchanged.

## Reasoning
0425 kept a level's shape within a pixel, but Medium and Strong (0430) soften normals, so the shading
changes at the swap and is seen however small the shape moves. Counting normals in the error would only
delay the pop. A cross-fade over time needs a level and a clock per row per view, against 0425's one
stateless answer for draws, shadows and the Inspector; a band in `s` stays stateless, at the price of a
stipple while a model rests inside it (no temporal smoothing here) and a second draw only for rows in
a band. Dither and not alpha: blending two opaque levels needs sorting and loses their depth. The swap
is a multiplier of the error's bound rather than a distance, so it holds for any size of model and keeps
the error rule's order between levels.

## Replaces
Amends 0425 point 3 (no blend, no second draw; the bound divided by the swap), 0430 point 4 (`swap`)
and point 7 (version 3), 0426 point 1 (the layout carries the swap).
