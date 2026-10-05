# 02 — Undo keeps a reveal's unfold in the state the world is at
folder: editor
after: none
decisions: 0168, 0366, 0355

## Change
An unfold made by a reveal is not an undo step; it amends the state the world is already at
(0366 point 4). Files: `editor/src/undo.h`, `editor/src/undo.c`, `editor/src/src.md`.

- `undo.h`: a field `bool revealed` on `voe_editor_undo`, beside `edited`: whether a reveal's
  unfold has reached the project since the last settle. A new call
  `void voe_editor_undo_revealed(voe_editor_undo *undo);` after `voe_editor_undo_edited`.
  `voe_editor_undo_settle`'s comment gains the amend: at rest, with `revealed` and no `edited`
  and a state recorded, the state at `at` is rewritten with the scene's text; nothing pushed,
  nothing that could be redone thrown away; with `edited` too, the ordinary record carries it.
  `voe_editor_undo_forget` forgets a reveal as it forgets an edit. The header gains a paragraph:
  a reveal unfolds for real but is no step (0355), so it rides in the state the world is, and
  the next edit's step differs from it only by the edit; its cost: a state stepped to that
  predates the reveal folds those parents again until the selection is revealed anew.
- `undo.c`: the call setting the flag; in settle, the amend path using the same scene write and
  `voe_editor_undo_state_set` at `at`; a text too long for a state empties the line as a record
  does; both flags cleared after either path; `_forget` and the line swaps (`_aside`,
  `_restore`) clear `revealed`. Update the file's header line.
- `src.md`: the `undo.h` and `undo.c` entries name the amend.

Nothing calls the new function yet; card 03 does.

## Done when
`grep -c "voe_editor_undo_revealed" editor/src/undo.h editor/src/undo.c` prints at least 1 for
each file, and the folder check passes.
