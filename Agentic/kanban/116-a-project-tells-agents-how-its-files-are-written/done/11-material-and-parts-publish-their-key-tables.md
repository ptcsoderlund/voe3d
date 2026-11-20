# 11 — Material and parts publish their key tables
folder: assets
after: none
decisions: 0168, 0498, 0511, 0512

## Change
- `assets/include/assets/sectioned.h`: add `voe_assets_documented_key` (0512), one row describing one
  key a format reads. The existing `voe_assets_sectioned_key`, the parsed `key=value` line, stays as it
  is. Fields: `section`; `key`, a literal key or a pattern such as `layer<N>`; `kind`, a short phrase
  such as "number", "three numbers", "linear colour r g b, 0–1", "true or false", "quoted path" or
  "one of lit, unlit"; `default_text`, the default as spelled in the file, "" when required; `least`
  and `greatest` as doubles, both 0 for no range; `unit`, "" for none; `refused`, true when the strict
  read refuses a value outside the range, false when it clamps or does not look; `says`, one line of
  meaning. The header comment makes these points: the row is documentation data, so layer one still
  interprets nothing; it is not the parsed line type beside it; each format reaches its table through
  a function, never extern data (0284).
- `assets/include/assets/material.h`: add three constants for the ranges the Inspector clamps to
  today: `VOE_ASSETS_MATERIAL_EMISSION_STRENGTH_MAX` (1000), `VOE_ASSETS_MATERIAL_REPEAT_MIN` (0.01)
  and `VOE_ASSETS_MATERIAL_REPEAT_MAX` (1000). Declare
  `const voe_assets_documented_key *voe_assets_material_keys(uint32_t *count);`.
- `assets/src/material.c`: a static table, one row per key in material.h's key list, with the
  defaults material.h lists. Ranges come from those constants; roughness, metal and cutout run 0–1.
  Set `refused` from how this file's strict read really behaves. Add the function.
- `assets/include/assets/parts.h` and `assets/src/parts.c`: the same for parts as
  `voe_assets_parts_keys`, with rows for `material<N>`, `count` (1–`VOE_ASSETS_LEVELS`), `share<N>`
  (0–100, %) and `swap<N>` (0.1–10). Take ranges from named constants in parts.h, adding one wherever
  parts.c has a literal.
- `assets/assets.md`: index entries for the row type and the two tables.
- `assets/tests/keys.c` is new; add its line to `assets/tests/tests.md`. Two tests,
  `material_keys_read_strictly` and `parts_keys_read`:
  - for every row with a default, a text setting that key to its default under its section is
    accepted by `voe_assets_material_read_strict` (for parts, `voe_assets_parts_read`). A pattern's
    `<…>` parts take the first index the reader accepts;
  - for every material row marked `refused`, a value above `greatest` is refused;
  - every row has a non-empty `key`, `kind` and `says`.

## Meaning
- Holds: one row type, reached through one function per format; ranges are the owner's named
  constants, which the panels clamp to (card 15). The table must not promise a key the reader
  refuses. (0511, 0498)
- Believed: the strict material read gives roughness and metal no range and refuses only where the
  old reader refused, so their rows likely carry `refused` false. Confirm in material.c.
- Believed: `.parts` has no strict read; an old `strength` key in `[Levels]` is still accepted (0437).
- Holds (scout): parts.c holds `count` to 1–6 through `VOE_ASSETS_LEVELS` (parts.h).

## Done when
`ctest --test-dir build/debug -R '^assets/keys$' --no-tests=error` exits 0.
