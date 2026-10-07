# 08 — Delete asks and names the users
folder: editor
after: 07
decisions: 0168, 0377, 0378

## Change
The Delete key on the selected row asks first, names what uses it, and on Delete sends it to the
desktop's trash (0377 point 3).

- New `editor/src/assets_ask.h` / `.c` — the question as an anchored panel over the dock, drawn and read
  the way `editor/src/errors.h` / `.c` draw and read theirs: a line asking to delete the row's name; when
  something uses it, a line naming up to four users and "and N more" (`voe_editor_assets_users`,
  assets_walk.h, run once when the question opens, never per frame); Delete and Cancel buttons. Its
  state: open, the `Assets/`-relative path asked about, the users. Escape and a press outside it cancel.
- `editor/src/assets_panel.h` / `.c` — `voe_editor_assets_delete_begin(assets)` leaves a request to ask
  about the selected row; nothing without one.
- `editor/src/frame_commands.c` — `delete_asset` (shortcuts.h) calls it.
- `editor/src/interface.h` / `.c` — the question's state held beside the other anchored panels, opened
  from the panel's request, drawn over the dock, read after the frame; Delete calls
  `voe_editor_assets_trash` (assets_manage.h) and closes it, Cancel only closes it. Escape's order
  (frame_commands.h) closes it first while it is open.
- `editor/src/src.md` — the new files' entries; headers of the files touched updated.

## Done when
`test -f editor/src/assets_ask.h && grep -c voe_editor_assets_trash editor/src/interface.c` prints 1 or
more, and the folder's check passes.
Human: in the tank game select `tank_body.glb`, press Delete; the question names `main.scene` and the
prefabs; Cancel changes nothing. Delete an unused folder: it goes and is in the desktop's trash.
