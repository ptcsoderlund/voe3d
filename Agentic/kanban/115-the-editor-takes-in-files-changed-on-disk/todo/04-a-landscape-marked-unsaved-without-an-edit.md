# 04 — Mark a landscape unsaved without an edit
folder: 3d
after: 01
decisions: 0168, 0509

## Change
- `3d/include/3d/models.h`, `3d/src/models_landscape.c`: add
  `void voe_3d_models_landscape_changed(voe_3d_models *models, const char *path)`.
  - It marks `path`'s entry edited and leaves its grid untouched, so the next save writes it.
  - It does nothing for a path that is not a loaded landscape.
  - Place it beside `voe_3d_models_landscape_saved`.
  - Its comment says why it exists: an undo step that puts a whole landscape back leaves the store
    unlike its file (0509 point 2).
- `3d/tests/models_landscape.c`: add `a_changed_landscape_is_edited`. A landscape that was saved
  and then changed is edited; an unknown path changes nothing.
- `3d/tests/tests.md`: the `models_landscape.c` entry.

## Done when
`ctest --test-dir build/debug -R '^3d/models_landscape$'` exits 0, and its output includes
`a_changed_landscape_is_edited`.
