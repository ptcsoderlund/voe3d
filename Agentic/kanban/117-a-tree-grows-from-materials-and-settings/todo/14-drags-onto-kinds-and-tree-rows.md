# 14 — A species drops as a foliage kind, a material onto Bark or Leaf
folder: editor
after: 13
decisions: 0168, 0515, 0442

## Change
`editor/src/assets_drag.h` and `editor/src/assets_drag.c`:
- a `.tree` row released over the Foliage section's kind rows adds or swaps a kind, as a `.glb` does;
- a `.material` row released over the tree section's Bark or Leaf row sets that path as a tree step
  (card 13).

`editor/src/inspector_tree.h` and `editor/src/inspector_tree.c`: the section reports its Bark and Leaf
rows' rectangles, as the material section reports its map rows.

Update the headers, and the index entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

The human: drag a bark material onto Bark and a needle material onto Leaf; the tree wears them. Drag the
species into the Foliage kinds and paint it.
