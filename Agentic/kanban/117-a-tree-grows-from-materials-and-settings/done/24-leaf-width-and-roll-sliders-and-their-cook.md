# 24 — Leaf width and Leaf roll sliders, and the cook writes both
folder: editor
after: 22, 23
decisions: 0168, 0516, 0517, 0515

## Change
`editor/src/inspector_tree.c` and `editor/src/inspector_tree.h`: two sliders after Leaf density:
- Leaf width, its range from `VOE_ASSETS_TREE_LEAF_WIDTH_MIN`/`MAX`;
- Leaf roll, from `VOE_ASSETS_TREE_LEAF_ROLL_MIN`/`MAX`.

Each joins the slider table, and any node or figure arrays sized by the slider count. A drag regrows the
tree as the other sliders do. Undo and the file write need nothing: `frame_commands.c` compares the whole
values.

`editor/src/game_tree_materials.c`: the cooked trees table writes `.leaf_width` and `.leaf_roll` as it
writes `.leaf_density`, in hex floats. Without them both would be 0 in Play and Ship.

Update the header points that list the sliders and the cooked fields, and the entries in
`editor/src/src.md` if they list them.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and
`grep -c 'leaf_roll\|leaf_width' editor/src/game_tree_materials.c` prints 2 or more.

A project copied to a scratch folder with a `.tree` under `Assets/` holding `leaf_roll=1` and
`leaf_width=2`:
- `voe_editor --check <it>` exits 0;
- its `Docs/voe3d-files.md` names `leaf_roll` and `leaf_width`;
- with `leaf_roll=2`, `voe_editor --check <it>` exits 1 and names the file.

The human: How to test steps 13 and 14 of `feature.md`.
