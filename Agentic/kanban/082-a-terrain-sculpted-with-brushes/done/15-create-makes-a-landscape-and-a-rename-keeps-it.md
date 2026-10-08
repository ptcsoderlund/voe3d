# 15 — Create makes a landscape, and a rename keeps it
folder: editor
after: 14
decisions: 0168, 0377, 0378, 0379

## Change
Create → Landscape names a flat landscape in place; a `.landscape` row drags into a view like a model;
a rename or move of one keeps its unsaved heights (0377 point 2, 0379 points 1, 2, 6).

- `editor/src/assets_menu.h` / `assets_menu.c` — Create's submenu gains Landscape after Folder
  (`VOE_EDITOR_ASSETS_MENU_KINDS` 2, item `VOE_EDITOR_ASSETS_MENU_LANDSCAPE`); the header's kinds
  sentence names it.
- `editor/src/assets_panel.h` / `assets_panel.c` — naming kind `VOE_EDITOR_ASSETS_NAMING_LANDSCAPE`: a
  pending row first among the files, named in place as a folder's is (0378 point 4); its request carries
  `folder` and `name`. A `.landscape` row, any case, is a model row: held as one, so assets_drag.h
  places it in a view and swaps it over the Inspector as a `.glb`. Header: the kind and the row.
- `editor/src/assets_manage.h` / `assets_manage.c` — new `[[nodiscard]] bool
  voe_editor_assets_landscape_make(voe_editor_session *, voe_base_arena *scratch, const char *folder,
  const char *name)`: `.landscape` appended unless the name already ends so; refused with a notice
  as a folder's name is and when the file is taken; else `voe_assets_landscape_flat` at
  `VOE_ASSETS_LANDSCAPE_SIZE_DEFAULT` and `_CELLS`, written with `voe_assets_landscape_write` through
  `platform`'s file write; the panel lists again. `voe_editor_assets_move` gains `voe_editor_models *`
  and, after a move that succeeded, calls a new `voe_editor_models_rename` (models.h/.c, a thin call to
  `voe_3d_models_rename`) with both paths under `Assets/`. Header points for both.
- `editor/src/interface.c` — the menu's Landscape begins that naming; a LANDSCAPE request calls the
  make; both move calls (here and `assets_drag.c`) pass the models.
- `editor/src/src.md` — the assets_menu, assets_panel, assets_manage and interface entries updated.

## Done when
`grep -c voe_editor_assets_landscape_make editor/src/interface.c` prints 1 or more, and the folder's
check passes.
Human: How to test step 9 in feature.md, and Create → Landscape named `Hill` makes `Hill.landscape`,
which dragged into a view shows a flat ground of 256 m.
