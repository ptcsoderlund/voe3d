# 01 — The Wayland window splits off its seat
folder: platform
decisions: 0168

## Change
`platform/src/window_wayland.c` is 1272 lines and card 02 adds to it. This card moves code and changes
nothing the program does. Read the file's header first; the paragraphs on input arriving down the same
socket, on the cursor image and on binding low go with the part that takes them.

`platform/src/window_wayland.h` — new, internal to `platform/src`. It holds `struct voe_platform_window`
(moved whole, with its comments, out of window_wayland.c) and declarations for the functions that now
cross between the two files below. Its header says what the struct is, that nothing outside `src/`
sees it, and where the split between the two `.c` files runs.

`platform/src/seat_wayland.c` — new. The seat and what arrives on it: `key_of`, `key_set`, the five
keyboard listeners, `pointer_at`, the pointer listeners and `button_of`, the relative pointer, the
locked pointer and `lock_start`/`lock_stop`, `pointer_arrived`/`pointer_left`, `seat_capabilities`,
and the public `voe_platform_window_input` and `voe_platform_window_lock_pointer` — each with the
comments above it. Functions that window_wayland.c still calls (the seat listener, whatever
`close_down` calls to release the seat) lose `static` and are declared in window_wayland.h with a
`voe_platform_` name. Its header says it is the seat half of the Wayland window and why that is its
own file; the "input is here and not in a file of its own" paragraph of window_wayland.c becomes
this reasoning, reworded to say why it is a separate file of the same backend.

`platform/src/window_wayland.c` keeps the registry, the shell, decoration, fractional scale, open,
close, poll and the small readers. Its header drops what moved.

The `_wayland` suffix keeps both new files off Windows (`cmake/voe.cmake` drops them by name).

`platform/src/src.md` gains the two entries and `window_wayland.c`'s entry is narrowed to the shell.

## Done when
`checks.sh --folder platform` exits 0, `wc -l platform/src/window_wayland.c` is under 700, and
`voe_dev` still opens, takes keys and mouse look, and closes.
