# 17 — A prefab row clicked opens it, and the bar's Back returns to the level
folder: editor
decisions: 0168, 0283, 0277

## Change
Needs card 16. 0283 point 8, what a person sees and presses.

- `editor/src/assets_panel.h`, `editor/src/assets_panel.c`: `voe_editor_assets_clicks_read`
  also reports a prefab row that fired (pressed and released on the row, which a drag to a view
  never is) as its project-relative path, through a new out-parameter or a field the caller reads
  and clears; the header says which.
- `editor/src/topbar.h`, `editor/src/topbar.c`: while `project->prefab` is set, the bar names the
  prefab's file in place of the project's name, still marked when unsaved, and shows a Back
  button first on the row; `voe_editor_topbar_clicks_read` answers `VOE_EDITOR_COMMAND_BACK` for
  it. Header: the Back button.
- `editor/src/interface.c`: a fired prefab row calls `voe_editor_session_prefab_open`, beside
  where Import shows the browser; Back reaches `voe_editor_session_do` as the bar's other
  commands do. Header: both.
- `editor/src/src.md`: the changed entries.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_editor` exits 0. The human's,
after card 14's check: click `tank_body.prefab` in the Assets panel: the views show the tank
alone and the bar names it with Back; give the turret another model (drop a `.glb` on the
Inspector), Save, Back: the four tanks wear it, each where it stood; Play is refused while the
prefab is open; Back with unsaved prefab edits is refused once; Ctrl+Z in the level undoes the
last placement from before the visit.
