# 13 — A tree dragged into the Assets panel becomes a prefab
folder: editor
decisions: 0168, 0283, 0282, 0277

## Change
Needs cards 05 and 10. 0283 point 6.

- `editor/src/prefabs.h`, `editor/src/prefab_make.c` (new):
  - `[[nodiscard]] bool voe_editor_prefab_refused(const voe_ecs_world *world, voe_ecs_entity
    root, voe_editor_notice *why);` — true, with why, when the tree under `root`
    (`voe_scene_parent_tree`) holds a camera, a light, a prefab row or a part row. Card 15 uses it
    for a prefab's own Save too.
  - `[[nodiscard]] bool voe_editor_prefab_make(voe_editor_project *project, voe_ecs_entity root,
    const char *shown, voe_base_arena *scratch, voe_editor_notice *why);` — `shown` is the Assets
    panel's folder relative to `Assets/`. Refused with a notice: untitled (no folder), a root
    with no identity, `_refused`, or `<folder>/Assets/<shown>/<name>.prefab` existing
    (platform/file.h). Else writes `voe_authoring_prefab_write`'s text there
    (`voe_platform_file_write`), then queues a `voe_scene_prefab` row on the root with the
    project-relative path and a `voe_scene_prefab_part` row naming the root on every entity of
    the tree, root included. False with why on any failure, nothing queued. Header points: 0283
    point 6; the file is written before the rows, so a failed write changes nothing; undoing the
    step leaves the file.
- `editor/src/scene.h`, `editor/src/scene_list.c`, `editor/src/scene_list.h`: a drag under way
  over the Assets panel has that as its target (no row rim; the ghost still follows); released
  there, the drop leaves the held entity in a new `list_made` field for the caller and parents
  nothing. A part is never held (card 10), so never made from.
- `editor/src/interface.c`: before the drop, whether `root->pointer.at` is over the Assets leaf
  of the dock tree — found the way `editor/src/assets_drag.c` finds the Inspector's rectangle;
  share that helper through `editor/src/dock.h` / `dock.c` rather than copying it — handed to
  the drop; after it, a set `list_made` calls `voe_editor_prefab_make` with `session->project`,
  `scene->assets.shown` and `&session->notice`, counts one in `scene->structural` on success
  (one undo step, unsaved, as Add entity), and clears the field.
- `editor/src/src.md`: entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's,
in a copy of `examples/tank_game`: drag `tank_body` (the turret under it) from the Scene list into
the Assets panel: `tank_body.prefab` is listed, the Scene list marks `tank_body` as a prefab and
nothing moved; Ctrl+Z makes it a plain tree again; dragging the Camera there is refused in the bar.
