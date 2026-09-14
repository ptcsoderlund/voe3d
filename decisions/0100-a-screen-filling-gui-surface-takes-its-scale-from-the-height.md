# 0100. A screen-filling GUI surface takes its scale from the target's height

- **Status:** Superseded by ADR-0103
- **Date:** 2026-09-09
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** ADR-0103 — the anisotropic-stretch fix stands; the *source* of the scale moved from the window's height to the display, six hours later, when the editor case arrived
- **Closes:** D-172

## Context

The principal, having looked at the element exhibit in a resized window:

> I noticed how current gui stuff stretches when window size changes. Font scales
> with window height. I also want all gui elements to stretch by height and not
> width. So we keep ratio on them. Since we are planning some flexbox like display
> we will do newline overflow scrollbar stuff as needed when we need them.

He is describing a real defect and prescribing the fix. **What stretches, and
why:** `voe_render_element_transform(size)` maps an authored millimetre rectangle
onto the whole render target, **each axis independently** — `2/size.x` across and
`-2/size.y` down. When the window's aspect differs from the authored rectangle's,
the interface is deformed: a square is a rectangle, a circle would be an ellipse,
and every glyph is stretched with it.

Constraints already fixed:

- **ADR-0089 — the GUI is authored in millimetres, and a millimetre is physical.**
  That ADR's case is *a panel standing in the world*: it has a real size, and how
  many pixels a millimetre becomes falls out of perspective and viewing distance.
  **That case is settled and this ADR does not touch it.** What was never decided
  is the other case — a surface stretched over the whole render target, which is
  the exhibit today and every heads-up panel.
- **ADR-0091 — a runtime UI scale is a push/pop of scale**, and re-laying-out is
  free because it happens every frame anyway.
- **ADR-0095 — layout is a lighter flexbox**: a child may grow, and overflow clips
  rather than shrinking. **D-155 leaves wrapping unbuilt** until something asks.
- **Card 033, landed** — the layout root's size is *fixed or natural per axis, given
  by the caller every frame*. So a root whose width changes frame to frame is
  already something layout can express, with no change.

## Options considered

### Option A — stretch both axes to the target (what the code does today)

The authored rectangle always fills the window exactly. Nothing ever reflows and
nothing is ever cut off. The interface is deformed by any window whose aspect
differs from the authored one, and text is deformed with it.

### Option B — one scale, taken from the height; the width in millimetres follows the aspect

Pixels-per-millimetre is *target height ÷ authored height*, and **both** axes use
it. The surface is always the authored number of millimetres tall; how many
millimetres *wide* it is is computed per frame from the window's aspect. A wider
window shows more millimetres, a narrower one fewer, and the layout reflows into
whatever width it was given.

### Option C — fit the authored rectangle inside the target, with bars

One scale, taken from whichever axis binds, and the authored rectangle is always
wholly visible. Ratio is kept and nothing reflows — but a window of the wrong shape
gets dead space at two edges, and no interface ever needs to reflow, so wrapping
and scrolling would never have a caller.

## Decision

**Option B, the principal's, in his own words: *stretch by height and not width, so
we keep ratio*.**

The deciding factor is that it is the only option where the interface **responds**
to the window rather than being deformed by it (A) or padded away from it (C). It
is also the one that makes the rest of his sentence coherent: *newline overflow
scrollbar stuff as needed* only ever has a job if a surface can be too narrow for
its content, and under A and C it never can be.

### What this pins

1. **For a surface stretched over the render target, pixels-per-millimetre is the
   target's height divided by the surface's authored height in millimetres**, and
   both axes use that one number. There is no second scale.
2. **The authored height is a constant of the program; the width in millimetres is
   computed every frame** and is an *input to layout*, not a constant of it.
3. **Text scales with the same number**, so a label keeps a constant fraction of the
   window's height. That is what the principal observed happening and wants kept.
4. **A panel standing in the world is untouched.** It has a physical size in metres
   and perspective decides its pixels — ADR-0089, unchanged and unweakened.
5. **Form-factor branching is the developer's, not the engine's.** No breakpoints,
   no media queries, no per-platform layout switch inside `ui`. The principal:
   *"Its up to devs to do if ANDROID and so on. It was just an example, android is
   not a planned platform."* A root whose width is known each frame is exactly what
   makes that a plain `if` in the program's own code and needs nothing from us.
6. **Wrapping, overflow behaviour and scrollbars stay unbuilt** until a real
   interface needs them (D-155, and the scroll area on card 035). This ADR is what
   gives them a trigger; it does not pull them forward.

## Blast radius

**Cheap this week.** The only caller of the surface transform is the element
exhibit, and the one card that touches that function — 032 — is written and
unclaimed, so the change lands inside work already scheduled.

It becomes load-bearing the moment programs outside the engine author panels
against an assumed width: after that, changing where the scale comes from moves
every layout anybody has written. Which is the argument for deciding it now, six
cards before the first real interface.

Reversibility: **cheap now, load-bearing once a program authors a panel.**

## Consequences

- **A narrow window genuinely does not fit the content**, and until wrapping and
  scrolling exist it clips at the clip rectangle. That is accepted rather than
  worked around, and it is the trigger those features were waiting for.
- **The unit is physical in the world and proportional on the screen**, and that
  duality has to be said out loud in `render`'s header. One millimetre on a panel
  standing in a scene is a millimetre; one millimetre on a screen-filling surface
  is *height ÷ authored height* pixels, which equals a physical millimetre only if
  the authored height happens to match the display's. The name stays *millimetre*
  because the authoring numbers are the same in both cases and ADR-0089's argument
  for the name is unchanged — but a reader who is not told will assume a screen
  millimetre is physical, and be wrong.
- **Two surfaces with different authored heights have different pixels-per-
  millimetre**, which is correct — a smaller surface is a smaller thing — and is
  the second half of the same surprise.
- **The exhibit's own header is now wrong.** It says *"drag the window narrow and
  the rectangles stretch with it, which is what a panel filling a window does and
  not a bug."* Under this decision that is a bug, and card 032 fixes the sentence
  along with the behaviour.
- **The consequence I do not like:** the authored height becomes a number every
  program picks, and two programs picking differently makes a shared widget library
  size differently in each. That is D-173 and it is not answered here, because the
  engine has no interface to size yet.

## Rejected options and why

**Option A — stretch both axes.** It is what the code does, and the principal
looked at it and rejected it. Deforming text is the part that cannot be argued
with: the glyph work of cards 025 and 027 exists to make letters correct at any
size, and an anisotropic scale makes them incorrect at every size.

**Option C — fit with bars.** Honest and simple, and it turns the window's shape
into dead space instead of into information. It also makes wrapping, overflow and
scrolling permanently unreachable, which contradicts the direction the principal
gave in the same message.

## Questions this opens

- **D-173 — what a screen-filling surface's authored height should be**, and whether
  the engine offers a default at all. Trigger: card 037, the first real interface.
- **D-174 — whether a surface may pin its width instead**, for the case of a panel
  that must not reflow. Cheap to add as a second helper if something asks; not
  built, because nothing has. Trigger: the first interface that wants a fixed
  authoring width.
