# 36 — C is a key
folder: platform
after: 32
decisions: 0168, 0339, 0342

## Change
0342 point 4: the editor's Ctrl+C needs the key where C sits. Read
`platform/include/platform/input.h` (its key enum and the paragraphs above it) and the evdev key map
near the top of `platform/src/seat_wayland.c`.

- `include/platform/input.h`: `VOE_PLATFORM_KEY_C` is added after `VOE_PLATFORM_KEY_R`, before the
  count, so no existing key's value moves. The "THE LIST IS SHORT" paragraph names C as the latest
  arrival: the editor's Ctrl+C, which copies from the Errors panel (052 bug 05).
- `src/seat_wayland.c`: the evdev map turns `KEY_C` into `VOE_PLATFORM_KEY_C`. Its comment's count
  of keys is brought up to date.
- `seat_win32.c` is not touched (0339).

## Done when
`grep -q 'VOE_PLATFORM_KEY_C,' platform/include/platform/input.h && grep -q 'case KEY_C:' platform/src/seat_wayland.c`
exits 0, and the folder builds.
