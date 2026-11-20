# 117 — needs decision: what a grown leaf is

## Question
What geometry does the tree maker grow for leaves from the leaf material? 0510 says only "leaves of a leaf
material on the outer branches". "Leaves are cards of the leaf material" is an `[assumed]` line by the
tech-lead in `Agentic/cognition/tree-maker.md`. Precog flags it as open and says to ask, not settle. Every
leaf card in 117 depends on the answer: the leaf geometry, what "leaf size" and "leaf density" mean in the
Inspector, how the tree maker builds a variation's far levels, and whether the forest stays within 114's 3 ms
foliage budget. That budget is spent on overdraw from cut-out leaves (0508, `foliage-cost.md`).

## Options
1. **Cards.** Each leaf, or each cluster of needles, is a small textured quad, single or crossed, cut out by
   the leaf material's base-colour alpha and drawn double-sided (0404 gives the material cut-out and
   double-sided). Size scales the card and density sets how many cards sit on each outer branch. Far levels
   use fewer, larger cards. This is what SpeedTree and Sapling do. Triangles stay few, the cost is overdraw,
   and 114's depth-first lay is built for that cost. A needle-leaf material is a picture of a needle spray.
2. **Leaf-shaped geometry, no cut-out.** Each leaf is a small opaque polygon fan in the material's colour,
   with nothing discarded. There is no cut-out overdraw, but a forest has millions of triangles. Needles do
   not read at that count, and far levels have to thin the leaves out, which shows as popping.
3. **Cards near, the whole crown as an impostor far.** Same as 1 up close, but the farthest level swaps the
   crown for a few large cards with a picture of it rendered by the tree maker. Far cost is lowest. It adds
   a render-to-texture step in the grower and a texture made per variation, which goes against "nothing but
   the species file is kept" unless that texture is also made again at load.

## Recommendation
Option 1. It is how tree tools work, it is the cost 114 already pays down, and it needs no new pass. Option 3
can follow as its own work order if the hill's far forest measures over budget.

## Also open, smaller (the planner can settle these if the tech-lead prefers)
- **Play and ship.** The game can grow variations at start from a cooked species table, so the game library
  carries the grower in `3d`, as it already carries foliage_grow. Or the cook can write the grown meshes into
  `Build/` (0235 allows that). Recommended: grow at start under the splash. It keeps one grower and fits
  0510's "nothing but the species file is kept".
- **Variation count changes.** When the number of variations changes, painted trees may pick their
  variation again. Recommended: yes. The pick is a hash of the copy's (kind, x, z), taken modulo the count,
  as lods_kept.h already keys a copy. It is stable across opens, which is all How to test step 6 asks for.
