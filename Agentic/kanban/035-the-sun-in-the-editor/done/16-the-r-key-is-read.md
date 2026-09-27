# 16 — The R key is read
folder: platform
decisions: 0168, 0274

## Change
The editor's gizmo switch (card 17) reads R; the key list has no R yet.

- `platform/include/platform/input.h`: `VOE_PLATFORM_KEY_R` in the key enum, before
  `VOE_PLATFORM_KEY_COUNT`. The header's rule-10 paragraph gains R as the latest arrival and
  its reader: the editor's gizmo switch between move and rotate (spec 035).
- `platform/src/seat_wayland.c`: the evdev scancode switch maps `KEY_R` to it, next to the
  `KEY_Z` / `KEY_Y` cases.
- `platform/src/seat_win32.c`: both tables that name `VOE_PLATFORM_KEY_Y` gain R — the
  scancode switch near line 75 and the virtual-key array near line 192 (`'R'`).

Nothing else changes; no other folder is touched.

## Done when
`cmake --preset debug && cmake --build --preset debug --target voe_platform && ctest --test-dir build/debug -R "^platform/"`
exits 0, and `grep -c "VOE_PLATFORM_KEY_R\b" platform/src/seat_wayland.c platform/src/seat_win32.c`
prints 1 and 2.
