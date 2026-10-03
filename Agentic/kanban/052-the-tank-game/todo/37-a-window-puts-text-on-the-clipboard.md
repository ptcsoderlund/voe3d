# 37 — A window puts text on the desktop's clipboard
folder: platform
after: 36
decisions: 0168, 0227, 0339, 0342

## Change
0342 point 3: a write-only clipboard, on Linux only. Read `platform/include/platform/input.h`,
`platform/src/window_wayland.h`, `platform/src/window_wayland.c`, `platform/src/seat_wayland.c`,
`platform/src/src.md` and `platform/platform.md`. Wayland's core protocol already has the data
device in `<wayland-client.h>`, so no XML is added.

- `include/platform/input.h`: declare
  `void voe_platform_input_clipboard_set(voe_platform_window *window, const char *text, uint32_t size);`
  near the cursor. The file header's "No clipboard" changes to: copying is here and pasting is not
  (rule 10). The new paragraph says: the text is UTF-8 and is not assumed to end in a NUL; the window
  keeps its own copy, so the caller's buffer may go at once; the copy is served until another
  program takes the clipboard; the compositor wants a recent key or button event, so a call before
  any input does nothing; it does nothing without a clipboard, like the cursor; Linux only (0339).
- `src/window_wayland.h`: new fields, each with its writer named. `wl_data_device_manager *`
  (window file). The latest key or button event serial (seat file). The data device, the current
  `wl_data_source *`, the owned copy and its size, and the last `wl_data_offer *` received (clipboard
  file). Declare `voe_platform_clipboard_release(window)` with the other crossings.
- `src/window_wayland.c`: the registry binds `wl_data_device_manager` at the advertised version or
  3, whichever is lower. `close_down` calls `voe_platform_clipboard_release` before the seat is
  released, then destroys the manager. Its header's list of what it binds names the manager.
- `src/seat_wayland.c`: the keyboard's key event and the pointer's button event store their serial,
  for presses and releases alike.
- New `src/clipboard_wayland.c`, with a header saying what it owns and why:
  - `voe_platform_input_clipboard_set`: asserts that `text` is not NULL when `size` is above 0. With
    no manager, no seat or no serial yet it returns. Otherwise it makes the data device on first use
    (`get_data_device`). It copies the text (`malloc`, a failed allocation fatal as in the rest of
    the folder), destroys and frees any earlier source and copy, then makes a source that offers
    `text/plain;charset=utf-8`, `text/plain` and `UTF8_STRING`, and calls `set_selection` with the
    stored serial.
  - The source's listener. `send` writes the whole copy to the fd, looping over short writes, and
    closes it. A reader that goes away early must not end the program through SIGPIPE; the header
    says how that is prevented. `cancelled` destroys that source and frees its copy if it is still
    the current one.
  - The data device's listener. Nothing reads offers, so each `wl_data_offer` it is handed is
    destroyed when `selection` replaces it, and the last one is destroyed on release.
  - `voe_platform_clipboard_release`: destroys the source, the offer and the device, and frees the
    copy.
- `src/src.md`: an entry for `clipboard_wayland.c`. The entries for `window_wayland.*` and
  `seat_wayland.c` name the manager and the serial.
- `platform.md`: `input.h`'s entry names putting text on the clipboard.

## Done when
`cmake --build --preset debug --target voe_platform && nm build/debug/platform/libvoe_platform.a | grep -q ' T voe_platform_input_clipboard_set'`
exits 0.
