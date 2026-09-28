# 18 — A Scene list drag starts past a threshold, knows its target, and Escape cancels it
folder: editor
decisions: 0168, 0281, 0282

## Change
Bug 01, the state half; card 19 draws it. 0282 points 1, 5 and 6. Read the headers of the
files below; `voe_editor_scene_list_drop` (card 09) is where the drag lives today.

- `editor/src/scene.h`: beside `list_held`, kept across frames and not cleared by
  `_rows_clear`: `voe_math_float2 list_from` (where the pointer was when the row was first
  held), `bool list_dragging` (past the threshold), `bool list_cancelled` (Escape during this
  press), `voe_ecs_entity list_target` (the row a release now would parent onto, zero if none)
  and `bool list_target_heading` (a release now would unparent). One comment for the group:
  what each means and that the drag's owner is `scene_list.c`.
- `editor/src/scene.c`, `voe_editor_scene_clicks_read`: a row that fired does not move the
  selection while `list_dragging` or `list_cancelled` is set (it runs before the drop, which
  zeroes them on the release frame). Its header comment says so.
- `editor/src/scene_list.h`, `editor/src/scene_list.c`:
  - `#define VOE_EDITOR_SCENE_DRAG_START 1.0f` (millimetres).
  - Split the release's decision out of `voe_editor_scene_list_drop` into one static function
    that, for the held entity and a point, answers the entity to parent onto, "unparent", or
    nothing, with every refusal card 09 lists. The release and the target both use it.
  - `voe_editor_scene_list_drop`: on the first held frame record `list_from`; while held and
    not cancelled, set `list_dragging` once the pointer is `VOE_EDITOR_SCENE_DRAG_START` from
    `list_from`; while dragging, set `list_target`/`list_target_heading` from the function above
    at `at`, else clear them. At the release, parent only if dragging and not cancelled; then
    zero all six fields.
  - New `bool voe_editor_scene_list_cancel(voe_editor_scene *scene);` — when a drag is under
    way, sets `list_cancelled`, clears `list_dragging` and the target, and returns true; else
    false and changes nothing.
  - Header: the threshold and why (a click stays a click), that the target is the release's own
    answer, and the cancel.
- `editor/src/main.c`, the `escape_free` chain: first, if `voe_editor_scene_list_cancel(&scene)`
  returns true, the edge is spent (`escape_free = false`) before the picker and browser get it.
- `editor/src/src.md`: the `scene_list`, `scene.h` and `main.c` entries mention the threshold,
  target and cancel, each entry under 300 characters.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0 and
`bash ~/.claude/skills/checks/scripts/checks.sh --folder editor/src` prints `FINDINGS: 0`. The
human's, in the editor on `examples/tank_game`: press `tank_head`, move onto `tank_body`, press
Escape, release: nothing nests and the selection is unchanged; a plain click on a row selects it.
