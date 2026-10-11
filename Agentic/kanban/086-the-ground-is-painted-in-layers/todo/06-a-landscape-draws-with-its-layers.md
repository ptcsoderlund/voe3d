# 06 — A landscape draws with its layers
folder: 3d
after: 03, 05
decisions: 0168, 0402

## Change
0402 point 4, the draw's half: every record a landscape node is drawn with names its paint and its
layers' shading records, in the view, shadow and capture passes alike.

- `3d/src/draw_terrain.h`, `3d/src/draw_terrain.c` — when the entry's grid has layers, each node
  record gets `paint` from `voe_3d_models_terrain` (index plus one, both textures) and `layers[i]`
  for i < layer_count the shading index of `voe_3d_draw_material_named(models, layers[i])`'s
  part (`material` record, opaque; the twin is not used, the object's own record carries the
  fade), or the landscape part's own record when that is NULL; unused layers repeat layer 0's.
  With no layers `paint` stays nought and the record is as before. The lookups are done once per
  landscape row per call, not per node. Header point: each frame asks the store again, so an
  Inspector tint or a reloaded map shows at once (feature step 6).
- `3d/tests/draw_terrain.c` — a new case: a 16 m, 4-cell landscape seen straight down filling
  the picture under a sun from above; two loaded materials, red and green
  (`voe_3d_models_load_material` with no maps), as layers 0 and 1; layer 1 put at 255 on the
  right half with `_put_paint` and written by `_landscape_frame` inside the frame. The ground is
  lit (its own record), so compare channels: the left reads red above twice its green, the right
  green above twice its red; `voe_3d_models_material_set` turning red to blue and one more frame
  reads blue above twice red on the left; a layer whose material is not loaded draws the
  ground's own colour there.
- `3d/src/src.md`, `3d/tests/tests.md` — entries for `draw_terrain.c` mention the layers.

## Done when
Tests `3d/draw_terrain`, `3d/terrain_shadow` and `3d/models_landscape` pass.
