# 09 — A row dragged onto another parents it, and onto the heading unparents it
folder: editor
decisions: 0168, 0281, 0271, 0204

## Change
Needs card 08. 0281 points 4 and 7. `ui` already says which row the pointer went down on and
still holds (`voe_ui_button_action(...).held`) and where a node was drawn
(`voe_ui_node_visible`, `ui/include/ui/layout.h`); a press and release on the same row stays a
click that selects, as now.

- `editor/src/scene.h`, `editor/src/scene.c`: new field `voe_ecs_entity list_held` in
  `voe_editor_scene`, kept across frames (not cleared by `_rows_clear`). Its comment: which
  row is being dragged; a world swapped under it (New, Open, undo) leaves it stale, which the
  release below tests for.
- `editor/src/scene_list.h`, `editor/src/scene_list.c`:
  `void voe_editor_scene_list_drop(voe_editor_scene *scene, const voe_ui_context *ui, bool down,
  voe_math_float2 at);` — read after `voe_ui_frame_end`, like the clicks. While a recorded row
  is held, remember its entity in `list_held`. On the first frame none is held and `down` is
  false with `list_held` set, that is the release, at `at` (the root's millimetres). A held
  entity no longer alive, or either entity without a transform, does nothing; otherwise:
  - over another recorded row: `voe_scene_parent_set(world, held, that row's entity)`, unless it
    is already that parent or `voe_scene_parent_within(world, that entity, held)` (onto itself
    or something under it: nothing);
  - over the `heading` node: `voe_scene_parent_set(world, held, (voe_ecs_entity){ 0 })` when it
    has a parent row, else nothing;
  - anywhere else: nothing.
  A set counts one in `structural` (which marks unsaved and one undo step, `main.c`); a false one
  sets `full`. Then `list_held` is zeroed. Header points: the three outcomes, that the world place
  is kept (`scene/parent_system.h`), and why it is read after the frame.
- `editor/src/interface.c`: call it right after `voe_editor_scene_clicks_read`, with
  `root->pointer.down` and `root->pointer.at`.
- `editor/src/src.md`: the `scene_list`, `scene` and `interface.c` entries mention the drop.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's,
in the editor: drag one thing onto another — it nests and nothing moves in the view; drag it
onto "Scene" — it is a root again where it was; Ctrl+Z puts it back under its parent.
