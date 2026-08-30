# 006 — ask the compositor for window decorations

claimed-by:
status: todo

**Needs card 004 (`platform`) finished.** It adds to the Wayland backend.

## Read this before starting

**This card is expected to change nothing visible on GNOME.** That is not a bug
in the card and it is not a reason to go further than it says. Read the whole
thing before deciding it is incomplete.

## Goal

Ask the compositor to draw the window frame, and take it where it is offered.

Done means: `cmake -P check.cmake` exits zero on both platforms, the dev window
has a real titlebar with minimise, maximise and close on a compositor that
provides them, and the card records what each compositor answered.

## What to do

Bind `zxdg_decoration_manager_v1` from the registry, and for the toplevel:

- Create a `zxdg_toplevel_decoration_v1`.
- Request `ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE`.
- Listen for the `configure` event, which reports the mode the compositor
  actually chose. **The compositor decides, not us** — asking for server-side
  does not mean getting it.
- If the manager is absent from the registry, or the configured mode comes back
  client-side, carry on exactly as today: an undecorated window, no error, no
  warning at runtime.

The protocol XML is `unstable/xdg-decoration/xdg-decoration-unstable-v1.xml`
from `wayland-protocols`. Vendor it beside the `xdg-shell` XML and generate it
the same way — nothing generated is committed.

On Windows: **nothing.** Win32 already provides a frame. There is no
corresponding work and no `#ifdef` in shared code.

## Scope — do not exceed this

**Do not draw a titlebar.** Not buttons, not a drag region, not a subsurface,
not `libdecor`.

Drawing our own frame is wanted and it is blocked on two things that do not
exist: pointer input, which card 004 deliberately left out, and somewhere to
draw it. It is also a decision nobody has taken — it would make the Linux and
Windows windows look different from each other, or replace the native Windows
frame, and that is the principal's call.

If this card leaves the window bare on your machine, **that is the expected
outcome**, and the finding below is the deliverable.

## Report back in this card

This is the real output of the card, so be specific:

- **Which compositor you ran on, and what mode it configured.** Server-side,
  client-side, or no manager in the registry at all.
- If you can test a second compositor cheaply, do, and report that too. The
  expectation being checked is that GNOME's Mutter does not offer the protocol
  while KDE and `wlroots` compositors do. Confirming or overturning that
  expectation is worth more than the code in this card.
- Whether the decoration mode can change while the window is open, and whether
  the size reported to the application changes when it does.

## Tests

None. Same reasoning as card 004: this is looked at, not asserted on.
