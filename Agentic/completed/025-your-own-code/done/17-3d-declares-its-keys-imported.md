# 17 — 3d declares its component keys and shape names imported
folder: 3d
decisions: 0168, 0245

## Change
Point 2 of 0245: every `extern` object in `3d`'s public headers.

- `3d/include/3d/panel_component.h`, `material_component.h`, `mesh_component.h` — include
  `base/imported.h`; the key's `extern` gains `VOE_BASE_IMPORTED` after `extern`.
- `3d/include/3d/shape_component.h` — the same for `voe_3d_shape_key` and
  `voe_3d_shape_kind_names`.
- No prose change unless a header says how its key is declared.

## Done when
1. `checks.sh --folder 3d` prints `FINDINGS: 0`.
2. `grep -rn '^extern' */include --include=*.h | grep -v VOE_BASE_IMPORTED` prints nothing.
