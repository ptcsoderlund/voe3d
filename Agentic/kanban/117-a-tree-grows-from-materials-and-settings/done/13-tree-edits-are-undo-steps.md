# 13 — A tree edit is an undo step and writes its file
folder: editor
after: 12
decisions: 0168, 0515, 0509

## Change
New `editor/src/tree_steps.h` and `editor/src/tree_steps.c`. Read the header of
`editor/src/material_steps.h` and mirror it:
- a step holds a `.tree` path and its values before and after;
- the file is written at once and told to the disk watch;
- either way, the values are put back into the trees table (through `voe_editor_models_tree_changed`),
  into the file, and into the open copy.

`editor/src/frame_commands.c`: an open species whose shown copy differs from `tree_before` settles at rest
into a tree step, as an open material does.

Update the headers, and the index entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

The human: drag Droop, release, and press Ctrl+Z; the tree and the file go back. Ctrl+Y brings the change
back.
