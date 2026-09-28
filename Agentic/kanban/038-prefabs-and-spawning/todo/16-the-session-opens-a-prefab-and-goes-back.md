# 16 — The session opens a prefab and goes back, and the level keeps its undo
folder: editor
decisions: 0168, 0283, 0204, 0188, 0264

## Change
Needs card 15. 0283 point 8, the session's and the undo line's share.

- `editor/src/undo.h`, `editor/src/undo.c`: `voe_editor_undo` gains a second line set aside
  (its states, count and at), both pushed by `_create`. `void voe_editor_undo_aside(voe_editor_undo
  *)` swaps the line in force aside and empties the other, whose first settle records the prefab;
  `void voe_editor_undo_restore(voe_editor_undo *)` swaps the set-aside line back in, dropping the
  prefab's. `_forget` empties both. Header: the two lines and why the level keeps its own (its
  states are texts that re-expand from the saved prefab files); the constraints paragraph's
  memory doubles.
- `editor/src/session.h`, `editor/src/session.c`:
  - `VOE_EDITOR_COMMAND_BACK`: refused once while the prefab is unsaved (armed, as Close), then
    `voe_editor_project_prefab_back`, clearing the selection and closing the picker and dropdown
    as NEW does; a failure is the notice. With no prefab open it does nothing.
  - `void voe_editor_session_prefab_open(voe_editor_session *, voe_editor_scene *, const char
    *path);` — clears the notice and disarms as a command does; refused with a notice while a
    prefab is open; else `voe_editor_project_prefab_open`, clearing the selection as NEW does.
  - Two flags beside `replaced`: `prefab_opened` and `prefab_closed`, set by those two on success
    and cleared by main.c.
  - PLAY and SHIP are refused with a notice while a prefab is open (Back first). CLOSE, NEW and
    OPEN count the project as unsaved when the prefab or the set-aside level is.
  - The header: BACK and the open in the command paragraphs; Play's and Ship's refusal.
- `editor/src/main.c`: where `session.replaced` is read, `prefab_opened` calls
  `voe_editor_undo_aside` and `prefab_closed` calls `voe_editor_undo_restore`, each clearing its
  flag and focusing the views on the world's camera as `replaced` does.
- `editor/src/src.md`: the changed entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/tank_game --capture "$d/t.png" && test
-s "$d/t.png"` exits 0. Nothing presses Back or opens a prefab until card 17; the human's check is
there.
