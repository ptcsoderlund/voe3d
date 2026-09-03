# 016 — an input layer

status: todo
claimed-by: claude-code (kanban-coder)
blocked-by: 015

Keyboard and mouse, in `platform`, on both platforms. Ships with a consumer:
by the end of this card the camera is flown by hand instead of orbiting.

## Goal

Move and look with keyboard and mouse in the dev window.

## Scope

- **Keyboard and mouse**, per the v1 capability list. Gamepad is on the *later*
  list and is not this card.
- **`platform` owns it and names no Vulkan and no renderer.** The seam that already
  exists for window handles is the model to copy.
- The camera from card 015 is driven by it. The orbit stays available — it is the
  better thing to look at when checking rendering rather than input.

## What makes this two-platform work rather than one

    Linux     wl_seat, wl_keyboard, wl_pointer   (Wayland, ADR-0037)
    Windows   WM_KEYDOWN / WM_INPUT / WM_MOUSEMOVE

**Decide and state whether input is polled state or a queue of events.** A camera
wants "is this key held"; a future editor wants "what happened, in order". Pick
one, say why, and do not build both.

**Mouse look needs relative motion, not cursor position**, and on Wayland that is
`zwp_relative_pointer_v1` plus a pointer lock — a separate protocol from the ones
already vendored. If that turns into more than this card should carry, report it
rather than half-building it.

## What this unblocks

The parked question of whether VOE3D draws its own window frame. That was blocked
on pointer input and stays parked — this card does not answer it, it just stops
being the reason.

## Verify

- Fly the camera on both platforms. Held keys, released keys, focus lost with a key
  still down — that last one is where input layers leak state.
- `check.cmake` zero. Windows is the principal's.
