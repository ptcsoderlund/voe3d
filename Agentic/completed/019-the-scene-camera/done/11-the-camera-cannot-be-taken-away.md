# 11 — The camera cannot be deleted, duplicated or have its component removed
folder: editor
decisions: 0168, 0193, 0217, 0218, 0221, 0223

## Change
Decision 0218: the scene's one camera is not deletable, its component not removable, and never offered by
Add component. Card 10 gave every scene its camera; this keeps it.

- `editor/src/scene.h` / `scene.c` — `voe_editor_scene_delete` and `voe_editor_scene_duplicate` do nothing
  when the selected entity has a camera (`voe_scene_camera_get` not NULL): no queue entry, no count, no
  `full`, the selection unchanged. Both the Inspector's buttons and main.c's Delete and Ctrl+D go through
  these, so the one check covers both; the header comment of each says so and names 0218.
- `editor/src/inspector.c` — the Duplicate and Delete row is not drawn for an entity with a camera, so
  `duplicate` and `remove` stay `VOE_UI_NODE_NONE` (inspector.h already says that is "not drawn"; the reads
  in inspector_edit.c need no change). `editor/src/inspector.h`'s field comment says when they are not drawn.
- `editor/src/dock.c` — the kept list it hands the Inspector (the types whose section has no Remove,
  ADR-0221) gains `voe_ecs_component_type(scene->world, &voe_scene_camera_key)` beside the identity's and
  the transform's. Add component already offers no camera: card 01 gives it no menu path, and a type with
  no path is not offered (add_menu.h).
- `editor/src/src.md` — the `scene.c` entry: Delete and Duplicate refuse the camera.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0, and
`grep -n "camera" editor/src/scene.c editor/src/inspector.c editor/src/dock.c` prints at least one line from
each of the three files.
