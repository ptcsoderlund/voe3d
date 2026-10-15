# 0425 — A model's levels are made by the engine's own simplifier and chosen by their error on screen
date: 2026-10-15
by: planner

## Decision
For 089.

1. **Up to four levels.** Level 0 is the file as it is; levels 1 to 3 aim at a half, a quarter and an
   eighth of level 0's triangles (`VOE_3D_MODEL_LEVELS` 4). Each part is simplified on its own, from
   level 0, until it reaches its target or a collapse would cost more than a quarter of the model's
   bounding radius. A level with more than three quarters of the previous level's triangles ends the
   list, it and the levels after it dropped.
2. **The simplifier is the engine's own** (rule 5), internal to `3d`. Half-edge collapse onto an
   existing vertex, cheapest first by quadric error: each vertex's quadric is the area-weighted planes
   of its triangles plus a plane along each open border edge; a collapse's cost is the weighted mean
   squared distance, so its root is in model metres. Topology is welded by position. A position whose
   vertices differ in normal, uv or colour (a seam) never moves; a border position moves only along a
   border edge; a collapse that turns any remaining triangle past 90° is refused. A level's error is
   the root of the largest cost taken. Its output is indices into level 0's part vertices.
3. **A level is chosen by what it costs on screen, with no state.** `s` = the model's bounding radius
   in the world × |P[1][1]| ÷ the centre's clip w: the radius as a fraction of half the screen's height.
   A made level k is drawn while `s` < 0.002 × radius ÷ its error, which keeps its error under 1/500 of
   half the screen (about a pixel at 1080 lines): the swap moves the picture by a pixel, which is why it
   is no pop, with no blend and no second draw. A level of error nought is always allowed. An eye inside
   the sphere draws level 0. Bounds are made non-increasing; the drawn level is the largest k whose
   bound `s` is under.
4. **A Blender file's own levels.** A node whose name ends `_LOD<n>` (LOD in any case, n 0 to 3) and the
   nodes under it are level n; a node with no suffix is its parent's level, the roots' level 0; n above
   3 is left out. Level n's primitives join level 0's part of the same glTF material; one whose material
   level 0 lacks is left out, and a part a level lacks draws nothing there. Own level k is drawn while
   `s` < 2^-k: they swap as their author made them. A file with any level 1 to 3 node is not simplified.
5. **Passes.** The camera's pass and its shadows (cascades and point shadows) draw the level the camera
   chooses for each row. The probe bounce always draws the last level, so the camera never changes the
   bounce (0331). Pick, outline and bounds keep level 0.
6. **Room.** The store's geometries become 2050 (512 parts × 4 levels + 2), its vertices 3 M and its
   indices 9 M: the levels add under seven eighths of level 0.
7. **Seen.** `render` counts each mesh draw's triangles × instances, every pass, reset by begin; the
   Frame panel shows "Triangles" under its passes (0358, 0388). The Inspector shows, for a selected
   model row, the level each view draws, of how many, made or own.

## Reasoning
An error bound on screen is what makes a swap invisible without a cross-fade: no dither in the shader,
no second draw in a band, no object record grown. Screen-size ratios alone would pop on a model whose
simplification is coarse; the error says when it is safe. Own levels carry no error, so they fall back
to the size rule every DCC export assumes. The bounce reads the world, not the eye (0331), and its
low-frequency light loses nothing to the coarsest level.

## Replaces
Nothing. Carries out 0375 point 3's "LOD level" and point 5's cooked levels.
