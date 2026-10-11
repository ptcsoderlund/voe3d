# 0402 — A landscape is painted in up to eight material layers, blended by height and hex-tiled
date: 2026-10-11
by: planner

## Decision
For 086, carrying out 0376's material layers on 0379's landscape:
1. **Layers in the file.** A `.landscape` gains `[Layers]` with `layer0=` … `layer7=`, each a quoted
   project-relative `.material` path (`VOE_ASSETS_LANDSCAPE_LAYERS`, 8). Layer 0 is the ground that
   covers everything; no layers draws as before. Quoted, so the follow (0378) rewrites them and
   `.landscape` joins the walked kinds; the store's rename follows them in memory.
2. **Paint.** Every height point holds one weight 0–255 per layer, summing to 255. Layer 0 is the
   rest and never written. `[Paint]` holds `paint<k>_<r>=` for layers k ≥ 1 and rows r, `cells + 1`
   whole numbers a row, a run of one value written `v*n`. No `[Paint]` reads as all layer 0; a point
   whose written weights pass 255 is MALFORMED. In memory two RGBA8 planes, layers 0–3 then 4–7, NULL
   meaning all layer 0. A resample is bilinear per weight, the rest given to layer 0.
3. **The brush.** `VOE_3D_BRUSH_PAINT` with a layer, 0379's radius, strength, softness and weight. A
   stamp moves the layer's share up toward strength·weight by min(1, 8·s) of the gap, never down,
   and scales the others down so the point still sums to 255. A light stroke caps at its strength.
4. **On the card.** Two RGBA8 DATA textures a landscape, FIELD-sampled (bilinear, one level), made at
   load; painted texels written inside a frame by `voe_render_texture_write_pixels`, sharing the
   heights staging and `VOE_3D_LANDSCAPE_WRITE_TEXELS`. The object record's two reserved words become
   `paint` (each texture's index plus one) and it gains `layers`, two four-word vectors of shading
   record indices. Each frame the draw takes each layer's record from the store's material entry;
   a layer whose material is not loaded draws with the landscape's own ground record.
5. **The blend.** A layer's maps are read at landscape metres × its material's repeat. A layer's
   height is its ORM map's red, else its colour's luminance. Of the layers with weight above
   nought, s = height + weight; those within 0.2 of the highest s blend by s − (max − 0.2),
   normalised. Colour, normal and ORM blend by it; alpha and lit come from the object's record (a
   fading landscape fades whole). The capture pass blends colour the same way.
6. **No grid of copies.** Every layer map is read hex-tiled: the three nearest cells of a hex grid
   in uv, each sampled at a hashed offset (no rotation, so normal maps need no turning), blended by
   barycentric weight sharpened by colour luminance, with the unoffset uv's gradients so no mip
   seam shows. A layer whose blend is nought is not sampled.
7. **The editor.** A Layers section under Sculpt lists each layer by its material's file name;
   pressing one chooses painting it with Sculpt's sliders, again chooses none. A `.material`
   dropped on a row replaces that layer; on the last row ("Drop a material to add a layer") adds one.
   Adding or replacing marks the landscape edited and the project unsaved, no undo step; removing
   a layer waits for need. A paint stroke is one undo step carrying its rectangle's weights.
8. **Saved and shipped.** Save writes layers and paint with the heights (0379 point 6). The cook
   writes a `uint8_t` array of one plane, or two past four layers; the game table's entry gains its
   layer paths and paint (NULL with none); the game loads every layer's material as rows' are.

## Reasoning
- Weights at the height points: one grid, one rectangle per stroke for undo, write and cook, and
  sculpting and painting share every index. At 2048 cells a texel is 2 m, which the height blend
  breaks up below the texel.
- Run-length text over a PNG beside it: keeps 0379's one file and its follow; an unpainted layer row
  is `0*513`.
- Height blending over a linear mix: a linear mix is the smear the feature rules out; ORM red is
  already dark in a stone's cracks, so moss and grass sink into them with no new map slot.
- Hex tiling over a second scale or a noise tint: a second scale still repeats from far; hex cells
  never line up. Three taps a map is paid only by layers present at the pixel.
- Eight layers in two textures: a fourth and fifth texture slot each would cost a pass per pixel;
  eight covers the hill's four and more.

## Replaces
nothing. Carries out 0376's painted layers on 0379 and 0396's landscape.
