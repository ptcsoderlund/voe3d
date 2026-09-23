# 02 — The views' share is read from the editor's settings
folder: editor
decisions: 0168, 0220, 0226, 0229

## Change
The settings file carries the views' share and the editor opens with it.

- `editor/src/settings.h`, `editor/src/settings.c`
  - `voe_editor_settings` gains `double view_share`: the top view's part of the middle, nought to one
    exclusive.
  - `voe_editor_settings_read`: a `view_share` line is taken when finite and strictly between nought and
    one; otherwise the field keeps what the caller put in, reporting nothing, as the other keys do.
  - `voe_editor_settings_write` writes it as a fourth `%.3f` line after the three sizes; other keys'
    lines are still kept in their order.
  - Header: the keys paragraph names `view_share` and that it is a share, not millimetres (0229).
- `editor/src/main.c` — where the panel sizes are filled from the default tree and set back after the
  read, `view_share` is filled with `voe_editor_dock_view_share` and set back with
  `voe_editor_dock_view_share_set`. The header's line on the panels' sizes also names the views' share.
- `editor/src/resize.c` — `voe_editor_resize_remember` fills `view_share` from the tree with
  `voe_editor_dock_view_share`, so a write after a side panel's drag keeps the share. Its comment in
  `resize.h` says it writes the share too.
- `editor/src/src.md` — the `settings.h` entry names the views' share.

Read the headers of the files named here and the functions you change; no other file.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. With `XDG_CONFIG_HOME` at a scratch
folder whose `voe3d/editor_settings` holds `view_share 0.700` and `colour blue`, `voe_editor <scratch>/p
--capture <scratch>/a.png --size 1280x720` shows the top view about 70% of the middle's height; at
`--size 1280x1440` still about 70%; with `view_share 1.500` or `view_share 0.990` in its place the views
are half and half or the bottom view keeps its minimum (about 40 mm) respectively; with no file, half and
half.
