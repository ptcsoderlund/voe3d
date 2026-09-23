# 01 — The pointer takes a resize shape
folder: platform
decisions: 0168, 0180, 0227

## Change
A caller names the pointer's shape; each backend shows it.

- `platform/protocol/` — copy in `/usr/share/wayland-protocols/staging/cursor-shape/cursor-shape-v1.xml`
  and `/usr/share/wayland-protocols/stable/tablet/tablet-v2.xml` unchanged (the first's generated code
  names `zwp_tablet_tool_v2_interface`, which the second defines). The build globs this folder.
- `platform/include/platform/input.h` — `voe_platform_cursor` enum: `_ARROW` (zero), `_LEFT_RIGHT`,
  `_UP_DOWN`, `_COUNT` (not a shape; asserts). `void voe_platform_input_cursor(voe_platform_window *window,
  voe_platform_cursor cursor)`. Header points: a request by name, drawn by the window system in the
  person's own cursor theme; set it every frame, only a change reaches the window system; on a compositor
  without the protocol nothing changes and nothing fails. The lock paragraph's "the card that owns a cursor
  image" stays true for hiding one; say the shape is now named here.
- `platform/src/input.h` — `struct voe_platform_input` gains `voe_platform_cursor cursor`, plain state that
  no poll, focus loss or pointer loss clears. Declare the backend hook
  `void voe_platform_window_cursor(voe_platform_window *window, voe_platform_cursor cursor)`, the way
  `voe_platform_window_lock_pointer` is.
- `platform/src/input.c` — `voe_platform_input_cursor` asserts the window and the range, stores the shape
  and calls the hook only when it differs from the stored one.
- `platform/src/window_wayland.h` — `struct wp_cursor_shape_manager_v1 *cursor_shapes` (window's),
  `struct wp_cursor_shape_device_v1 *cursor_shape` and `uint32_t pointer_serial` (seat's).
- `platform/src/window_wayland.c` — bind `wp_cursor_shape_manager_v1` at version 1 in the registry when
  offered, destroy it on close; header's list of optional protocols gains it.
- `platform/src/seat_wayland.c` — make the shape device with the pointer and destroy it with it; keep the
  enter serial; on enter and in `voe_platform_window_cursor` (while the pointer is over and a device
  exists) `set_shape` with it: ARROW → DEFAULT, LEFT_RIGHT → EW_RESIZE, UP_DOWN → NS_RESIZE. Header: the
  "nothing here touches the cursor image" paragraph now says the shape is set by name and hiding for the
  lock still is not.
- `platform/src/window_win32.c` — answer `WM_SETCURSOR` with hit test `HTCLIENT` by `SetCursor` of
  `IDC_ARROW`, `IDC_SIZEWE` or `IDC_SIZENS` for the stored shape and return TRUE (other hit tests to
  `DefWindowProc`); `voe_platform_window_cursor` calls `SetCursor` at once while the pointer is over.
  Linux cannot build this file; keep the change that small.
- `platform/tests/input.c` — `a_poll_and_a_loss_keep_the_cursor`: a stored shape survives
  `begin_poll`, `focus_lost` and `pointer_lost`. Header gains a line for it.
- `platform/platform.md` — the two XML entries (what each is, optional, the second only for the first's
  types); the `input.h` entry says the pointer's shape. `platform/src/src.md` — the `seat_wayland.c` and
  `window_wayland.c` entries; `platform/tests/tests.md` — the `input.c` entry.

## Done when
The Checks line of `CLAUDE.md` with `{folder}` = `platform` exits 0, and `platform/input` runs
`a_poll_and_a_loss_keep_the_cursor`.
