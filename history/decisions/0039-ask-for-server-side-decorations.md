# 0039. Ask the compositor for decorations; draw none ourselves yet

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Amends:** ADR-0037 §5
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal ran a window and reported the expected consequence of ADR-0037 §5:
no minimise, maximise or close buttons, and no titlebar. They would like one.

§5 said "no client-side decorations, and no `libdecor`", and named drawing them
ourselves or adding `libdecor` as a later decision. This is that decision, and it
splits into two very different halves.

**The half that is cheap.** Wayland has a protocol,
`zxdg_decoration_manager_v1`, by which a client asks the compositor to draw the
window frame. Where a compositor implements it, that is real, native, correct
decorations for about thirty lines of code and no dependency. KDE Plasma,
`wlroots` compositors — sway, Hyprland — and others implement it.

**The half that is not.** GNOME's compositor, Mutter, is understood not to
implement it: GNOME's position is that clients draw their own frames, which is
what GTK does. Fedora's default desktop is GNOME, so **on the principal's own
machine the cheap half is expected to do nothing.** That expectation is not
taken on faith — the card that implements this reports what the compositor
actually answered, so if it is wrong we find out rather than assume.

Getting buttons on GNOME therefore requires drawing them, and that runs into
things that do not exist:

- **Pointer input.** Card 004 deliberately excludes input; nothing consumes a
  click yet. A button that cannot be clicked is not a button.
- **Somewhere to draw.** The window's content will belong to Vulkan. A frame
  drawn by `platform` needs either a `wl_subsurface` with its own software
  buffer, or the renderer, or `ui` — the last two of which are far off.
- **Windows does none of this**, because Win32 provides a frame. So a
  self-drawn frame is either Linux-only, which makes the two platforms look
  different, or it replaces the native Windows frame too, which is a much
  larger decision about what a VOE3D window is.

## Options considered

### Option A — Ask for server-side decorations; defer the rest
Implement `zxdg_decoration_manager_v1` and request server-side mode. Real
decorations wherever the compositor offers them, nothing on GNOME, no
dependency, no input required.

### Option B — `libdecor`
A library that draws client-side decorations, with a GTK-backed plugin that
produces a GNOME-native-looking frame. Solves the principal's actual complaint
on the principal's actual machine.

Costs: a third-party dependency, which ADR-0023 requires asking about, and a
heavier one than it looks — the good-looking plugin path pulls GTK. It is also
not part of a base system the way `libwayland-client` is, so it cannot be
treated the way ADR-0037 treated the Wayland package.

### Option C — Draw the frame ourselves, now
A `wl_subsurface` with a software buffer, three hit-tested buttons calling
`xdg_toplevel_set_minimized`, `set_maximized`, `close`, and a drag region
calling `xdg_toplevel_move`.

Costs: pointer input first, several hundred lines of drawing and hit-testing
inside `platform`, and a per-platform divergence. Genuinely wanted, genuinely
blocked.

## Decision

**Option A now.** Ask the compositor for server-side decorations, take them
where they are offered, and carry on undecorated where they are not.

Deciding factor: it is thirty lines and no dependency, and it is the only part
of this that can be built today. Options B and C are both real answers to the
principal's request and neither is buildable this week — B needs a dependency
decision the principal has twice declined in spirit, and C needs input, which
does not exist.

Amending ADR-0037 §5: "no decorations" becomes "no decorations *we draw*". Asking
for them is now required.

**Not decided, and recorded as open:** how a VOE3D window gets a frame on GNOME.
That is D-043, and it needs pointer input before it can be answered with code.

## Blast radius

**Reversibility: cheap.** The decoration request is a few lines in the Wayland
backend, and removing it returns to today's behaviour.

## Consequences

- **The principal's own machine is expected to look unchanged.** This is the
  unsatisfying part and it should be said plainly rather than discovered: this
  decision most likely delivers nothing visible on GNOME. It is still right,
  because the alternative is doing nothing at all on the compositors where it
  works.
- **The engine will look different on different Linux desktops.** Decorated on
  KDE and sway, bare on GNOME. Accepted for now.
- **We learn something either way.** The card reports what the compositor
  answered, which either confirms the GNOME expectation or overturns it.
- **D-043 will not stay deferred for long.** A window without a close button is
  tolerable for a dev project and not for an engine, and the principal has
  already asked once.

## Rejected options and why

- **Option B (`libdecor`)** — the direct answer, and a third-party dependency in
  a project whose default is to write it, for window furniture. Not rejected on
  principle forever; rejected as something to adopt without the principal
  deciding it explicitly, which ADR-0023 requires.
- **Option C (draw it now)** — blocked on pointer input, which no card has
  produced and nothing else needs yet.

## Questions this opens

- **D-043, new:** how a VOE3D window gets a frame on a compositor that draws
  none — `libdecor`, our own subsurface frame, or a deliberate choice to be
  borderless. Blocked on pointer input, and it will want the principal's view on
  whether the Windows window keeps its native frame.
