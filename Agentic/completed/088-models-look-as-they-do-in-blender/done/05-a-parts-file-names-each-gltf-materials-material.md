# 05 — A parts file names each glTF material's material
folder: assets
after: 04
decisions: 0168, 0404, 0420

## Change
0420 point 1's `.parts` text, read and written. Read `assets/include/assets/material.h` and
`assets/src/material.c` as the pattern: a struct, a default, a reader over `assets/sectioned.h` and a
writer that cannot fail.

- `assets/include/assets/parts.h` (new, header comment): what the file is (0404: which `.material`
  each glTF material of a `.glb` wears, shared by every placed copy), its name beside the `.glb`, the
  example text, why keys are indices (0420 reasoning), that nothing here opens a file.
  - `VOE_ASSETS_PARTS` (32).
  - `typedef struct { uint32_t count; char materials[VOE_ASSETS_PARTS][VOE_ASSETS_MATERIAL_PATH]; }
    voe_assets_parts`: entry N for glTF material N, "" for none; `count` one past the highest key read.
  - `[[nodiscard]] bool voe_assets_parts_read(const char *text, size_t size, voe_assets_parts *out,
    voe_base_error *error)`: MALFORMED for text the sectioned reader refuses, no `[Parts]`, a key not
    `material<N>` with N below VOE_ASSETS_PARTS, or a path of VOE_ASSETS_MATERIAL_PATH bytes or more;
    a stderr line names what; `out` untouched on failure.
  - `voe_assets_material_text voe_assets_parts_write(const voe_assets_parts *parts,
    voe_base_arena *arena)`: the text the reader takes back, one key per entry below `count`, paths
    quoted; asserts each path is NUL-terminated in its room and holds no line break.
- `assets/src/parts.c` (new, header comment): the reader and writer.
- `assets/tests/parts.c` (new): three entries with the middle one empty round-trip; an empty section
  reads count 0; `material32`, `colour=…` and an over-long path are MALFORMED.
- `assets/assets.md`: an entry for `include/assets/parts.h`; `assets/src/src.md` and
  `assets/tests/tests.md`: one each for the new files (each at most 300 characters).

## Done when
The test `assets/parts` passes after the folder's build.
