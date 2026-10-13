# 04 — A material file holds cut-out, sides and emission
folder: assets
after: 03
decisions: 0168, 0399, 0420

## Change
0420 point 2's five keys in the `.material` text, read and written. Read `assets/include/assets/material.h`
and `assets/src/material.c` first.

- `assets/include/assets/material.h`: `voe_assets_material_file` gains `float cutout`, `bool
  double_sided`, `float emission[3]` (linear), `float emission_strength` and `char
  emissionmap[VOE_ASSETS_MATERIAL_PATH]`. The example file in the header shows them; the defaults
  sentence names theirs (0, false, 0 0 0, 1, ""); the paragraphs say what `cutout` means (a pixel
  whose colour map alpha is below it is not drawn; 0 none), that emission adds emission × strength ×
  the map, and that `emissionmap` is a colour picture.
- `assets/src/material.c`: `voe_assets_material_default`, the reader and the writer take the five keys.
  MALFORMED for `cutout` outside 0–1, a negative `emission_strength` or emission channel, and a
  `double_sided` that is not `true` or `false`, each naming the line as the other refusals do. The
  writer writes them after `ormmap`, `emissionmap` quoted as the other paths.
- `assets/tests/material.c`: the defaults read from an empty section include the five; a full file
  round-trips through write and read byte for byte; each new refusal is MALFORMED.
- `assets/tests/tests.md`: the `material.c` entry says so (at most 300 characters).

## Done when
The test `assets/material` passes after the folder's build.
