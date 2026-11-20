# 15 — The material panel clamps to the material constants
folder: editor
after: 11
decisions: 0168, 0511

## Change
- `editor/src/inspector_material.c`: the Strength slider's upper bound becomes
  `VOE_ASSETS_MATERIAL_EMISSION_STRENGTH_MAX`. The Repeat bounds become `VOE_ASSETS_MATERIAL_REPEAT_MIN`
  and `_MAX`, from `assets/material.h`. Nothing else changes.
- `editor/src/inspector_material.h`: the header comment names those constants where it gives the
  figures today.

## Done when
`grep -q VOE_ASSETS_MATERIAL_EMISSION_STRENGTH_MAX editor/src/inspector_material.c && grep -q VOE_ASSETS_MATERIAL_REPEAT_MAX editor/src/inspector_material.c`
exits 0.
