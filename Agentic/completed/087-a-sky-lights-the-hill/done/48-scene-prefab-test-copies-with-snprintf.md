# 48 — Scene's prefab test copies with snprintf
folder: scene/tests
after: none
decisions: 0168, 0413

## Change
Bug 05: `scene/tests/prefab.c` line 54 copies into `row.path` with `strcpy`, deprecated by the Windows
headers. It becomes `snprintf(row.path, sizeof row.path, "%s", ...)` (0413 point 3); add `<stdio.h>` if
missing. No header or index change.

## Done when
- `grep -nE '\bstrn?cpy[[:space:]]*\(' scene/tests/*.c` prints nothing.
- Test `scene/prefab` passes.
