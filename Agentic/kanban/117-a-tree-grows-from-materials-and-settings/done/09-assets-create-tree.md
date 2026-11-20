# 09 — Assets: Create → Tree
folder: editor
after: 08
decisions: 0168, 0515

## Change
- `editor/src/assets_menu.h` and `editor/src/assets_menu.c`: the Create submenu gains Tree, after
  Material.
- `editor/src/assets_manage.h` and `editor/src/assets_manage.c`:
  - New `voe_editor_assets_tree_make`, shaped as the default material's make: it writes
    `voe_assets_tree_write(voe_assets_tree_default())` at the next free `Tree.tree` name in the shown
    folder, refused with a notice as the others are.
  - Where a made material has the tables read again, the trees table is read with them (card 08).
- Update the headers' Create points and the `editor/src/src.md` entries.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

The human: right-click empty Assets space, then Create, then Tree; a `Tree.tree` row appears, and its
text has every key at its default.
