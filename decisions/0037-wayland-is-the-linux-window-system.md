# 0037. Wayland is the Linux window system; no SDL

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

D-042, blocking card 004. `platform` needs a window on Linux, and the two
candidate backends share no code, so the choice has to be made before the card
starts. The principal runs Wayland on the Linux machine and Windows 11 on the
other.

The question widened once, deliberately: the principal asked what SDL3 would
mean. Because SDL3 handles Wayland, X11 and Win32 and picks at runtime, adopting
it would have dissolved this decision rather than answered it. So the two are
one decision and are recorded together.

Constraints:

- **The onboarding invariant.** Clone and build, without assembling an
  environment.
- **ADR-0021's line.** Tools that transform source are installed by the
  programmer; anything linked or shipped is fetched by the build.
- **ADR-0026's precedent, which turns out to decide this.** Windows already
  requires the Windows SDK and the MSVC C runtime, from Visual Studio or its
  Build Tools, and that was classified as **a tool, not a dependency** — the
  platform's own development package, without which no native code is built at
  all.
- **ADR-0023.** A third-party dependency is asked about, never assumed. The
  principal was asked and declined SDL3.

## Options considered

### Option A — Wayland, natively
Native on the principal's machine: correct scaling, no translation layer, the
direction the Linux desktop has gone.

Costs: the `xdg-shell` protocol must be turned into C by `wayland-scanner`, and
the client library must be reachable. Both come from the distribution's Wayland
development package. No X11 fallback unless one is written.

### Option B — X11
One library present on every desktop, no code generation, and it runs everywhere
including under a Wayland compositor via XWayland.

Costs: on the principal's own machine, every day, through a translation layer —
XWayland's scaling rather than the compositor's. And it is a backend for a
display server the principal is not running.

### Option C — SDL3
Deletes most of `platform`. Wayland, X11 and Win32 with a runtime fallback, plus
input, gamepad, clipboard, HiDPI, multi-monitor, monitor hotplug and audio — a
swamp of edge cases, already drained. zlib-licensed, CMake, `FetchContent`, so
it sits on the fetched side of ADR-0021 and does not break the invariant.

Costs: the project's first dependency, and a large one, in a policy that has
been doing real work. Nine slower standalone configures in `check.cmake`. And an
unverified risk that SDL3's own Linux build wants the same Wayland development
package, in which case the dependency has moved rather than disappeared.

## Decision

**Option A. Wayland only on Linux, and no SDL.**

Deciding factor, on SDL3: the principal's call, and it is the one this project's
policy points at — windowing is tedious rather than interesting, but ADR-0023's
default is to write it, and the card's API already hides whichever answer we
pick.

Deciding factor, on Wayland over X11: it is what the machine runs. X11 would
have meant developing through a compatibility layer for a display server nobody
here uses.

Fixed by this ADR:

1. **Wayland only. No X11 backend**, not even as a fallback, until something
   needs one — ADR-0034. The engine will not run on an X11-only session or over
   plain X forwarding. Accepted.
2. **The Wayland development package is a tool, not a dependency.** It is added
   to the required-tools list for Linux, beside Clang, CMake and `slangc`
   (`wayland-devel` on Fedora, `libwayland-dev` + `libwayland-bin` on Debian).
   This is the same classification ADR-0026 already gave the Windows SDK and the
   MSVC runtime, and the symmetry is the argument: each platform's own
   development package is a tool, and neither platform builds native code
   without one. The onboarding invariant is intact — one more install on the
   tools line, not an SDK to hunt down.
3. **The protocol descriptions are vendored**; `wayland-scanner` runs at build
   time and its output goes to the build directory. **Nothing generated is
   committed**, consistent with the shader decision.
4. **`check.cmake` step 1 checks for `wayland-scanner` on Linux** and fails
   naming this ADR when it is absent, the way it already checks the other tools.
5. **No client-side decorations, and no `libdecor`.** Wayland does not guarantee
   server-side decorations, and GNOME — the principal's desktop — does not
   provide them. A window will therefore appear without a titlebar or borders.
   That is acceptable and arguably correct for a game engine, which is usually
   borderless anyway; the compositor's own shortcuts move and resize it.
   Drawing decorations ourselves, or adding `libdecor`, is a dependency and a
   later decision if it is ever wanted.

## Blast radius

**Reversibility: moderate, and contained by the card's API.** Card 004 specifies
six functions and forbids any OS type from appearing in a public header, so the
whole backend sits behind an interface nothing above it can see. Adding an X11
backend later, or replacing the lot with SDL3, changes `platform/src/` and
nothing else. This is a dependency-shaped decision that is not load-bearing,
because of a constraint taken for unrelated reasons.

The part that is *not* cheap to reverse is item 2 — once a system development
package is on the tools list, the next one is easier to add, and the tools list
is the thing standing between this project and "just install the SDK".

## Consequences

- **The tools list grows for the first time**, and it is now asymmetric: Windows
  needs the Windows SDK and MSVC CRT, Linux needs the Wayland development
  package. Honest, symmetric in principle, and it must be stated where a
  contributor meets it before their first build fails — D-032.
- **An undecorated window.** The dev program in card 004 will open a rectangle
  with no titlebar on GNOME. Expected, not a bug, and stated in the card so it
  is not reported as one.
- **No X11 means no X forwarding and no X11-only session.** Nobody here is
  affected today.
- **The Wayland backend is more work than X11 would have been.** `xdg-shell` is
  a handshake, not a call. This is the cost of the machine the principal
  actually uses.
- **SDL3 remains available and remains cheap to adopt**, because of the API
  boundary. If `platform` turns into a swamp of compositor edge cases, this is
  the decision to revisit, and revisiting it costs one folder's `src/`.
- **The check script gains a platform-conditional step.** Its first
  Linux-versus-Windows divergence beyond the compiler guards.

## Rejected options and why

- **Option B (X11)** — a compatibility layer for a display server the principal
  does not run. Its real advantage, running everywhere, buys nothing today and
  is available later as a second backend behind the same API.
- **Option C (SDL3)** — declined by the principal. It was the pragmatic choice
  and would have reached a triangle sooner; ADR-0023's default is to write it,
  and the answer to "may we add a dependency" was no.
- **Vendoring the generated protocol C to avoid requiring `wayland-scanner`** —
  keeps the tools list at three, at the price of committing generated code, which
  the shader decision already rejected for good reasons. Kept as the escape
  hatch if the tool requirement ever proves to be a real barrier.
- **`libdecor` for window decorations** — a second dependency to put a titlebar
  on a window that does not need one.

## Questions this opens

- **Closes D-042.** Card 004 is unblocked.
- **D-032 grows.** The required-tools list now differs per platform and must be
  stated somewhere a contributor reads before building.
- **`check.cmake` step 1** needs the `wayland-scanner` check — part of card 004.
