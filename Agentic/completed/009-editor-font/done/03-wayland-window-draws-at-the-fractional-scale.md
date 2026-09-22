# 03 — The Wayland window draws at the compositor's fractional scale
folder: platform
decisions: 0168, 0180, 0177

## Change
The blur in bug 01 is the compositor stretching our buffer (KWin at 1.25), not the text path. Fix it in
`platform` only; `render`, `ui`, `text` and `editor` do not change.

- Vendor the two protocols: copy `/usr/share/wayland-protocols/staging/fractional-scale/fractional-scale-v1.xml`
  and `/usr/share/wayland-protocols/stable/viewporter/viewporter.xml` into `platform/protocol/`, unmodified. The
  build already runs `wayland-scanner` over every XML there.
- New `src/scale.h` / `src/scale.c`, OS-free like `src/input.h`: `int voe_platform_scale_length(int logical,
  uint32_t scale)` and `double voe_platform_scale_position(double logical, uint32_t scale)`, where `scale` is in
  120ths as the protocol sends it (120 = 1.0). Length rounds half away from zero, as the protocol asks.
- `src/window_wayland.c`: bind `wp_fractional_scale_manager_v1` and `wp_viewporter` off the registry at version
  1, both optional. When both are there, make a `wp_viewport` and a `wp_fractional_scale_v1` for the surface;
  `preferred_scale` stores the scale (default 120). Keep the configure's logical size; `voe_platform_window_size`
  returns it through `voe_platform_scale_length`, and whenever the logical size or the scale changes, call
  `wp_viewport_set_destination` with the logical size (the swapchain's next present commits it). Pointer
  `enter` and `motion` positions go through `voe_platform_scale_position`. Relative motion and the wheel are
  untouched. Never call `wl_surface_set_buffer_scale`. Destroy both objects before the surface.
- Rewrite the header paragraph in `window_wayland.c` that says a surface unit is a pixel of ours, and the
  "client area, in pixels" comment in `include/platform/window.h` and the position paragraph in
  `include/platform/input.h`: pixels are the buffer's, which on a fractionally scaled Wayland output are more
  than the logical size asked for in `_new`.
- Update `platform.md` (the two protocols), `src/src.md` (`scale.*`, and the window entry) and `tests/tests.md`.

## Done when
- New test `platform/tests/scale.c` passes under `ctest --test-dir build/debug -R '^platform/'`: at 120 both
  functions are the identity; at 150, 1536 → 1920, 864 → 1080 and 100.5 → 125.625; at 180, 1 → 2 (1.5 rounds
  up); at 150, 0 → 0.
- `timeout 4 env WAYLAND_DEBUG=1 ./build/debug/editor/voe_editor 2>&1 | grep -c -E 'preferred_scale|set_destination'`
  prints 2 or more (on a compositor offering the protocols; the programmer's KWin does).
- `checks.sh --all` exits 0.

For the human, on the 1.25 display, with `./build/debug/editor/voe_editor`:
1. At full screen Pixel Operator's edges are hard, no grey between letter and panel; at least as sharp as
   Oxanium was before this card.
2. Shrink the window in steps: the text shrinks smoothly, stays readable, and is never soft (a stroke one pixel
   wider than another is accepted).
3. Choose Oxanium in Preferences: it looks no worse than before.
4. Buttons and rows still answer a click exactly where they are drawn, and a view's middle-button drag still
   moves its camera.
5. `./build/debug/dev/voe_dev` still draws and answers the mouse where it did.
