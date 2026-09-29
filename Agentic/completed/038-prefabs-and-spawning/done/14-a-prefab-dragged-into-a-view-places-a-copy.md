# 14 — A prefab dragged into a view places a copy
folder: editor
decisions: 0168, 0283, 0277

## Change
Needs card 09. 0283 point 7.

- `editor/src/assets_panel.h`, `editor/src/assets_panel.c`: a row whose name ends `.prefab`, in
  any case, is a prefab row: `voe_editor_assets_row` gains `prefab`; it is drawn marked as a
  model row is, and `held` is set for a held prefab row as for a model row. Header: the prefab
  row, beside the model row paragraph.
- `editor/src/entities.h`, `editor/src/entities.c`: `[[nodiscard]] bool
  voe_editor_entities_prefab_add(voe_ecs_world *world, const char *path, voe_math_double3
  position, voe_ecs_entity *out);` — as `_model_add`: named after the path's last name less
  `.prefab` by the file's name rules, a transform at `position` with no turn and scale one, and a
  `voe_scene_prefab` row naming `path` (shorter than `VOE_SCENE_PREFAB_PATH`). The world step
  expands it (card 09). Header: the third make, beside the model's.
- `editor/src/assets_drag.h`, `editor/src/assets_drag.c`: the drag remembers whether it holds a
  prefab (the path buffer sized for the larger of the two paths). A prefab released over a view
  places a copy at the same point a model lands, selected, one undo step and unsaved as a model's
  place is; over the Inspector or anywhere else, nothing. Header: the fourth outcome.
- `editor/src/src.md`: the changed entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's,
after card 13's make: drag `tank_body.prefab` into a view three times: three tanks stand where
dropped, each listed as a prefab with its turret under it; Save, reopen: all four are there.
