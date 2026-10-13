# 40 — Import textures dresses a glb's materials
folder: editor
after: 39
read: feature.md
decisions: 0168, 0403, 0404, 0420

## Change
0403's and 0404's right-click Import textures, 0420 point 3's second half; the feature's last card.
Read the headers of `editor/src/assets_menu.h` and `editor/src/model_import.h` (card 36) first.

- `editor/src/model_import.h`, `editor/src/model_import.c`: `[[nodiscard]] bool
  voe_editor_model_import_textures(const char *project_folder, const char *glb, voe_base_arena
  *scratch, voe_editor_notice *why)`: reads the `.glb` and its `<stem>.parts`; writes every picture as
  `<stem>-<image name>.png` (`voe_assets_png_encode`, `assets/image.h`) and, per material with one,
  `<stem>-<material name>-orm.png` from `voe_assets_model_orm` (`assets/model_material.h`); then for
  each glTF material its `.parts` names, reads that `.material` (or takes the glTF values when it is
  gone), sets `colormap`, `normalmap`, `ormmap` and `emissionmap` to the written PNGs' `Assets/…` paths
  (a slot the glTF material leaves empty keeps its path), and writes it. No `.parts` is said in `why` ("Import
  the model first"). PNGs are written whole and atomically, over a file of that name.
- `editor/src/assets_menu.h`, `editor/src/assets_menu.c`: over a `.glb` row the menu also holds Import
  textures, after Delete; `VOE_EDITOR_ASSETS_MENU_IMPORT_TEXTURES`; opening is told whether the row is a
  `.glb`. Header: the rows.
- `editor/src/assets_panel.c` (the menu's opening) and `editor/src/interface_assets.c` (a fired row
  carried out): Import textures calls it on the selected row, then the tables are read again and the
  open material refreshed as after an import (card 36), and the panel lists again.
- `editor/src/src.md`: entries follow (each at most 300 characters).

## Done when
`voe_editor` builds in the folder's check with no warning. The human runs the feature's How to test,
steps 1–8, in the editor, and every step holds.
