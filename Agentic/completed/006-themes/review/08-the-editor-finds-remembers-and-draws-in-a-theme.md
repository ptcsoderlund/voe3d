# 08 — The editor finds, remembers and draws in a theme
folder: editor
decisions: 0168, 0167, 0170, 0172, 0175

## Change

Registration (0175): add `theme` to `editor`'s row in `cmake/voe.cmake` (`set(deps ...)` in the `editor` branch,
and a sentence in its comment citing ADR-0170), and to `editor/CMakeLists.txt`'s `DEPENDS`. It still names no
`assets`.

New `src/themes.h` / `src/themes.c` — the themes the editor has and the one in force:
- A list whose first entry is the built-in theme (`voe_ui_theme_default_inputs()`, Oxanium, no file, display
  name `Built-in`), then one per `*.theme` file in `<settings>/voe3d/themes/` (`voe_platform_folder_settings`,
  `voe_platform_folder_list`; the folder made when missing, as `src/last_project.c` makes its own), in listing
  order. Each file is read with `voe_platform_file_read` and `voe_theme_read` into an arena of its own; a file
  that refuses is left out of the list (its reason is already on stderr) and does not stop the rest. Each entry holds its
  display name, file name, `voe_theme` and derived `voe_ui_theme`, derived with whichever of the two fonts its
  `typeface` names.
- The chosen one: `<settings>/voe3d/theme`, one line, the file name, empty or absent for the built-in; read at
  startup and written by a `choose` call, the way `src/last_project.c` reads and writes its file. A remembered
  file that is gone or refused falls back to the built-in and the load says so by its return; `src/main.c`
  then fills the session's notice with `voe_editor_notice_from_report` naming the file, as a failed open does.
- The header says why each theme's arena is its own (a refused or re-read one is dropped without touching the
  others) and what a failure to read one leaves behind.

`src/main.c` creates both fonts, `VOE_TEXT_TYPEFACE_OXANIUM` and `VOE_TEXT_TYPEFACE_PIXEL_OPERATOR` (ADR-0167),
loads the themes, and sets the chosen one's `voe_ui_theme` on the context. `src/interface.c` stops deriving
its own built-in `interface_theme`. `src/dock.c` draws the selected row's name with
`voe_ui_label_role(..., VOE_UI_TEXT_ROLE_ACCENT)` and drops `SELECTED_MARK`. `src/src.md` and `editor.md`
updated.

## Done when

`cmake --preset debug && cmake --build --preset debug --target voe_editor` succeeds, and with
`C=$(mktemp -d)`: `XDG_CONFIG_HOME=$C ./build/debug/editor/voe_editor --capture $C/a.png --size 1280x800` exits 0
(built-in, and `$C/voe3d/themes/` now exists); then with `$C/voe3d/themes/paper.theme` a one-section file
setting `mode=light` and the three other required keys, and `paper.theme` as the one line of `$C/voe3d/theme`,
the same command writing `$C/b.png` exits 0 and `cmp -s $C/a.png $C/b.png` exits 1; and with
`$C/voe3d/theme` naming `gone.theme`, it still exits 0.
