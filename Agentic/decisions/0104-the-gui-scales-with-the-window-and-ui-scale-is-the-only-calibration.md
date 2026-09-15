# 0104. The GUI scales with the window, and `ui_scale` is the only calibration

- **Status:** Accepted
- **Date:** 2026-09-09
- **Deciders:** Human, Tech Lead
- **Supersedes:** ADR-0103
- **Superseded by:** —
- **Closes:** D-177 (finally), D-190 opened in its place

## Context

The principal, on being told that reading a display's scale needs per-platform code:

> Its still not in the vulkan api. I would rather not use it. What is our option
> then? a gui scales with windows height. dev and/or enduser can change ui_scale
> which calibrates to their display or how they want to play their game. Which
> inverts our dialogue. Instead of perfect scaling we just do window relative and
> fine tuning can someone else do. In any case, i want to keep ratio of text and
> gui elements.

**One correction to the premise, then the decision is adopted whole.** A display's
scale was never going to come from Vulkan — it is a window-system fact, and
`platform` exists to own exactly those. But the substance underneath is right and is
the better argument: **reading it is per-platform code with real asymmetries**, and
card 042 had already accumulated three of them before a line was written — the work
area exists on Windows and not on Wayland, Windows returns 96 until the process
declares awareness, and Wayland's physical size is unreliable. That is the surface he
is declining, and declining surface is this project's habit.

**"Which inverts our dialogue" is exactly right, and it is the useful sentence.** The
engine stops trying to be *correct* about physical size and becomes *predictable*
about proportion, with a human calibrating the rest. That is how games have always
done it, and it is one number instead of a platform integration.

## What has oscillated today, and what has not

Worth stating because this is the third ADR on one number in a day:

- **ADR-0100** — one uniform scale, taken from the window's height. The real content
  was the fix: width and height were being scaled independently and everything,
  text included, was deformed by a resize.
- **ADR-0103** — the scale becomes **a parameter**: `render` is handed
  pixels-per-millimetre and holds no policy. Its default source became the display.
- **This ADR** — **keep ADR-0103's parameter, restore ADR-0100's source, and add
  `ui_scale`.**

**The mechanism never moved. What moved is where one number comes from, twice — and
the reason that was cheap is precisely ADR-0103's parameter**: `render` never learned
either policy, so neither reversal touched a line of it. Three decisions, one number,
no code churn. That is the argument for keeping policy out of a leaf folder, and it
is now demonstrated rather than asserted.

## Decision

1. **Pixels per millimetre = (the target's height in pixels ÷ the surface's authored
   height in millimetres) × `ui_scale`.** One scale, both axes, and the surface's
   width in millimetres follows from it — ADR-0103's call keeps its shape exactly,
   and this is simply what the caller now passes it.
2. **`ui_scale` is the only calibration, and it belongs to the program and its
   user.** Default 1.0. It multiplies the scale, so raising it makes everything
   bigger and shows less of the surface — which is what a *UI scale* slider in a game
   does, and what it means for a developer shipping a default that suits their art.
3. **Nothing in the engine reads a display.** No DPI, no scale factor, no monitor
   size, no work area, on any platform. `platform` grows no display surface and
   **card 042 is withdrawn** with nothing built.
4. **The ratio is preserved, which was the principal's explicit requirement and was
   ADR-0100's real content.** One scale on both axes, text and boxes together.
   Anisotropic stretch — the bug this whole thread started from — stays fixed.
5. **A millimetre is now window-relative.** Not physical, and not nominal against 96
   per inch either: an authored 8 mm is a proportion of the surface's authored
   height, made physical only by whatever `ui_scale` the person in front of the
   screen chose. **This is the third meaning the unit has had today and the header
   must say the current one plainly.** Whether it should still be called a millimetre
   is a real question and is D-190; it is not answered here because renaming it
   touches every header and card, and it is cheap only until programs author panels.
6. **The sharpness problem is untouched by this decision and is made cheaper by
   it** — see below. It is D-188 and it stands on its own.

## Why this makes the sharpness fix nearly free, which is the useful consequence

On a display set to 150%, with no density awareness: Windows reports a window
two-thirds the real pixel height and stretches our output to fit. With awareness: it
reports the true pixel height and we draw every pixel.

**Under this ADR the GUI fills the window's height in both cases, so its physical
size is identical and only its sharpness differs.** The scale is derived from
whatever pixel height we are told, and being told a bigger number means a
proportionally bigger scale — the interface lands in the same place, drawn properly.

**Under ADR-0103's policy the same change would have moved sizes**, because the
display's scale would have been applied on top of a window size that had itself
changed meaning. So this decision decouples the two questions that were welded
together an hour ago: **density awareness becomes a pure sharpness change with no
size consequence**, and it needs no scale factor read from anywhere — only the
window's true pixel count, which `platform` already reports.

What remains of its cost is Wayland-specific and real: a surface with a buffer scale
renders in physical pixels while pointer coordinates stay in logical surface units,
so those two must be reconciled where today they are the same number. That is D-188's
work and it is one multiplication in one file.

## Blast radius

**Cheap.** One call site computes the scale, `dev` owns the `ui_scale` it passes, and
the call `render` exposes does not change. Card 042 is withdrawn before anybody wrote
code for it.

Reversibility: **cheap.** Reading a display later would be a new platform card and a
different number passed to the same call.

## Consequences

- **A small window has small text**, which is the known weakness of proportional
  scaling: a game windowed at 800×600 on a large monitor gets a small interface.
  **That is the trade being accepted rather than an oversight** — `ui_scale` is the
  answer and it is the user's to set, which is what *fine tuning can someone else do*
  means.
- **`ui_scale`'s persistence is now the only calibration path there is**, so D-186 —
  where an end user's choice is written down — matters more than it did this morning
  and should be answered with the settings panel rather than after it.
- **Three ADRs in one day on one number, two of them superseded.** The record says so
  in each of them, and the honest summary is that the mechanism was right from the
  second one and the policy took three attempts. **Nothing was built against any of
  them**, which is what a planning root is for.
- **The consequence I do not like:** the engine now has no idea how big anything
  physically is, so it cannot warn anybody about anything — a hairline that
  disappears on one machine and not another has no explanation available to it
  (D-137). We chose a unit called a millimetre partly to make that question sayable,
  and it is now sayable only by the person looking at the screen.
- **Card 042's planning is not wasted.** What each platform can and cannot report,
  and the awareness coupling, are recorded in D-187 through D-189 — so the day
  somebody wants it, the survey exists and no one repeats it.

## Rejected options and why

**Reading the display (ADR-0103).** Correct in principle, and it costs per-platform
code whose asymmetries were already visible before implementation. The principal
declined the surface; the reasoning is his and it is consistent with how this project
has treated every other optional integration.

**Nominal millimetres at 96 per inch** — ADR-0103's amendment. That definition only
earns its keep when there is a system scale to multiply it by; with no display read
at all, a fixed reference density is a number pretending to mean something.

## Questions this opens

- **D-190 — whether the unit keeps the name *millimetre*** now that it is
  window-relative rather than physical. ADR-0089's argument for the name was that a
  physical unit makes a physical question askable, and that property is gone. Cheap
  to change until programs author panels; expensive immediately after. Trigger: card
  037, the first real interface, or the first outside program.
- **D-186 promoted** — where an end user's `ui_scale` is persisted, now that it is
  the only calibration in the engine.
