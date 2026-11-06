# 0508 — Cut-out foliage lays its depth first and is shaded once a pixel
date: 2026-11-06
by: planner

## Decision
For 114.

1. **The camera draws each seen foliage group twice, inside the span it is drawn in today.** First a
   lay step: a depth-only pipeline whose fragment runs only the cut-out and LOD-dither test of the
   shaded fragment, discarding exactly where it does, writing depth and no colour. Then a shade
   step: the solid pipeline's own stages unchanged, depth compare EQUAL, no depth write. Both steps
   of the plain groups sit in `foliage`, both of the blending groups in `foliage blend`, so each
   line reads the whole cost.
2. **render holds the step as pass state**, as it holds two-sidedness (0421 point 2): one call sets
   whole (today's single draw), lay or shade for the mesh draws after it; every pass opens whole; a
   step other than whole outside a camera pass, or under a blended draw, is the caller's bug. Two
   pipelines are added, lay and shade, both with dynamic cull. A lay draw counts as a draw and adds
   no triangles, so the triangle count still means what the picture holds.
3. **Nothing else changes.** The cascades, the bounce sun maps, captures, point shadows, models and
   landscapes draw as they do. Any later caller may use the steps; none is made to.
4. **0375 point 4's condition is not met.** The forest is a few instanced draws per pass, and cutting
   shadows or LODs changes nothing, so the cost is pixels, not draws: indirect draws, GPU culling
   and cooker merging still wait for a breakdown that names draws.

## Reasoning
The leaves are cut out by `discard` in a pipeline that writes depth, so each layer of leaves behind
the nearest is shaded in full before it is hidden, and the driver may test depth late for all of
them. Laying depth first makes every hidden layer cost one texture read, and the shade step's EQUAL
test lets only the kept surface through, once a pixel, however deep the canopy (How to test 3).
- Alpha-to-coverage: needs multisampling and softens the cut edge; the look must not change.
- Sorting copies near to far: leaves of one tree still overdraw each other, and discard with depth
  write may still keep the test late.
- Forcing early fragment tests in the one pass: a discarded pixel would still write depth.
- A shade step with no discard: a dissolving copy's outgoing and incoming LODs share vertices, so
  both would pass EQUAL where only one was laid. Keeping the same fragment keeps the dissolve
  exact; with depth writes off, a discard no longer holds the depth test back.
- A depth pass for the whole world: reaches past this feature; the breakdown has not named terrain
  or models.

## Replaces
Nothing. Extends 0442 point 5 and 0464 point 4: the camera draws each seen group twice.
