# 07 — The toggle opens and closes Project and Errors
folder: editor
after: 06
decisions: 0168, 0351, 0363

## Change
0363 point 4, for card 08's menu.

- `editor/src/session.h` and `session.c`: the static show of `errors` from the project's build log becomes
  `void voe_editor_session_errors_show(voe_editor_session *session)`, every caller in `session.c` using it.
- `editor/src/panels.h` and `panels.c`:
  - `voe_editor_panels_toggle` gains `voe_editor_project_panel *project`, `voe_editor_preferences
    *preferences` and `voe_editor_session *session`. PROJECT: shown → hidden, else shown and Preferences and
    Errors hidden. ERRORS: shown → hidden, else `voe_editor_session_errors_show` and Project and Preferences
    hidden. Neither writes the settings file.
  - `bool voe_editor_panels_open(voe_editor_closable which, const voe_editor_dock_root *root, const
    voe_editor_project_panel *project, const voe_editor_session *session)` — the menu's tick.
  - The header says what each does and that a failed build still shows Errors through the session (0351).
- `editor/src/interface.c`: its toggle call passes the three.
- `editor/src/src.md`: `panels.h` and `session.h` entries.

## Done when
- The folder builds.
- `grep -q voe_editor_session_errors_show editor/src/panels.c` exits 0.
