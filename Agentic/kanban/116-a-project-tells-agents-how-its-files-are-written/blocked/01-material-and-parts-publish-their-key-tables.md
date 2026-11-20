# 01 — Material and parts publish their key tables
folder: assets
after: none
decisions: 0168, 0498, 0511

## Change
- `assets/include/assets/sectioned.h`: add `voe_assets_sectioned_key`, one row describing one key a
  format reads. Fields: `section`; `key`, either a literal key or a pattern such as `layer<N>`; `kind`,
  a short phrase such as "number", "three numbers", "linear colour r g b, 0–1", "true or false",
  "quoted path" or "one of lit, unlit"; `default_text`, the default as spelled in the file, "" when the
  key is required; `least` and `greatest` as doubles, both 0 for no range; `unit`, "" for none;
  `refused`, true when the strict read refuses a value outside the range and false when it clamps or
  does not look; `says`, one line of meaning. The header comment makes these points: the row is
  documentation data, so layer one still interprets nothing; each format reaches its table through a
  function, never extern data (0284).
- `assets/include/assets/material.h`: add three constants for the ranges the Inspector clamps to
  today: `VOE_ASSETS_MATERIAL_EMISSION_STRENGTH_MAX` (1000), `VOE_ASSETS_MATERIAL_REPEAT_MIN` (0.01) and
  `VOE_ASSETS_MATERIAL_REPEAT_MAX` (1000). Declare
  `const voe_assets_sectioned_key *voe_assets_material_keys(uint32_t *count);`.
- `assets/src/material.c`: add a static table with one row per key in material.h's key list, giving
  the defaults material.h lists. Ranges come from those constants; roughness, metal and cutout run
  0–1. Set `refused` from how this file's strict read really behaves. Add the function.
- `assets/include/assets/parts.h` and `assets/src/parts.c`: do the same for parts as
  `voe_assets_parts_keys`, with rows for `material<N>`, `count` (1–6), `share<N>` (0–100, %) and
  `swap<N>` (0.1–10). Take ranges from named constants in parts.h, adding a constant wherever parts.c
  has a literal.
- `assets/assets.md`: add index entries for the row type and the two tables.
- `assets/tests/keys.c` is new; add its line to `assets/tests/tests.md`. It holds two tests,
  `material_keys_read_strictly` and `parts_keys_read`:
  - for every row with a default, a text that sets that key to its default under its section is
    accepted by `voe_assets_material_read_strict` (for parts, `voe_assets_parts_read`). A pattern's
    `<…>` parts take the first index the reader accepts;
  - for every material row marked `refused`, a value above `greatest` is refused;
  - every row has a non-empty `key`, `kind` and `says`.

## Done when
`ctest --test-dir build/debug -R '^assets/keys$' --no-tests=error` exits 0.

## Blocked
`voe_assets_sectioned_key` is already a type in `assets/sectioned.h`: the parsed `key=value` line
(`name`, `value`, `line`), used in `assets/src` and in `authoring/src/project.c`,
`authoring/src/scene_read.c` and `theme/src/theme.c`, so the row type cannot take that name. Unblock
by choosing either a different name for the row type (and amending 0511 and cards 02–04, which name
it) or renaming the existing line type, which needs cards for `authoring` and `theme`.
