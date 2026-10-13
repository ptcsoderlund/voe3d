# 45 — 3d's tests copy with snprintf
folder: 3d/tests
after: 42, 43
decisions: 0168, 0413

## Change
Bug 05: `strcpy` and `strncpy` are deprecated by the Windows headers. Each copy into a fixed `char` field
becomes `snprintf(field, sizeof field, "%s", text)` (0413 point 3); add `<stdio.h>` where missing.

- `3d/tests/draw_material.c` lines 111 and 131 (`strncpy` with `sizeof - 1`; the bound is now snprintf's).
- `3d/tests/emitter_component.c` line 53.
- `3d/tests/models_material.c` lines 49–51.
- `3d/tests/model_component.c` lines 56, 134, 154.

No header or index change.

## Done when
- `grep -nE '\bstrn?cpy[[:space:]]*\(' 3d/tests/*.c` prints nothing.
- Tests `3d/draw_material`, `3d/emitter_component`, `3d/models_material` and `3d/model_component` pass.
