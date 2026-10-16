# 0435 — Each LOD has its own reduction and its own swap, and own LODs are measured
date: 2026-10-16
by: tech-lead

## Decision
For 089 bug 08. The engine still picks a model's LOD by its error on screen (0425 point 3) and still dissolves
between two LODs across a band (0432 point 3). Four things change.

1. **Reduction per LOD.** Each made LOD from LOD 2 on has its own triangle share, as a percentage of the
   full model (LOD 1). The defaults are 50, 25, 12, 6 and 3 %, which is half per LOD as Medium makes them
   now. Each share must be lower than the one before it. The simplifier still gives up when a collapse
   costs too much. A LOD it cannot bring under the one before it is dropped, along with the LODs after it.
   The panel shows the triangles each LOD really got. Strength (Gentle, Medium, Strong) now only sets how
   edges are softened and how far a collapse may go. Its cut per level is gone.
2. **Swap per LOD.** Each LOD from LOD 2 on has its own multiplier on the distance its error allows. The
   default is ×1. ×2 swaps in twice as far away and looks better; ×0.5 swaps in twice as close, saving
   triangles and looking worse, which is the dev's call to make. It is a multiplier, not an amount added
   to the error, because error is in the model's own metres. ×2 means the same on a handle as on a wall.
   A LOD never swaps in nearer than the LOD before it: the bounds stay in order whatever the
   multipliers say. This replaces 0432's single per-model `swap`.
3. **Own LODs are measured.** When the levels file is made, each of a `.glb`'s own `_LOD<n>` levels gets
   an error measured against LOD 1, in the same metres as a made LOD's error. Own LODs then swap by
   pixels like made ones and take the same per-LOD multiplier. Their reduction is not settable; the panel
   shows their real triangle count instead. This replaces 0425 point 4's size rule (2^-k). A model is
   still either all own LODs or all made ones.
4. **Kept and carried.** The values live in the model's `.parts` `[Levels]` section. The multipliers are
   carried in the levels file, because a shipped game reads no `.parts`. They are asked in "LODs of
   <file>", one row per LOD with its share and its swap. Changing only swaps takes effect without
   preparing the model again. Changing a share makes the LODs again under the splash, as Make LODs does
   now. The planner picks the keys, the file version, the rows' widgets and how old `.parts` files read.

## Reasoning
The sponsor wants the dev to own when each LOD swaps and how much it drops, without moving to distances in
metres or to an automatic system with no handles. Values per LOD let both the drop in triangles and the swap
distances follow a curve the dev shapes across the LODs, instead of one fixed factor per step. The error rule
stays as the baseline because it already holds for any model size, scale, resolution and field of view; the
multiplier only moves a swap from where looks-right would put it.

Alternatives weighed:
- Switch distances in metres. These ignore scale and field of view and throw away the error rule.
- Keeping the single per-model swap. It cannot treat one LOD differently from another.
- An error offset added to the error. Its meaning depends on the size of the model.

A player's quality setting can later be one more multiplier over all of these. Meshlets and Nanite-style
detail are rejected (ideas.md).

## Replaces
- 0430 point 2: Strength no longer sets the cut per level.
- 0432 point 1: one swap per model becomes one per LOD.
- 0425 point 4: own LODs swap by measured error instead of 2^-k.
