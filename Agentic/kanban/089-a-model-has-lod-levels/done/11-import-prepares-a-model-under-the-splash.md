# 11 — Import prepares a model's levels under the splash
folder: editor
after: 09
decisions: 0168, 0426

## Change
0426 point 3: after a `.glb` is copied in and its materials written, a splash wait "Preparing
<file>" makes its levels file with `voe_game_models_prepare` (`game/include/game/models.h`), so
placing it costs nothing. Shaped as the New/Open load already is (`session.load_due`).

- `editor/src/assets_panel.h` / `assets_panel.c`: `voe_editor_assets_import` gains
  `char imported[VOE_3D_MODEL_PATH]`, set to the copied file's project-relative path (`Assets/…`,
  as `voe_editor_model_import` is given it) after a successful write, empty on any failure or a
  path that does not fit. The comment says so.
- `editor/src/session.h` / `session.c`: the session gains `bool prepare_due` and
  `char prepare_path[VOE_3D_MODEL_PATH]`, commented beside `load_due` (set by an import of a
  `.glb`, cleared by main.c once the wait has run). `session_import` passes the buffer and sets
  them when the path ends `.glb` in any case. The IMPORT_FILE sentence in the browser-do comment
  names the prepare.
- `editor/src/loading.h` / `loading.c`: a third work. `voe_editor_loading_prepare`: device-free
  context of `const char *folder`, `const char *path`, `voe_base_arena *scratch`, and a phase buffer
  it owns (the progress holds the pointer). `voe_editor_loading_prepare_work`: phase "Preparing
  <file name>", then `voe_game_models_prepare`; a false is one line on stderr, not a false of the
  work (the model loads and fails as any broken file does); false only once stopped. Scratch
  cleared. The header's first paragraph says three works and gains a PREPARE sentence.
- `editor/src/main.c`: beside the `load_due` block (around its line 447), the same shape for
  `session.prepare_due`: the context filled from the session's project folder and `prepare_path`,
  the wait run with `splash_held ? &splash : NULL` and `worker_scratch`, a false wait ends the
  program as the load's does, `prepare_due` cleared after. 744 lines: add only this block.
- `editor/src/src.md`: the `loading.h` entry names the prepare.

## Done when
`voe_editor` builds in the folder's check with no warning. The human: import a downloaded `.glb`
with no levels; the splash shows "Preparing <file>" once, and
`<project>/Cache/levels/Assets/<file>.levels` exists. Close and reopen the project: no splash
says "Preparing" again (How to test steps 1 and 3).
