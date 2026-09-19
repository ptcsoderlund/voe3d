# 13 — Delete and Duplicate
folder: editor
decisions: 0168, 0193, 0177

## Change
`src/scene.h` / `scene.c`: `void voe_editor_scene_delete(voe_editor_scene *scene)` and
`void voe_editor_scene_duplicate(voe_editor_scene *scene)`. With nothing selected they do nothing. Delete calls
`voe_editor_entities_delete` and clears the selection. Duplicate calls `voe_editor_entities_duplicate` and selects
the copy. Each success adds one to `scene->structural` (card 12). A false return sets `scene`'s refusal, which
`main.c` turns into the notice "The scene is full.", as card 12 does.

`src/main.c`: the Delete key (`VOE_PLATFORM_KEY_DELETE`) and Ctrl+D fire on their down edge, tracked like
Ctrl+N. Neither fires while the browser shows or while `voe_ui_typing(ui)` is true.

`src/inspector.c`: with an entity selected, a row at the top of the Inspector holds **Duplicate** and **Delete**
buttons. The buttons are recorded, and after the frame a fired one calls the same scene function as the key.
Raise `src/interface.h`'s budgets for the row.

Update `editor.md` and `src/src.md`.

## Done when
`checks.sh` for `editor` passes. With `C=$(mktemp -d)`, `./build/debug/editor/voe_editor --capture $C/o.png`
exits 0, and reading it (ADR-0177) shows no Duplicate or Delete row, because nothing is selected at start.
