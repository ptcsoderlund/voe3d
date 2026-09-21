# 05 — Ctrl+Z, Ctrl+Shift+Z and Ctrl+Y in the loop
folder: editor
decisions: 0168, 0204
read: feature.md

## Change
The wiring that makes cards 03 and 04 a feature. Read `editor/src/session.h` and main.c's header.

`editor/src/session.h` and `session.c`: the session struct gains `bool replaced`, set true wherever a
different project lands in `session->project` — NEW in `voe_editor_session_do`, and the browser's Confirm in
OPEN mode in `voe_editor_session_browser_do` — and cleared by whoever acts on it. The header says why it is a
flag and not a return value: the two places that replace a project sit in two calls, one of them made from
interface.c, and the loop is the only thing that needs to hear about it.

`editor/src/main.c`:

- beside `scene`, `views` and `browser`: a `voe_editor_undo undo;`, created from `arena` once it exists; and
  `bool step_back = false, step_forward = false, at_rest = false;` outside the loop.
- at the top of the loop, before `voe_ecs_structure_apply`: when `session.replaced`, clear it and
  `voe_editor_undo_forget`; otherwise, when `step_back` or `step_forward`, `voe_editor_undo_take(&undo,
  session.project, &scene, &session.notice, step_forward)` and, when it answers true,
  `voe_editor_session_edited(&session)` — an undone project is still unsaved (feature.md step 8) — and never
  `voe_editor_undo_edited`, a step not being an edit to record. Both flags are cleared whether one fired or
  not.
- after the four systems have run and before `voe_app_frame_open`: `voe_editor_undo_settle(&undo,
  session.project, arena, at_rest)`. It is here and not above because an edit reaches the world through an
  intent or the queue, and the world only holds it once these have run.
- where `quiet` is worked out for Delete and Ctrl+D, beside it:
  `at_rest = !left && !voe_ui_typing(ui) && !browser.showing && !scene.picking.open &&
  !scene.dropdown.open;` and then, from card 02's key frame, `step_back` is Control down, Shift up, Z
  pressed; `step_forward` is Control down with either Shift down and Z pressed or Y pressed. Both only while
  `!quiet && at_rest`.
- beside the existing `voe_editor_session_edited(&session)` that a replaced component or a structural change
  fires: `voe_editor_undo_edited(&undo)`.
- the header gains a paragraph: Ctrl+Z undoes and Ctrl+Shift+Z or Ctrl+Y redoes, neither while the browser
  shows or a field holds the keyboard, and neither while a drag, a picker or an open list is in the middle of
  something — the same rest a step is recorded at; the step itself is taken at the top of the next frame,
  before the queue and the systems, and what a step is is undo.h's.

`editor/editor.md`: the sentence about each edit marking the project unsaved gains that every such change can
be undone with Ctrl+Z and redone with Ctrl+Shift+Z or Ctrl+Y, newest first, one drag, one typed commit or one
visit to the colour picker at a time; that a new change after undoing throws away what could have been
redone; that selecting is not a change; that the history survives Save and that New or Open starts an empty
one; and that the shortcuts do nothing while a field holds the keyboard or the browser shows.
`editor/src/src.md`: `session.h`'s entry gains the flag that says a different project is in place.

## Done when
`checks.sh --folder editor` exits 0, and at a running `voe_editor` on a project with two or three shapes:
dragging a cube's X in one drag and pressing Ctrl+Z puts it back in one step, Ctrl+Shift+Z puts it forward
and Ctrl+Y does the same; deleting an entity and pressing Ctrl+Z brings it back with its name and every
component; Ctrl+Z inside a number box being typed into changes no history; and Save then Ctrl+Z undoes the
change before the save and leaves the bar saying unsaved.
