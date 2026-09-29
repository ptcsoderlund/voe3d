# 09 — The editor expands every placed copy from its file
folder: editor
decisions: 0168, 0283, 0204, 0277

## Change
Needs cards 02 and 06. 0283 point 4.

- `editor/src/prefabs.h` (new): what the editor does with prefabs; this card adds
  `void voe_editor_prefabs_expand(voe_ecs_world *world, const char *folder,
  voe_base_arena *scratch, voe_editor_notice *why);`. Folder NULL (untitled) does nothing. Every
  entity with a `voe_scene_prefab` row and no `voe_scene_prefab_part` row
  (scene/prefab_component.h), taken in ascending identity id, has `<folder>/<path>` read
  (platform/file.h) and handed to `voe_authoring_prefab_read` (authoring/prefab.h) with the first
  id one above the largest identity id in the world, carried from one root to the next. A file
  that will not read or is refused: `Could not read <path>` and the report's category in `why`
  (notice.h), and the root still gets its part row through a direct `voe_ecs_component_add`, so
  it is not tried again. Scratch is rewound to where it was. Header points: 0283 point 4; why it
  is a load (rule 3) and so runs only between a scene read or the world step and anything that
  reads the world; ids are deterministic for one text and one set of files, which is what lets
  undo re-find a selected part.
- `editor/src/prefab_expand.c` (new): the above.
- `editor/src/project.c`, `editor/src/project.h`: after every successful scene text read into a
  world — `voe_editor_project_new_opened`, `voe_editor_project_scene_set`,
  `voe_editor_project_code_set` — expand with the project's folder and a scratch the call
  already has; a failed prefab read is said in `why` and does not fail the call. The header's
  paragraphs on those three say so.
- `editor/src/world_step.h`, `editor/src/world_step.c`: `voe_editor_world_step` gains `const char
  *folder, voe_base_arena *scratch, voe_editor_notice *why` and expands after the game step, so
  a root queued this frame (a drop, a duplicate, card 13's make) is expanded before anything
  draws it. Header: why after.
- `editor/src/main.c`: its one call passes `session.project->folder`, the frame's scratch arena
  it already hands to `voe_editor_undo_settle`, and `&session.notice`.
- `editor/src/src.md`: entries for the two new files and the changed three.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. The human's: in a copy of `examples/tank_game`, write
`Assets/t.prefab` by hand (the tank's two things, ids 1 and 2, 2 under 1) and add to
`main.scene` an entity `[9]` with a transform and `[9.voe_scene_prefab] path = "Assets/t.prefab"`;
open it: a second tank stands there, its two parts listed under it; Save leaves `[9]` with its
three sections only.
