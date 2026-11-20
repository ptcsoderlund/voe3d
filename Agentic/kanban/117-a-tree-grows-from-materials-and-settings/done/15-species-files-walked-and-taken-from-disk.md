# 15 — Species files are walked, followed and taken in from disk
folder: editor
after: 14
decisions: 0168, 0515, 0509, 0378

## Change
`editor/src/assets_walk.h` and `editor/src/assets_walk.c`: `.tree` texts are walked beside
`.material`:
- their bark and leaf paths count in "who names a path";
- a rename's follow rewrites them;
- the sorted file list that the watch and check see includes `.tree` files.

`editor/src/disk_take.c`, `editor/src/file_steps.h` and `editor/src/file_steps.c`: a `.tree` changed on
disk is taken in as a material is:
- it is read strictly; a refusal goes into the notice;
- otherwise it becomes a file step of the bytes before and after, and `voe_editor_models_tree_changed` is
  called with the new values;
- an open species' copy is refreshed.

Wherever a rename or delete reads the materials table again, the trees table is read with it.

Update the headers, and the index entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

The human: with a species painted, edit its `height=` in a text editor and save; the painted trees change.
Then rename the bark material in Assets; the `.tree` file names the new path.
