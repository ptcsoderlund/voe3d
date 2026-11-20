# 07 — Split the editor's model store file
folder: editor
after: none
decisions: 0168

## Change
`editor/src/models.c` is 856 lines. Split it by function, with no change in behaviour:
- `models.c` keeps the store's life, update, reload, frame, save, revert and the materials and map queue.
- New `editor/src/models_edit.c` takes the landscape and foliage edits: brush, put, put_paint, layer_set,
  scatter, erase, put_copies, kind_set, look_set, grow_set, rename.
- If both need the struct, it goes in a private `editor/src/models_store.h`.

`editor/src/models.h` is unchanged. Each new file gets a header comment saying what it holds. Update the
entries in `editor/src/src.md`.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0, and `wc -l editor/src/models.c
editor/src/models_edit.c` shows each under 600.
