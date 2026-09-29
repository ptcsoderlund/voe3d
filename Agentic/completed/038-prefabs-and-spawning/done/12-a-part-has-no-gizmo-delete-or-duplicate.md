# 12 — A part has no gizmo, Delete or Duplicate; duplicating a copy places another
folder: editor
decisions: 0168, 0283, 0205

## Change
Needs card 09. 0283 point 5, the rest of the editor's share. Whether an entity is a part is
"has a `voe_scene_prefab_part` row naming another entity" (scene/prefab_component.h).

- `editor/src/scene.c`, `editor/src/scene.h`: Delete and Duplicate (keyboard and Inspector reach
  them here) do nothing on a part. Deleting a placed copy's root deletes its tree, as today.
- `editor/src/entities.c`, `editor/src/entities.h`: `voe_editor_entities_duplicate` of an entity
  with a `voe_scene_prefab` row queues only its identity (new id and name by the file's rules),
  transform, parent and prefab rows, so the copy is expanded by the world step (card 09). The
  header's duplicate paragraph says so.
- `editor/src/gizmo.c`, `editor/src/gizmo.h`: no handle is hovered or grabbed on a part.
- `editor/src/view_passes.c`: no gizmo is drawn for a selected part; its outline still is.
- `editor/src/assets_drag.c`, `editor/src/assets_drag.h`: a model released over the Inspector
  while a part is selected does nothing.
- `editor/src/src.md`: the changed entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's,
on the card 09 setup: the copy's turret, selected, shows an outline and no gizmo, and Delete and
Ctrl+D leave it; Ctrl+D on the copy's root makes a second copy with both parts; Ctrl+Z removes it.
