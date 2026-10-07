# 16 — The Landscape panel sets its size
folder: editor
after: 15
decisions: 0168, 0379

## Change
Clicking a `.landscape` row, or making one with Create, opens a Landscape panel whose Size box writes the
file at once (0379 point 6). The feature's last card.

- New `editor/src/landscape_panel.h` / `landscape_panel.c`, shaped as `editor/src/project_panel.h`'s
  (read its header): an anchored panel over the dock below the bar, a title row "Landscape" with the
  file's name and an ×, and one row "Size (m)" as a number box rounded and clamped to
  `VOE_ASSETS_LANDSCAPE_SIZE_MIN..MAX`. `_show(panel, path)` keeps the project-relative path; `_hide`;
  `_draw(ui, panel, size_shown, top, surface)`; `_clicks_read` answers closed or a new size. Header: a
  change is a setting written at once, never unsaved or undone; it carries out nothing itself.
- `editor/src/assets_panel.h` / `assets_panel.c` — a `.landscape` row pressed and released on the row
  fires as a prefab row does, into a new `landscape_opened` (`Assets/…`, "" for none) for the caller to
  read and clear; a dragged row fires nothing.
- `editor/src/models.h` / `models.c` — new `voe_editor_models_landscape_size(models, const char *folder,
  const char *path, float size, gpu, scratch, voe_editor_notice *why)` → bool: the heights from the
  store's entry when loaded, else read from the file; the size set, the file written, and a loaded entry
  loaded again from those bytes (`voe_3d_models_load`), its edits now saved. A size shown is the entry's,
  else the file's.
- `editor/src/interface.c` — opens the panel on `landscape_opened` and after
  `voe_editor_assets_landscape_make` succeeds; draws and reads it as it does the Project panel; a new size
  goes to `voe_editor_models_landscape_size`, a refusal into the session notice. Escape closes it
  (`frame_commands.c`), as it closes the Project panel. Where the Project panel is held (main.c or the
  interface's state) the Landscape panel is held too.
- `editor/src/src.md` — entries for the new files; the assets_panel, models, interface and
  frame_commands entries updated.

## Done when
`grep -c voe_editor_models_landscape_size editor/src/interface.c` prints 1 or more, and the folder's
check passes.
Human: every step of How to test in feature.md.
