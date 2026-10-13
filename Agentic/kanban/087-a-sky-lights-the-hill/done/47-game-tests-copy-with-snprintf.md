# 47 — Game's tests copy with snprintf
folder: game/tests
after: 42, 43
decisions: 0168, 0413

## Change
Bug 05: `strcpy` is deprecated by the Windows headers. Each copy into a fixed `char` field becomes
`snprintf(field, sizeof field, "%s", text)` (0413 point 3); add `<stdio.h>` where missing.

- `game/tests/models.c` lines 216, 225, 379.
- `game/tests/steps.c` line 349.

`game/tests/frame.c`'s `fopen` already carries 0159's suppression; leave it. No header or index change.

## Done when
- `grep -nE '\bstrn?cpy[[:space:]]*\(' game/tests/*.c` prints nothing.
- Tests `game/models` and `game/steps` pass.
