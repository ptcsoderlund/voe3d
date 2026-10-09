# 36 — Create makes a material
folder: editor
after: 35
decisions: 0168, 0378, 0399

## Change
0399 points 1, 7 and 9: Create → Material, and Assets commands keep materials whole.

- `editor/src/assets_menu.h` / `.c` — Create's submenu gains Material after Landscape
  (`VOE_EDITOR_ASSETS_MENU_MATERIAL`).
- `editor/src/assets_panel.h` / `.c` — `voe_editor_assets_material_begin`, a pending "New material"
  row first among the files, and a `VOE_EDITOR_ASSETS_NAMING_MATERIAL` request, as the landscape's.
- `editor/src/assets_manage.h` / `.c` — `[[nodiscard]] bool voe_editor_assets_material_make(...)`
  with the landscape make's parameters less size: `.material` appended, refused as a landscape's
  name is, the file `voe_assets_material_write(voe_assets_material_default())`'s text. After every
  successful make, move, duplicate and trash, `voe_editor_models_materials_read` (card 35).
- `editor/src/assets_walk.c` — `.material` joins the one list of followed extensions, so a rename of
  a map follows into material files and Delete names a material using a picture.
- `editor/src/interface_assets.c` — the menu's Material row begins the naming; the request
  goes to `voe_editor_assets_material_make`, the panel listed after.

Update the headers' kind lists (`assets_menu.h`, `assets_panel.h`, `assets_walk.h`,
`assets_manage.h`) and `editor/src/src.md`'s lines for them.

## Done when
`grep -n '\.material' editor/src/assets_walk.c editor/src/assets_manage.c` finds both, and the editor
builds. Human: right-click the empty part of Assets, Create → Material, type Dirt, Enter:
`Dirt.material` appears holding the defaults.
