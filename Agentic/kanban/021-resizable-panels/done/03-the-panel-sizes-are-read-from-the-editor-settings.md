# 03 — The panel sizes are read from the editor's settings
folder: editor
decisions: 0168, 0220, 0225, 0226

## Change
The editor's settings file exists, the top bar can be taller than its content, and the editor opens with
the sizes the file holds.

- `editor/src/settings.h`, `editor/src/settings.c` — new, modelled on `last_project.h`/`.c` (read that
  pair's header and body for the folder-making and write pattern).
  - `voe_editor_settings { float scene_wide; float inspector_wide; float topbar_high; }`.
  - `void voe_editor_settings_read(voe_editor_settings *settings)`: fields keep what the caller put in
    unless the file has a parsing, in-range line for them (`scene_wide`/`inspector_wide` above nought,
    `topbar_high` nought or more, all finite and at most 1000 mm); missing folder or file, or a bad line,
    reports nothing.
  - `[[nodiscard]] bool voe_editor_settings_write(const voe_editor_settings *settings)`: re-reads the file,
    keeps every line with another key in its order, writes the three keys as `%.3f` lines, making the
    two folders as needed; false on failure, reported at the site.
  - Header points: one file for the person, not the project (0220); the line shape and why lines with
    other keys are kept (0226); a first start is not a failure.
- `editor/src/topbar.h`, `topbar.c` — `voe_editor_topbar` gains `float wanted` (the person's height,
  nought = fit). `voe_editor_topbar_least(bar)`: today's measured-or-first-frame height.
  `voe_editor_topbar_high(bar, float surface_high)` becomes the larger of least and `wanted`, at most
  `surface_high - VOE_EDITOR_DOCK_VIEW_ROOM`, never below nought. Header: 0225 amended by 0226.
- `editor/src/interface.c` — every `voe_editor_topbar_high` call passes the root's own `size.y`.
- `editor/src/main.c` — after the root's tree is made with `voe_editor_dock_default`, fill a
  `voe_editor_settings` from it (`voe_editor_dock_panel_length` of SCENE and INSPECTOR, `topbar_high`
  nought), read the file over it, and set it back with `voe_editor_dock_panel_length_set` and the bar's
  `wanted`. Also in a capture (it reads themes from the same place). The header gains one line saying the
  panel sizes come from `settings.h`.
- `editor/src/src.md` — `settings.h` and `settings.c` entries; `topbar.h`'s says the bar is at least as
  tall as its content and as tall as the person made it.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `editor` exits 0. With `XDG_CONFIG_HOME` at a scratch
folder whose `voe3d/editor_settings` holds the lines `scene_wide 90.000`, `topbar_high 30.000` and
`colour blue`, `voe_editor <scratch>/p --capture <scratch>/a.png --size 1280x720` shows the Scene list
wider than the Inspector and a bar about three times its usual height; with `scene_wide -4.000` in its
place the Scene list is the default width; with no file the capture looks as card 02's did.
