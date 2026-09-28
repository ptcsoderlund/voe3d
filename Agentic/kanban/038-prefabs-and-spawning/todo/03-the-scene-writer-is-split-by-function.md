# 03 — The scene writer is split by function
folder: authoring
decisions: 0168

## Change
`authoring/src/scene_write.c` is 802 lines, and cards 04 and 05 change it. Split it by what the
code does, with no change in behaviour:

- `authoring/src/scene_write.c` keeps the walk: which entities and sections are written, in
  what order, the kept sections merged in, the refusals about entities and ids.
- `authoring/src/value_write.c` (new) and a private `authoring/src/value_write.h` (new): one
  field's value spelled by its kind and shape, nesting included, and the refusals about a value
  (ENUM, NaN or infinite, a control byte, a compiled-out description).
- Each file opens with a header saying what it owns and why the split falls there; each part
  under ~500 lines. If the walk needs a second split to get there, make it the same way.
- `authoring/src/src.md`: the entries for the files as they now are.

The public header `authoring/include/authoring/scene_write.h` does not change.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_authoring $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_authoring_[A-Za-z0-9_]+") && ctest --test-dir build/debug
-R "^authoring/"` exits 0, and `test $(wc -l < authoring/src/scene_write.c) -le 500 && test
$(wc -l < authoring/src/value_write.c) -le 500` exits 0.
