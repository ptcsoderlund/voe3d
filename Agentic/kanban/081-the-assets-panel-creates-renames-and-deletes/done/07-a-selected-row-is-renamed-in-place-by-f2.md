# 07 — A selected row is renamed in place by F2
folder: editor
after: 01, 06
decisions: 0168, 0377, 0378

## Change
The panel gains a selected row, the keyboard, and a name field in a row (0378 points 4 and 6).

- `editor/src/assets_panel.h` / `.c`:
  - State: the selected row's name (in the panel's arena, kept across a listing while the name is
    still there, cleared on entering a folder or Up); `keyboard`, whether F2 and Delete are the panel's;
    the naming in progress — none, a row's rename, or a new folder — with its field node and, for a
    new folder, its pending row.
  - Draw: the selected row drawn as a selected choice; a row being renamed drawn as a `voe_ui_field`
    (ui/widgets.h) holding its name, focused with `voe_ui_field_focus` on the frame naming begins; a new
    folder's pending row is a field holding "New folder", drawn first among the folders. The rows and the
    empty space under them are recorded as nodes whose rectangles a read can test.
  - Read: a row pressed (held, as `ui` reports) becomes the selected row and takes `keyboard`; a primary
    press anywhere outside the panel gives `keyboard` back. A committed field leaves a request the caller
    reads and clears — rename `from` to `to`, or make a folder named so in `shown`; a cancelled one
    leaves none. Either way the naming ends.
  - `voe_editor_assets_rename_begin(assets)` (on the selected row, nothing without one) and
    `voe_editor_assets_folder_begin(assets)`.
- `editor/src/shortcuts.h` / `.c` — a `rename` flag for F2 alone; a guard `assets_keyboard`: while it
  holds, Delete sets a new `delete_asset` flag instead of `delete_entity`, and Ctrl+D does nothing.
- `editor/src/frame_commands.c` (and `.h` if the guard is filled there) — the guard filled from
  `scene.assets.keyboard`; `rename` calls `voe_editor_assets_rename_begin`.
- `editor/src/interface.c` — after the panel's read, a rename request calls `voe_editor_assets_move`
  and a folder request `voe_editor_assets_folder_make` (assets_manage.h) with the frame's scratch.
- Headers of every file touched say what changed; `editor/src/src.md` entries updated.

## Done when
`grep -c 'voe_editor_assets_rename_begin\|voe_editor_assets_folder_begin' editor/src/assets_panel.h`
prints 2 or more, and the folder's check passes.
Human: open the tank game, select `tank_head.glb`, press F2, type a new name, Enter; the level still
shows it; Escape on a second F2 changes nothing; a taken name is refused with a notice.
