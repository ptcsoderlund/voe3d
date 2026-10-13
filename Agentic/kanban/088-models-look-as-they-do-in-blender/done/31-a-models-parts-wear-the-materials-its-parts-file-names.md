# 31 — A model's parts wear the materials its parts file names
folder: 3d
after: 30
decisions: 0168, 0404, 0420

## Change
0404 and 0420 point 1 in the store and the draw: part materials leave the model row for the store's
entry. The game and the editor stop building until cards 33 and 34; do not touch them.

- `3d/include/3d/model_component.h`, `3d/src/model_component.c`: the `materials` field and
  `VOE_3D_MODEL_MATERIALS` go; the drain corrects only `path`. Header: the MATERIALS[I] paragraph is
  replaced by one saying a part's material is the model's (3d/models.h, 0404), never the row's.
- `3d/include/3d/models.h`: `voe_3d_model_part` gains `uint32_t source` (the glTF material index the
  part was merged from, VOE_ASSETS_MODEL_NONE for glTF's default; a picture's, landscape's or material's
  parts NONE) and `char wears[VOE_ASSETS_MATERIAL_PATH]` (the `.material` path it wears, "" for the
  file's own). New `void voe_3d_models_parts_set(voe_3d_models *models, const char *path, const
  voe_assets_parts *parts)`: each part of `path`'s loaded model takes `parts->materials[source]` into
  `wears`, "" past `count` or for NONE; NULL `parts` empties every `wears`; nothing for a path that is
  no loaded model. A load leaves `wears` empty. The header's store paragraph says so.
- `3d/src/models.c`: `source` from the bake part's `material`; `_parts_set`.
- `3d/src/draw_group.h`, `3d/src/draw_group.c`: `voe_3d_draw_group_part_worn(models, model, part)`
  loses its row parameter and reads `model->parts[part].wears` through `voe_3d_draw_material_named`
  (`3d/src/draw_material.h`); its comment follows. Callers in `3d/src/draw_system.c`,
  `3d/src/draw_shadows.c` and `3d/src/draw_bounce.c` follow.
- `3d/tests/model_component.c`: the materials cases go. `3d/tests/draw_material.c`: the model cases
  give materials through `_parts_set` instead of the row. `3d/tests/models.c`: a loaded model's
  `source`s match its glTF materials, `_parts_set` fills and empties `wears`, and a reload empties them.
- `3d/3d.md`, `3d/tests/tests.md`, `3d/src/src.md`: entries for the model component, the store and
  the changed tests follow (each at most 300 characters).

## Done when
The tests `3d/models`, `3d/draw_material` and `3d/model_component` pass after the folder's build.
