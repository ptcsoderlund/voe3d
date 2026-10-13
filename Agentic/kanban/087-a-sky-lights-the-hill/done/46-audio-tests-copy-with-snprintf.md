# 46 — Audio's tests copy with snprintf
folder: audio/tests
after: 43
decisions: 0168, 0413

## Change
Bug 05: `strcpy` is deprecated by the Windows headers. Each copy into a fixed `char` field becomes
`snprintf(field, sizeof field, "%s", text)` (0413 point 3); add `<stdio.h>` where missing.

- `audio/tests/sound_system.c` lines 98 and 188.
- `audio/tests/sound_component.c` line 69.

No header or index change.

## Done when
- `grep -nE '\bstrn?cpy[[:space:]]*\(' audio/tests/*.c` prints nothing.
- Tests `audio/sound_system` and `audio/sound_component` pass.
