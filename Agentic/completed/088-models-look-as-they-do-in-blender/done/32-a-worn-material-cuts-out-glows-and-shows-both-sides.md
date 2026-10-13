# 32 — A worn material cuts out, glows and shows both sides
folder: 3d
after: 31
decisions: 0168, 0399, 0420, 0421

## Change
Card 04's material keys reach the store's records and the draw; card 27's call is made per draw.

- `3d/include/3d/material_component.h`, `3d/src/material_component.c`: `voe_3d_material` gains `bool
  double_sided` (not in the record; the draw reads it). Header paragraph: why it is the draw's and
  not the record's (0421 point 2).
- `3d/include/3d/models.h`: `voe_3d_material_maps` gains `emission`; `_load_material`'s and
  `_material_set`'s comments name the new values.
- `3d/src/models_material.c`: the values take `cutout` > 0 as CUTOUT with that cutoff (else OPAQUE),
  `emission` × `emission_strength` as `emissive`, and `double_sided`; the emission map uploaded COLOUR
  and mipped into the emissive slot; `_material_set` takes all of these live but the map. MAPS 4.
- `3d/src/model_upload.c`: a glTF material's own record also takes `double_sided`, `unlit`, and
  `emissive` × `emissive_strength` (card 03).
- `3d/src/draw_group.h`, `3d/src/draw_group.c`, `3d/src/draw_system.c`: a mesh draw (shape or model
  part, solid as found or held in a group) whose worn material is double-sided is drawn after
  `voe_render_frame_two_sided(device, true)` and followed by `(device, false)`, so every other draw is
  one-sided; a held entry carries the flag. Shadow and bounce draws need nothing. Headers follow.
- `3d/tests/models_material.c`: a material with cutout 0.4, emission 1 0.5 0 strength 2, double-sided
  and an emission map loads with those in its part's material and a fourth texture; a set changes
  cutout and emission live. `3d/tests/draw_material.c`: a cube shape wearing a double-sided unlit red
  material, seen from inside, reads red; one-sided, the clear colour.
- `3d/tests/tests.md`, `3d/src/src.md`: entries follow (each at most 300 characters).

## Done when
The tests `3d/models_material` and `3d/draw_material` pass after the folder's build.
