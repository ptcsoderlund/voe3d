# 12 — Changes on disk are taken in, refused, or queued to ask
folder: editor
after: 11
decisions: 0168, 0204, 0505, 0506, 0509

## Change
Make new files `editor/src/disk_take.h` and `editor/src/disk_take.c`.

- `voe_editor_disk_take` holds which held change is being asked, if any.
- `_step(take, session, scene, undo, models, scratch, now)` runs once a frame, between frames. It
  calls `voe_editor_disk_watch_look` with the project's folder, then handles each held, unsettled
  change (`voe_editor_disk_watch_changes`) by its path:
  - The open scene or the open prefab (`project.h` says which path that is):
    - When `project->unsaved` is set, the change is queued to ask.
    - Otherwise it is taken. Note the selected entity's identity id, call
      `voe_editor_project_scene_take`, select the entity with that id again (`scene.h`), and record
      the take as one undo step, the way an edit is recorded (`voe_editor_undo_edited`; read its
      header).
  - The level's scene while a prefab is open: left unsettled until Back (0509 point 4).
  - Any other `.scene`: settled, nothing taken.
  - A `.landscape`:
    - refused by the watch: the notice says so;
    - not held by the store: settled;
    - edited: queued to ask;
    - otherwise: make a landscape step from `_landscape_held` to the change's landscape, record it
      (`voe_editor_undo_landscape`), and take it in (`voe_editor_models_landscape_take` with
      `edited` false).
  - A `.material`, a `.parts`, a `.prefab` that is not open, or `project.voe3d`: call
    `voe_editor_file_take`. When it is taken and the watch knows a before
    (`voe_editor_disk_watch_known`), record a file step (`voe_editor_undo_file`).
- Before taking, a change whose content says what the editor already holds is settled without a
  step: the scene's text, the table's row, the project file's values. This is a safety net under
  the told writes.
- Refused: the session's notice says the file, line, key and why. The change is settled untaken and
  the last good state kept.
- Taken: the change is settled taken.
- One change is asked at a time; the others carry on.
- `const char *_asking(take)`: the path being asked, NULL when none.
- `_answer(take, session, scene, undo, models, scratch, bool load)`:
  - load: taken the way a clean change is, recorded as one undo step so Ctrl+Z gives mine back, and
    `unsaved` or `edited` set false;
  - keep: settled untaken. Mine stays unsaved, and the next Save writes it.
- Header points: the rule for each kind and why (0506, 0509), what waits, and the safety net.
- `editor/src/src.md`: an entry for the new files.

## Meaning
- Scout: unsaved is tracked per file in part. The scene and an open prefab share `unsaved`, the
  set-aside level has `level_unsaved`, and each landscape has the store's `edited` flag. Materials
  and the project file are never unsaved.

## Done when
- `grep -q voe_editor_disk_take_answer editor/src/disk_take.h` and
  `grep -q voe_editor_disk_take_asking editor/src/disk_take.h` both exit 0.
- `d=$(mktemp -d) && XDG_CONFIG_HOME=$d build/debug/editor/voe_editor --capture $d/a.png && test -s $d/a.png`
  exits 0.
