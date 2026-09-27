# 17 — Import copies a .glb into the shown folder
folder: editor
decisions: 0168, 0277, 0164

## Change
Needs card 16 (0277 point 7). Read the headers of `editor/src/browser.h`,
`editor/src/session.h` and `editor/src/assets_panel.h`.

- `editor/src/browser.h`, `editor/src/browser.c`: a third mode, `VOE_EDITOR_BROWSER_IMPORT`.
  Its listing is folders as now, then the folder's `.glb` files (ignoring case, hidden left
  out), each row marked as a file; entering a folder row navigates as now; pressing a file row
  fires a new action, IMPORT, with that file's absolute path (in the browser's arena); Confirm
  is not drawn in this mode. The OPEN and SAVE listings are unchanged. Header: the mode, and why
  files show only here.
- `editor/src/assets_panel.h`, `editor/src/assets_panel.c`:
  - An `Import` button beside Up, drawn only when the project has a folder; its press is a flag
    `voe_editor_assets_clicks_read` reports.
  - `void voe_editor_assets_import(voe_editor_assets *, const char *project_folder, const char
    *source, voe_editor_notice *why);` — makes `Assets/` when missing
    (`voe_platform_folder_create`), reads `source` whole and writes it to the shown folder under
    its own name (`voe_platform_file_write`, which is atomic and overwrites), then lists again.
    A failure is said in `why` naming the file and changes nothing else.
- `editor/src/interface.c` (and `editor/src/session.c` if the browser's actions are carried out
  there, `voe_editor_session_browser_do`): the Import flag shows the browser in IMPORT mode; the
  browser's IMPORT action calls `voe_editor_assets_import` with the session's folder and notice
  and hides the browser; Cancel and Escape hide it as in OPEN.
- `editor/src/src.md`: the `browser` and `assets_panel` lines say Import.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`d=$(mktemp -d) && build/debug/editor/voe_editor examples/coin_game --capture "$d/c.png" &&
test -s "$d/c.png"` exits 0. Importing through the panel is the human's step 2, card 19.
