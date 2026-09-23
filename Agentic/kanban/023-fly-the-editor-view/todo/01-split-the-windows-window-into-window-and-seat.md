# 01 — Split the Windows window into window and seat
folder: platform
decisions: 0168, 0233

## Change
`platform/src/window_win32.c` is 840 lines and card 02 changes its lock. Split it by function the way
the Wayland side already is (`window_wayland.h`, `window_wayland.c`, `seat_wayland.c`); move code,
change no behaviour. The Win32 files are not compiled on Linux, so keep every moved line as it is.

- `platform/src/window_win32.h` (new) — `struct voe_platform_window` moved out of `window_win32.c`,
  and declarations of the functions that cross between the two files. Header comment: the struct both
  halves write, seen by nothing outside this folder, and which functions cross.
- `platform/src/seat_win32.c` (new) — the keyboard, the mouse and the lock, moved from
  `window_win32.c`: `key_of`, `key_set`, `handle_char`, `focus_gained`, `clip_to_client`,
  `apply_lock`, `raw_input`, `pointer_inside`, `pointer_at`, `any_button_down`, `button_set`,
  `capture_lost`, `cursor_show`, and the public `voe_platform_window_input`,
  `voe_platform_window_lock_pointer` and `voe_platform_window_cursor`. Those `window_proc` calls lose
  `static` and gain a `voe_platform_seat_` prefix (rule 7), declared in `window_win32.h`. File
  header: the seat half of the Windows window; takes over the "input is here", "the lock is a clip and
  a hide" paragraphs from `window_win32.c`'s header, reworded for a file of its own.
- `platform/src/window_win32.c` — keeps `window_of`, `window_proc`, `class_ready`, `_new`,
  `_destroy`, `_poll`, `_wait` and the queries; includes `window_win32.h`; its header drops what
  moved and names `seat_win32.c` for input.
- `platform/src/src.md` — entries for `window_win32.h` and `seat_win32.c`; the `window_win32.c`
  entry says what stayed.

Read `window_win32.c` whole, `window_wayland.h`'s header for the shape, and `src.md`.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `platform` exits 0, and
`wc -l platform/src/window_win32.c platform/src/seat_win32.c` shows each under 800.
