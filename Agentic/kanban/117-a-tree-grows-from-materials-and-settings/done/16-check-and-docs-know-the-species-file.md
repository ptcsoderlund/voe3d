# 16 — Check and Docs know the species file
folder: editor
after: 15
decisions: 0168, 0511, 0512, 0515

## Change
- `editor/src/check.c`: a `.tree` file from the walk's list is handed to `voe_assets_tree_read_strict`,
  its first error a line as for the other kinds.
- `editor/src/docs.c`: `voe_assets_tree_keys` is written into the files reference with a short
  description of the species file, as the material's table is.
- Update the headers of `check.h` and `docs.h` where they list kinds.

## Done when
`cmake --build --preset debug --target voe_editor` exits 0.

A project copied to a scratch folder with one valid and one broken `.tree` under `Assets/`:
- `voe_editor --check <it>` exits 1 and names only the broken file;
- with the broken one removed, it exits 0;
- its `Docs/voe3d-files.md` lists the tree keys.
