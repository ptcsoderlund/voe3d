# 0396 — A landscape is drawn by CDLOD from a heights texture, up to 2048 cells
date: 2026-10-09
by: planner

## Decision
1. **Cells.** A `.landscape` may hold up to 2048 cells a side (`VOE_ASSETS_LANDSCAPE_CELLS_MAX`); Create
   still writes 512, the size range stays 16–8192 m, and the file stays 0379's text. A 4 km terrain at
   2048 cells is about 2 m a cell, and 2049² texels sits inside Vulkan's guaranteed 4096 a side.
2. **Cells box.** The Landscape panel gets a Cells box beside Size: a multiple of 4 from 4 to 2048, the
   heights resampled bilinearly, the file written at once, no undo step, as Size is (0379 point 6).
3. **CDLOD.** A loaded landscape is one R32F texture of its (cells + 1)² heights in metres, read in the
   vertex stage, and one geometry the store shares: a grid of 32 × 32 quads whose vertices have y 0 or 1,
   so its box is a unit cube. A node at level L covers 32·2^L cells at a step of 2^L cells; its world
   matrix is the landscape's transform times its box (box height at least 1 mm), so render's bounding
   spheres and point-shadow faces stay right. In the vertex stage the grid point morphs toward the next
   coarser grid by the eye's distance, its height is four texel loads blended by hand (R32F filtering is
   not guaranteed), its normal is a central difference at the node's step, and points past the grid's
   edge are clamped to it. Level 0's range is twice a leaf node's diagonal, each next range doubles, and
   a node morphs over the last 30% of its range; neighbours then differ by at most one level, so no crack.
4. **Selection.** On the CPU, from a min/max pyramid of the heights, from the frame's eye taken into the
   landscape's space; every pass of a frame selects from the same eye, so shadows, captures and the view
   draw the same ground. No frustum test: culling is 093 (0395). At most `VOE_3D_LANDSCAPE_NODES` (1024)
   nodes a landscape a pass; past it refining stops and coarser nodes are drawn. At most
   `VOE_3D_LANDSCAPES_DRAWN` (4) landscape rows a frame; a further one is not drawn and says so once.
5. **Sculpting.** A brush or put refreshes the pyramid over its rectangle and grows the entry's dirty
   rectangle; the frame writes the dirty texels into the texture before its first pass from a per-slot
   staging capacity (`heights_texels`), at most `VOE_3D_LANDSCAPE_WRITE_TEXELS` (512², shared by every landscape) a frame, the rest
   carried to the next. Chunk parts, transient landscape geometry and the settle go.
6. **Its share of the frame.** render times named spans inside a pass; the view pass's terrain draws are
   the span `terrain`, listed after its pass as `<pass name>: terrain`, part of that pass and not beside it.
7. **Seeing it.** Editor views' far plane is 16000 m. While flying, the wheel scales the fly speed by
   1.25 a notch between 0.5 and 1000 m/s, per view; Shift still triples it.

## Reasoning
CDLOD morphs geometry continuously, so there is no pop and no skirt, and one grid geometry and a texture
cost a few MB of vertex data instead of 16 chunk meshes per landscape; it fits render's object record and
its static geometry with one new texture kind and a write. Clipmaps need toroidal updates and a second
mesh scheme; chunked LOD with skirts pops. Keeping the text file and the C cook keeps 0379 and 0236 whole;
at 2048 cells the cooked table is large but compiled once per Play. Selection on the CPU per pass is
microseconds at seven levels, and keeps 0375 point 4's wait on GPU culling.

## Replaces
0379 point 1's cells limit of 512, point 2's 16 chunk parts and point 4's transient chunks and settle.
