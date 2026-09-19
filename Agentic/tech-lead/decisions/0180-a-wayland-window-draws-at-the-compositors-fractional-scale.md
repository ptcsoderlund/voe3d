# 0180 — A Wayland window draws at the compositor's fractional scale, and its pixels are the buffer's
date: 2026-09-18
by: planner

## Decision
On Linux, `platform`'s window binds `wp_fractional_scale_manager_v1` and `wp_viewporter` when the compositor
offers them (both optional, bound at version 1, as xdg-decoration is). The surface's buffer is drawn at the
preferred scale: buffer size = logical size × scale / 120, rounded half away from zero, and the viewport's
destination is the logical size. `wl_surface_set_buffer_scale` stays unused (1).

`voe_platform_window_size` and the pointer's position are in buffer pixels: the logical values the compositor
sends are multiplied by the same scale in the one file that receives them. Relative motion and the wheel are
not scaled. With neither protocol offered, nothing changes from today.

Nothing above `platform` changes: `render` already builds the swapchain at the window's size on Wayland, and the
editor and dev already derive millimetres from the window's height (ADR-0104), so the interface keeps its size on
screen and gains only sharpness.

## Reasoning
Bug 01 of spec 009: Pixel Operator is smeared. The programmer's KWin runs at 1.25; the surface has no buffer
scale, so the compositor stretches a 1536×864 picture onto 1920×1080 with a smoothing filter. The engine draws no
grey (a threshold in `elements.slang`, nearest sampling and a nearest blit); the compositor does.
ADR-0103's amendment and ADR-0104 foresaw this as D-188 — "reading the number and rendering sharply are one act",
"one multiplication in one file" — and left it open only because nothing had asked. This does it for Wayland.
- **Integer `wl_surface_set_buffer_scale` from the output**: rejected; it cannot express 1.25 and still blurs.
- **Leaving it to the compositor**: rejected; that is the bug.
Windows' DPI awareness, the other half of D-188, stays open: Linux alone verifies a card (ADR-0130).

## Replaces
D-188's Wayland half. The Wayland backend's "a surface unit is a pixel of ours" in `window_wayland.c`.
