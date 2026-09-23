# 02 — Hide a locked pointer and put it back where it was
folder: platform
decisions: 0168, 0227, 0233

## Change
While the lock holds, the pointer is hidden, and when it ends the pointer shows where it was (0233).
No signature changes.

- `platform/src/seat_wayland.c`
  - `locked_pointer_locked`: when the cursor-shape device is bound, hide the cursor with
    `wl_pointer_set_cursor(pointer, <enter serial>, NULL, 0, 0)`; without it do nothing, as today.
  - `locked_pointer_unlocked` and `lock_stop`: when the cursor was hidden, put the shape back through
    `cursor_apply`. Keep one flag in the window struct (`platform/src/window_wayland.h`) saying the
    cursor is hidden, so `cursor_apply` on enter or a shape change does not show it mid-lock, and so
    it is shown exactly once.
  - The header's "nothing here hides the cursor" paragraph becomes: it hides a locked pointer only
    when cursor-shape-v1 can bring it back by name; without it, frozen and visible; the lock is
    frozen in place, so it shows where it was.
- `platform/src/window_wayland.h` — the flag, with a comment on what it means.
- `platform/src/seat_win32.c` (made by card 01) — `apply_lock`: when the clip starts, remember the
  cursor's screen position (`GetCursorPos`) in a field of the struct in `platform/src/window_win32.h`;
  when it ends, put it back with `SetCursorPos` before the cursor is shown. Header: the lock gives the
  pointer back where it was taken.
- `platform/include/platform/input.h` — the `voe_platform_input_lock_pointer` paragraph "what happens
  to the arrow differs" now says both platforms hide it while locked and show it where it was, with
  Wayland's one exception (no cursor-shape-v1: frozen and visible); drop "the card that owns a cursor
  image"; the `over` paragraph's "frozen on Wayland, clipped but tracked on Windows" says Windows puts
  it back.
- `platform/src/src.md` — the `seat_wayland.c` entry no longer says the cursor is never hidden.
- `platform/platform.md` — the `input.h` entry: the lock hides the pointer.

Read the headers of the files named and the functions named in them.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `platform` exits 0, and
`grep -c "never hidden" platform/src/src.md` prints 0.
