# 029 — pointer position and mouse buttons

status: todo
claimed-by: -
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it follows
028 and takes the next number.

**This is the first card of the GUI stack** (`docs/prestudy/gui-stack.md` in the
planning root — you do not need it). It is entirely inside `platform` and touches
nothing else.

## The card your own header asked for

`platform/include/platform/input.h` already says this:

> **MOUSE BUTTONS ARE NOT HERE, AND THAT IS THIS CARD REPORTING RATHER THAN
> FORGETTING.** Relative motion is the mouse's whole contribution to a camera and
> it is below; nothing in the engine clicks on anything yet, so a button would be
> surface with no caller. **The card that puts something on screen to click is the
> card that adds them.**

Something is about to be put on screen to click. This is that card, and the
paragraph above is the one to rewrite when you are done.

## Goal

A program can ask **where the pointer is** and **whether a button is down**, on
both platforms.

Today `platform` offers relative motion only — *how far the mouse moved, not where
it is* — which is exactly what a camera needs and nothing a GUI can use.

## Scope — `platform`

### Where the pointer is

- A call that hands back the pointer's position **in the window's own pixels**,
  origin top-left, matching what both backends report and what
  `voe_platform_size` already means.
- **It is a position, not a delta, and both now exist side by side.** Say in the
  doc comment which is for what: motion drives a camera, position drives a
  pointer. Somebody will reach for the wrong one.
- **What it does when the pointer is not over the window** is a real question and
  the card must answer it out loud rather than let each backend decide. Recommended:
  hand back the last position and a separate *is it over the window* answer, since
  a GUI needs to know the difference between *the pointer is at the edge* and *the
  pointer is gone*.
- **What it does while the pointer is locked.** `voe_platform_input_lock_pointer`
  exists for camera look and, while it is on, there is no meaningful position.
  Decide it and write it down; do not let it return stale numbers that look live.

### Which buttons are down

- Left, right and middle. Not more: engine rule 10, implement on demand, and the
  key table's own precedent is that keys are added when something wants them.
- **Down is not enough on its own and the GUI will need more.** A button reports
  *clicked* when it is released over the same widget it was pressed on, so a caller
  needs to distinguish *held* from *went down this frame*. Whether that edge
  detection belongs here or in the GUI is **yours to decide and to state** — the
  keyboard's existing shape is level-only (`_key_down`), so matching it and leaving
  edges to the caller is the consistent answer and probably the right one. Say
  which you chose and why.

### Scroll

- A wheel delta, drained per frame like motion. The scroll area on a later card is
  the caller. **If adding it would grow this card much, leave it out and say so** —
  it is honest to report that rather than half-build it.

## What must not change

State in your report that you checked each of these:

- **Relative motion stays exactly as it is.** The camera depends on it and on its
  drain-and-forget behaviour. This card adds beside it; it does not reshape it.
- **Pointer locking stays as it is**, including that it is a request whose answer
  is `_pointer_locked`.
- **No cursor image.** The header explains at length that this engine has none to
  hand a compositor, and that a hidden cursor with no way back is worse than a
  still one. **That reasoning stands and this card does not touch it.** Drawing a
  pointer is a GUI card's problem, later, and in the world rather than as a system
  cursor.
- **The key enum.** Buttons are not keys; do not fold them in.

## Where this card is likely to go wrong

- **The two backends disagree about coordinate origin and scaling.** Wayland
  reports surface-local coordinates that are already the compositor's business, and
  the existing motion comment says the units carry pointer acceleration. Get the
  origin and the scale to agree between platforms, and **state in your report what
  each backend actually gave you** — that note is what a later Windows bug gets
  written against.
- **Linux is what you can verify.** Windows is the principal's, as always. Write the
  Win32 side carefully and say plainly that it is unverified.
- **A position that is silently stale.** Locked pointer, pointer outside the window,
  window unfocused — three states where the honest answer may be *no position*, and
  a number that looks fine is worse than a refusal.
- **Over-building.** No gestures, no double-click, no drag detection, no cursor
  shapes. Those belong to whatever needs them.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **The dev program prints or draws the pointer position and the three button
  states**, live. Move the mouse to the four corners of the window and confirm the
  numbers reach the corners and do not overshoot or invert.
- **Confirm the camera still works.** Fly around with the mouse as before — this is
  the check that motion was not disturbed.
- **Toggle pointer lock and confirm the position behaves as you documented**, rather
  than as whatever fell out.
- Move the pointer off the window and back; confirm the over-the-window answer.
- Windows is the principal's.

## Report when this lands

- Whether edge detection went in `platform` or was left to the caller, and why.
- What each backend reported for coordinates, in its own units, and what you did to
  make them agree.
- Whether scroll made it in.
- What the position does when locked, outside the window, and unfocused.
- The rewritten *mouse buttons are not here* paragraph — it is now false and it is
  the sentence a future reader will trust.
