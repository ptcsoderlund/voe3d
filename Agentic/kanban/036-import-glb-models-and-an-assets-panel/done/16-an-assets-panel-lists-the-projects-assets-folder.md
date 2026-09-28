# 16 — An Assets panel lists the project's Assets folder
folder: editor
decisions: 0168, 0277, 0270, 0142, 0226

## Change
0277 point 6. Read the headers of `editor/src/dock.h`, `editor/src/scene.h`,
`editor/src/browser.h` (a listing shown as rows, the pattern to follow) and
`editor/src/interface.h`.

- New `editor/src/assets_panel.h`, `editor/src/assets_panel.c`: `voe_editor_assets` — its own
  arena, the shown folder relative to `<project>/Assets/` (empty for `Assets/` itself), the
  project folder it listed, when it last listed, up to `VOE_EDITOR_BROWSER_ROWS` rows (name,
  folder or file, model when the name ends `.glb` ignoring case, the row's `ui` node), the Up
  button.
  - `void voe_editor_assets_update(voe_editor_assets *, const char *project_folder, double
    now);` — lists again when the project folder differs from the one listed (then back at
    `Assets/`) or a second has passed: folders first, then files, each by name, hidden left
    out, through `voe_platform_folder_list`. A failed listing keeps the old rows; a missing
    `Assets/` or an untitled project is no rows and a flag the panel shows as a line.
  - `void voe_editor_assets_draw(voe_ui_context *, voe_editor_assets *);` — Up (not shown at
    `Assets/`), the shown folder's path as a label, the rows in a scroll area, a model row marked
    as the Scene panel marks a row (read `dock.c`'s Scene panel), or the line.
  - `void voe_editor_assets_clicks_read(const voe_ui_context *, voe_editor_assets *);` — a
    folder row entered, Up taken; both list at once.
  - Header points: why its own arena, why a second, why never above `Assets/`, what a model row is.
- `editor/src/scene.h`: `voe_editor_scene` holds a `voe_editor_assets assets`, so the dock
  reaches it with no new parameter; its header says so.
- `editor/src/dock.h`, `editor/src/dock.c`: `VOE_EDITOR_PANEL_ASSETS`, titled `Assets`, drawn
  through `voe_editor_assets_draw`; the default tree's left column splits the Scene list above
  and Assets below, Assets held at 70 mm (`VOE_EDITOR_DOCK_ASSETS_TALL`). No drag on that seam in
  036 (0277). Check `resize.c` still finds its three seams.
- `editor/src/interface.c`: the clicks read beside the other panels'.
- `editor/src/main.c`: one `voe_editor_assets_update` a frame with the session's folder and
  the frame clock.
- `editor/src/src.md`, `editor/editor.md`: lines for the new files; dock's says four panels.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0, and
`d=$(mktemp -d) && cp -r examples/capsule "$d/p" && rm -rf "$d/p/Build" && mkdir -p
"$d/p/Assets/tanks" && echo x > "$d/p/Assets/a.glb" && build/debug/editor/voe_editor "$d/p"
--capture "$d/c.png" && test -s "$d/c.png"` exits 0. The panel's look is the human's, card 19.
