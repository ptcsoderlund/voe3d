# 006 — ask the compositor for window decorations

claimed-by: claude-code (kanban-coder)
status: review

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

---

## Result

`status: review`. Markers: **none**. No `DEVIATION:`, no `BLOCKED:`.

### What landed

    platform/protocol/xdg-decoration-unstable-v1.xml   vendored, unmodified
    platform/src/window_wayland.c                      bind, ask, listen
    platform/src/window_win32.c                        one function, always true
    platform/include/platform/window.h                 _decorated()
    platform/platform.md
    dev/src/main.c                                     reports it
    cmake/voe.cmake                                    protocol/ is globbed now

On Windows: nothing, as the card said. `window_win32.c` gained only the
`_decorated` implementation, which is `return true` — `WS_OVERLAPPEDWINDOW` is a
frame, there is nothing to negotiate and nothing that can take it away. No
`#ifdef` in shared code.

**`voe_platform_window_decorated()` is a widening of this card, made on Human's
explicit instruction and recorded here because it was.** The card asks for the
configured mode to be *reported*, and I first did that with a throwaway
instrumented binary in a scratch directory. Human's instruction was: *"Keep
everything you want me to test in the dev project"* — which cannot be done
without `platform` exposing the answer, since `dev` may not reach behind the API.
So the query is public, the throwaway is deleted, and the shipped code still
prints nothing at runtime. `guidelines.md`: an explicit instruction in the task
overrides, and says so.

`protocol/` is now globbed rather than naming `xdg-shell.xml`, for the same
reason `src/` is: vendoring a protocol is dropping in a file and it needs no
CMake edit. Adding the second XML needed no edit and proved it.

### Which compositor, and what mode it configured

**KWin (KDE Plasma, Wayland session, Fedora 44). Answer: `SERVER_SIDE`.**

`voe_dev` now opens with a real KWin titlebar — minimise, maximise, close, and a
draggable border. Human confirmed it on screen. That is the card's headline
result and it is the expected one: KDE offers `zxdg_decoration_manager_v1` and
grants server-side when asked.

**No second compositor was tested.** GNOME/Mutter is the interesting comparison
and the card's stated expectation is that it does not offer the protocol at all,
but there is no second session on this machine and installing one is not this
card's work. The expectation stands unconfirmed. Note that the code path for it
is exercised anyway: the manager is checked for NULL on the way in, and a NULL
manager leaves `decoration_mode` at zero, which is neither mode, so
`_decorated()` returns false and nothing warns.

### Can the mode change while the window is open? Measured: no, on KWin — and
### that is the most interesting thing this card found

The protocol permits the compositor to re-send `configure` at any time, and the
listener stays registered for the object's life, so the client is ready for it.

**KWin does re-send it, on every change, and the answer never changes.** Measured
directly, with a temporary print inside the configure handler, opening the window
and toggling **More Actions -> No Borders** off and then on again:

    [TEMP] decoration configure -> mode 2 (SERVER_SIDE)
    opened     960x540
    decorated  yes
    [TEMP] decoration configure -> mode 2 (SERVER_SIDE)     <- borders removed
    size       970x567
    [TEMP] decoration configure -> mode 2 (SERVER_SIDE)     <- borders restored
    size       960x540
    closed

Three configures for three state changes. All three `SERVER_SIDE`. The
instrumentation was reverted afterwards and the shipped code prints nothing.

That single run answers both of the card's questions and one it did not ask:

- **Can the mode change while the window is open?** The *event* fires every time.
  The *value* never moves. Those are different questions and only the second one
  matters to a caller.
- **Does the reported size change when it does?** Yes, and it is the only thing
  that does: 960x540 with a frame, 970x567 without one. KWin keeps the outer
  geometry and hands the client the ten by twenty-seven pixels the frame had.
- **The mode names who is *responsible* for the frame, not whether one exists.**
  KWin is responsible in both states and has merely chosen to draw nothing in one
  of them, so `SERVER_SIDE` is the honest answer both times. A client that drew
  its own frame on seeing the titlebar vanish would be wrong, and this is why.

The practical rule for anything downstream: **a decoration change cannot be
observed through the decoration protocol on this compositor, and arrives as a
plain resize instead** — which is the event a swapchain has to react to anyway,
so nothing is lost by not seeing the first one.

It also settles what `voe_platform_window_decorated()` can and cannot be asked.
It is constant-true on KWin and constant-true on Windows. The one case where it
is expected to differ is a compositor offering no manager at all, which is the
untested GNOME case below — so the function's whole value now rests on the one
thing this card did not verify.

> **Two corrections, both to this section, neither to the code.** The first
> version said toggling printed nothing and the client area did not move; the
> second said KWin sends the configure only once. Both were written from partial
> reports before the measurement above existed. Three file headers carried the
> wrong version at one point or another and all three now carry the measured one:
> `platform/include/platform/window.h`, `platform/src/window_wayland.c` and
> `dev/src/main.c`. The behaviour of the code never changed.

This corrected a comment that would otherwise have shipped as a lie.
`_decorated()` was first documented as "true when something is drawing the
window's frame". The measurement above says that is wrong, and the header now
says *who is responsible* and names the KWin case as the reason the distinction
is written down. **A caller that reads it as "is there a titlebar on screen" is
wrong on exactly that case**, and now the header says so.

### Which C23 features were used

**None new.** `bool` as a keyword, already in use from card 004. The floor stays
at clang 18.

### Verified

`cmake -P check.cmake`, all twelve steps ok, run again after the documentation
correction:

    ok    tools (clang 22, cmake 4.3.0, slangc, wayland-scanner)
    ok    standalone base / dev / math / platform
    ok    root configure and build
    ok    guard compiler / version / map
    ok    includes
    ok    tests (1 passed)
    ok    harness reports a failure

Same standing caveat as cards 003–005: **`slangc` is still not installed**, so
the run used a stub on `PATH` from the scratch directory, with nothing in the
repository changed.

Also verified:

- **Both protocols generate and compile** —
  `wayland-scanner: xdg-decoration-unstable-v1 client header` and
  `... protocol code` appear in the build alongside the xdg-shell pair, and
  adding the second XML required no CMake edit.
- **Nothing generated reaches the source tree.** `platform/protocol/` holds the
  two XML files and nothing else after a full build.
- **The scratch instrumented binary is deleted**, and the shipped
  `window_wayland.c` contains no `printf` — the card's "no warning at runtime"
  holds. What `dev` prints is `dev`'s business, and it prints on change only.

### Not verified

- **GNOME/Mutter, or any second compositor.** The card's central expectation is
  therefore neither confirmed nor overturned. It is the one thing left in this
  card that is worth doing and it needs a machine with another session.
- **Windows, entirely.** `window_win32.c` has still never been compiled by
  anyone. `_decorated()` there is three lines and `return true`, but it has not
  been through a compiler.
- **`git` is still unusable in this checkout** — `.git` is a gitlink to a
  `../.git/modules/voe3d` that does not exist. Unchanged from cards 001–005;
  the card was moved with `mv` and nothing was committed.

## Notes — suggestions, not done here

- **`zxdg_decoration_manager_v1` is bound at version 1; the interface is at
  version 2.** Version 1 carries everything this card needs and binding low is
  what keeps unhandled listener slots unreachable, which is the rule the Wayland
  backend already follows. Worth revisiting only if something wants what v2
  added.
- **Testing a second compositor is cheap in principle and awkward in practice.**
  A nested `weston` or a `wlroots` compositor would answer the GNOME question
  without a second session, and `weston --backend=wayland` runs inside the
  current one. That is its own small card, and it would confirm or overturn the
  expectation this card left standing.
- **`_decorated()` has one real caller and it is `dev`.** That is deliberate —
  nothing in the engine should behave differently on it — but it does mean the
  function exists to be looked at rather than used. If that stops being true, the
  thing that starts using it is probably wrong.
