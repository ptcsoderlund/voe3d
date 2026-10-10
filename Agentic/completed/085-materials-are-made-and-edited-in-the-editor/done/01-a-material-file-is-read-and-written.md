# 01 — A material file is read and written
folder: assets
after: none
decisions: 0168, 0399

## Change
New `assets/include/assets/material.h` and `assets/src/material.c`, the `.material` file of 0399
point 1 over the sectioned reader (`assets/include/assets/sectioned.h`), written the way
`assets/include/assets/landscape.h` and `assets/src/landscape.c` are (read them for the pattern).

- `VOE_ASSETS_MATERIAL_PATH` 128, the room of one map path with its NUL.
- `voe_assets_material_shader` enum: `VOE_ASSETS_MATERIAL_LIT = 0`, `VOE_ASSETS_MATERIAL_UNLIT`.
- `voe_assets_material` struct: `shader`, `float colour[3]` (linear), `roughness`, `metal`,
  `repeat`, and `char colour_map[128]`, `normal_map[128]`, `roughness_map[128]`.
- `voe_assets_material voe_assets_material_default(void)` — 0399's defaults.
- `[[nodiscard]] bool voe_assets_material_read(const char *text, size_t size, voe_assets_material
  *out, voe_base_error *error)` — one `[Material]` section; a missing key is its default; MALFORMED
  for text the sectioned reader refuses, a number that does not parse, an unknown shader word or a
  path of 128 bytes or more; a stderr line names what; `out` untouched on failure.
- `voe_assets_material_text voe_assets_material_write(const voe_assets_material *material,
  voe_base_arena *arena)` — NUL-terminated text the read takes back exactly (floats printed so they
  round-trip, e.g. `%.9g`); cannot fail. A struct of text and size as the landscape's.

Header points: what the file holds and its defaults, why a missing key is its default (old files
open), why paths and not ids, that nothing here opens a file. Add both files to
`assets/assets.md` and `assets/src/src.md`.

New test `assets/tests/material.c`: defaults read from an empty `[Material]` section; a full file
round-trips through write and read field for field; an unknown shader and a bad number refused
MALFORMED. Add it to `assets/tests/tests.md`.

## Done when
`ctest --test-dir build/debug -R '^assets/material$'` passes.
