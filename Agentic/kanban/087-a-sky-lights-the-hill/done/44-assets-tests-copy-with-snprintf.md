# 44 — Assets' material test copies with snprintf
folder: assets/tests
after: 43
decisions: 0168, 0413

## Change
Bug 05: `assets/tests/material.c` lines 74–75 copy into `full.colormap` and `full.normalmap` with `strcpy`,
deprecated by the Windows headers. Each becomes `snprintf(field, sizeof field, "%s", text)` (0413 point 3).
Add `<stdio.h>` if missing. No header or index change.

## Done when
- `grep -nE '\bstrn?cpy[[:space:]]*\(' assets/tests/material.c` prints nothing.
- Test `assets/material` passes.
