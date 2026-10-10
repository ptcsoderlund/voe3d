# 20 — The material file's values are named for the file
folder: assets
after: none
decisions: 0168, 0399

## Change
`assets` has two types named `voe_assets_material`: card 01's file values in
`assets/include/assets/material.h` and the glTF material in `assets/include/assets/model.h`. No
translation unit can include both, and `3d` must (the model store takes models and material files).
Rename the file's struct only, inside `assets`; its functions and the glTF type keep their names.

- `assets/include/assets/material.h` — the struct becomes `voe_assets_material_file`; every
  signature naming it follows. Header point: the name says it is the file's values, apart from the
  glTF material in `model.h`.
- `assets/src/material.c` — the same rename.
- `assets/tests/material.c` — the same rename; it also includes `assets/model.h`, so a second type of
  either name fails to build here.
- `assets/assets.md`, `assets/src/src.md`, `assets/tests/tests.md` — any line naming the type follows.

`3d` names the old type in `3d/include/3d/models.h`, `3d/src/models_material.c` and
`3d/tests/models_material.c`; card 21 renames those. Do not touch them.

## Done when
`ctest --test-dir build/debug -R '^assets/(material|model)$'` passes, and
`grep -rn 'voe_assets_material\b' assets/include/assets/material.h assets/src/material.c` prints nothing.
