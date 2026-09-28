# 04 — A file has a stamp
folder: platform
decisions: 0168, 0277

## Change
Additive (0277 point 5): what tells a changed file from the same one.

- `platform/include/platform/file.h`:
  `[[nodiscard]] bool voe_platform_file_stamp(const char *path, uint64_t *out);` — false when
  the path is not a regular file (then `out` is untouched); otherwise a number mixing the
  modification time at the finest resolution the system gives and the size. Only equality means
  anything; a caller never orders two stamps. Add a paragraph to the header saying so and why a
  size is in it (a rewrite inside one clock tick).
- The Linux and Windows implementations beside `voe_platform_file_exists` (read
  `platform/src/src.md` for which files): `stat` (`st_mtim` and `st_size`) on Linux,
  `GetFileAttributesExW` (`ftLastWriteTime` and the size) on Windows, with the path widened the
  way the other Windows calls there widen it.
- `platform/tests/file.c`: cases — a missing path is false; a written file has a stamp, the same
  when asked again; written again with a different length it has a different one; a folder is
  false. Update its line in `platform/tests/tests.md`.
- `platform/platform.md`: the `file.h` entry mentions the stamp.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_platform $(ninja -C build/debug
-t targets all | grep -oE "^voe_test_platform_[A-Za-z0-9_]+") && ctest --test-dir build/debug -R
"^platform/"` exits 0.
