# 049 — The engine opens on fifo, and `dev` asks for uncapped at startup

claimed-by: claude-opus-5 (kanban-coder, session af55c55a)
blocked-by: -
status: review
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

**Verified on Linux only** — Fedora 44, clang 22.1.8, NVIDIA RTX 4070 Laptop GPU, Vulkan
1.4.341, Wayland. Windows not checked.

- `cmake -P check.cmake`: exit 0, every step `ok` — all 12 folders standalone, 40 tests
  passed, the analyser clean over 107 files, no warnings.
- `grep -rn 'PRESENT_MAILBOX' render/src`: three hits, all in the request path —
  `swapchain.c` `present_mode_for` (the test and the fallback's other arm) and
  `device.c` `voe_render_present_set`'s assert. Nothing at device creation.
- **Headless opens on fifo.** A probe printed `present_wanted` and `_get` from
  `voe_render_device_new_headless` under `voe_test_render_pools`: `wanted=fifo in_force=fifo`,
  both devices. Reverted before `check.cmake` ran.
- **`voe_dev`, 7 s each, before and after** (unchanged binary kept for the comparison). Every
  timing block says `mailbox` in both: 1770 and 1908 frames/s before, 1914 and 2371 after,
  which is run-to-run spread.
- **One readout line differs, accepted by the principal 2026-09-11.** The startup `present`
  line said `mailbox` and now says `fifo`. `voe_render_device_new` builds the swapchain before
  it returns, so it is built on fifo, and `dev`'s request is acted on at the top of the first
  frame. Holding the line identical needs a request before the swapchain exists, a `render`
  change the card rules out. A comment at that print in `dev/src/main.c` says why it reads
  fifo. That first-frame rebuild is the only other behavioural difference; the after-run's first
  block had a 33 ms worst frame against 11 ms before. That is one sample, not established as
  the rebuild.
- **`P` not pressed** — a script cannot press a key. Its code, the call and the variable are
  unchanged. It needs a person to confirm it still halves the rate and back.
- `render/render.md` states no default and is unchanged. `dev/dev.md`: one line — asking for
  mailbox at startup is now a decision in `dev` beside the key bindings.
- `device.h`: besides the sentence the card names, the `MAILBOX` enumerator's comment and
  `_get`'s comment also said a device opens on mailbox, and are corrected. `_set` gains one
  sentence: a request made straight after opening takes effect on the first frame.
- Two folders, `render` and `dev`, both named by the card. No `DEVIATION:` or `BLOCKED:` markers.
