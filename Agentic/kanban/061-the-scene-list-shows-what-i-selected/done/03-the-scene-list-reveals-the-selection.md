# 03 — The Scene list unfolds and scrolls to a selection made elsewhere
folder: editor
after: 01, 02
decisions: 0168, 0355, 0366
read: feature.md

## Change
Carry out 0355 as 0366 points 2 and 3 say. Files: `editor/src/scene.h`, `editor/src/scene.c`,
`editor/src/dock_walk.c`, `editor/src/interface.c`, `editor/src/frame_commands.c`,
`editor/src/src.md`.

- `scene.h`, on `voe_editor_scene`:
  - `voe_ecs_entity revealed` — the selection the list last showed, kept across frames, not
    cleared with the rows;
  - `voe_ui_node list_area` — the Scene leaf's scroll area this frame, or NONE;
  - `uint32_t unfolded` — parents a reveal opened this frame; it marks the project unsaved but is
    not an undo edit, unlike `structural`.
  New call `void voe_editor_scene_reveal(voe_editor_scene *scene, voe_ui_context *ui);`: after
  the clicks and the drop are read, in the same window. The header gains a paragraph on the
  reveal: what triggers it (a live selection other than `revealed`, so every outside path at
  once, undo's re-found selection included), the wait for a queued identity, the unfold then the
  scroll a frame later, giving up when no row is drawn, and a row click setting `revealed`.
- `scene.c`:
  - `voe_editor_scene_rows_clear` zeroes `list_area` (NONE) and `unfolded`;
  - in `voe_editor_scene_clicks_read`, a fired row sets `revealed` as well as `selected`
    (Add entity sets only `selected`, so it is revealed);
  - `voe_editor_scene_reveal`: nothing when the selection is not alive, equals `revealed`, or has
    no identity row yet. Walk up by `voe_scene_parent_get`, at most
    `VOE_SCENE_PARENT_DEPTH_MAX` links; each ancestor whose identity has `folded` gets the
    identity submitted with `folded` false (as `fold_flip` submits; a refused submit sets
    `full`), counting `unfolded`. Any submitted: return, the row is drawn next frame. Else find
    the selection in `listed`; if there and `list_area` is not NONE, call
    `voe_ui_scroll_centre(ui, list_area, row node, { .y = true })`; then set `revealed`.
  - Update the file's header.
- `dock_walk.c`: where the Inspector's area is handed on, also set `scene->list_area = area` for
  `VOE_EDITOR_PANEL_SCENE`. Check `voe_editor_scene_rows_clear` runs before the walk draws.
- `interface.c`: call `voe_editor_scene_reveal(scene, ui)` after `voe_editor_scene_list_drop` and
  the prefab make; mention it in the header's order of reads.
- `frame_commands.c`, `voe_editor_frame_commands_after_draw`: when `scene->unfolded > 0`, call
  `voe_editor_session_edited` and `voe_editor_undo_revealed` (not `_edited`). Header line too.
- `src.md`: the entries for `scene.h`, `scene.c`, `interface.c` and `frame_commands.c` name the
  reveal.

## Done when
`grep -c "voe_editor_scene_reveal" editor/src/interface.c` prints 1,
`grep -c "voe_editor_undo_revealed" editor/src/frame_commands.c` prints 1, and the folder check
passes.

Human: walk `## How to test` in `feature.md`, steps 1–6, in the tank game.
