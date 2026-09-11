# 049 — The engine opens on fifo, and `dev` asks for uncapped at startup

claimed-by: -
blocked-by: -
status: todo
decision: *The engine opens on FIFO, and a program asks for uncapped* (ADR-0131, restoring ADR-0067 to force) — `render` opens on fifo; a program that wants mailbox asks for it at startup; `dev` is such a program.

## Goal

A caller that opens a device and asks for nothing gets fifo. `dev` looks and behaves exactly
as it does today, because it asks for mailbox at startup.

## Scope

- **`render/src/device.c`** — remove the line that sets `device->present_wanted =
  VOE_RENDER_PRESENT_MAILBOX` at device creation. A freshly created device wants fifo.
- **`render/src/swapchain.c`** — the comment near the top documenting the mailbox default as
  deliberate and attributing it to the principal is now wrong. Delete it. What stays is the
  description of how a requested mode becomes the one in force and how the fallback to fifo
  happens; that is still accurate.
- **`dev/src/main.c`** — `mailbox_wanted` is currently initialised `true` and inherited.
  Make `dev` **ask**: request mailbox explicitly at startup, through the same public call
  the `P` key already uses. The variable's initial value and the request must agree.
- **`render/include/render/device.h`** — the header describes which of the two present modes
  a caller wants. Update the sentence so it says fifo is what a device opens on and mailbox
  is a request; keep the existing note that falling back to fifo is not a failure.
- **`render/render.md` and `dev/dev.md`** — one line each if either states the old default.
  **Present tense, no card numbers** (ADR-0120); card 047 may already have reshaped these
  files, so read them as they are rather than as described here.

## What must not change

- **The `P` key.** It swaps the two modes at runtime exactly as it does now.
- **The fallback.** A device asked for mailbox on a machine that lacks it still falls back to
  fifo, and that is still not a failure.
- **`dev`'s readout, timings and behaviour** are identical before and after — it asks for the
  mode it used to inherit.
- No other caller of `voe_render_device_*` changes. No swapchain logic changes.
- Nothing about bug 003, which is parked. Do not add a frame-rate cap, a setting, or a
  player-facing option — ADR-0131 opens that as a question and it has no card.

## Verify

- Linux: `cmake -P check.cmake` green.
- Run `voe_dev`: the frame readout shows what it showed before, and `P` still halves it to
  the display rate and back.
- `grep -rn 'PRESENT_MAILBOX' render/src` shows no assignment at device creation — only the
  request path and the fallback.
- A headless test device, which asks for nothing, opens on fifo.

## Done looks like

The engine's default is the one the record has said it was since ADR-0067, `dev` states what
it wants instead of inheriting it, and the only comment claiming otherwise is gone.

## Notes
