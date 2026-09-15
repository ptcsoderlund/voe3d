# 0141. The editor's interface is world geometry, not pixels, and its camera is replaceable

- **Status:** Accepted
- **Date:** 2026-09-12
- **Deciders:** Human, Tech Lead
- **Supersedes:** — (closes D-178 by rejecting its premise; narrows D-180)
- **Superseded by:** —

## Context

The principal asked for a dock layout: a scene tree like Godot's, an inspector, panels the
developer rearranges as they like, and panels that tear off into their own window. The tech
lead said that a dock layout with draggable splitters needs **pixel-exact panels**, because
two panels sharing a boundary must not show a seam — and so reopened D-178, parked since
2026-09-09 against exactly this trigger.

The principal rejected the premise:

> We dont want pixel perfect. Someone might say "hey, i want the editor in VR". We should be
> able to respond "Put it on and do a plugin for camera distance, use vr controller as mouse
> cursor and put a keyboard in your lap and you are good to go. But you have to build a
> dedicated scene view for holographic rendering yourself".

**ADR-0089 had already answered this and the tech lead had not noticed.** That ADR names the
GUI unit a millimetre rather than a pixel specifically so nobody expects screen-pixel
behaviour, and its own *what this honestly does not solve* section says it does not make a
hairline crisp at every distance and nothing can. D-178 was an attempt to claw that
expectation back for one program.

Constraints already fixed, which make the principal's answer nearly true today:

- **ADR-0049** — everything is content in 3D space; there is no screen-space path.
- **ADR-0074** — the overlay layer is above the world and still in perspective.
- **ADR-0093** — a panel is a component carrying a transform and a layer, and `ui` is kept
  from reading input: it is *given* a pointer as plain values. A panel on a wall and a panel
  in front of your face differ only in their matrix.
- **ADR-0103** — the pixels-per-millimetre scale is an input `render` is handed, and the
  default policy is physical, from the display. A millimetre on screen is a millimetre.
- **ADR-0089** — a GUI is authored in millimetres and the unit is deliberately not a pixel.
- **ADR-0057** — the editor is a C program built on the engine.

What is *not* true today: card 058 as written computes the interface's pointer by dividing
window pixels by a scale, copying `dev/src/interface.c`. That one line is the place the
interface learns it is flat.

## Options considered

### Option A — pixel-exact editor panels
Snap panel geometry to the target's pixel grid, most plausibly through an orthographic camera
whose view height is the target's pixel height (the mechanism D-180 describes). Panel edges
meet exactly at every window size, hairlines stay one pixel, text is crisp always. Costs: an
orthographic pixel mode in the camera; a second coordinate convention for the interface
beside millimetres; and it makes the editor's interface flat **by construction** — there is
no camera you can substitute, because the geometry is defined in terms of the screen.

### Option B — the interface is world geometry at millimetre scale, with no snapping
Panels are real surfaces at a transform, seen through whatever camera is looking at them.
There is a distance at which a millimetre of panel lands on a millimetre of display and the
interface is crisp; it is where the editor opens and nothing depends on it. Off that
distance text softens or breaks up, since there is no anti-aliasing (ADR-0078) and nothing
minifies gracefully (D-137). Substituting the camera and the pointer source is all a VR
build of the *interface* needs. A holographic **scene view** is not covered and is said not
to be.

### Option C — millimetres, plus an opt-in pixel-snapped mode for the editor
Both. Two conventions through layout, hit-testing and emission, and the snapped path becomes
the one everything is written and tested against while the geometric one rots.

## Decision

**Option B.** The deciding factor is that Option A buys crispness by defining the interface
in terms of the screen, and the principal's sentence is the test it fails: an interface made
of pixels cannot be put in a headset at all, at any price.

Eight points.

1. **Nothing in this engine is pixel-exact, and the editor gets no exception.** D-178 is
   closed by rejecting its premise rather than by answering it.
2. **Panels are surfaces at a transform.** Adjacent panels share a computed millimetre edge
   because `ui`'s layout tiles a content box — they are adjacent **by construction**, not by
   two numbers that might round apart. A visible seam between two panels is a layout bug, not
   a rasterisation one, and snapping would hide it rather than fix it.
3. **There is a distance at which a millimetre is a millimetre**, falling out of ADR-0103's
   display scale and the panel's matrix. The editor opens there. It is a default and a
   comfort, **never a guarantee, and no code may assume it.**
4. **The interface is handed its pointer in the panel's own millimetres and never computes it
   from window pixels.** The editor's `main.c` does that conversion for a panel that fills the
   window and faces the camera; a VR build replaces that one function. **This point is what
   makes point 1 keepable** — without it the interface knows it is flat and point 1 is a
   sentiment.
5. **A panel's transform is the editor's to choose.** By default the editor's panels are
   locked to its camera, so orbiting the scene does not swing the interface around. Pinning
   them in the room instead is a different matrix and nothing else.
6. **The ray-against-a-plane pointer is not built now.** Every editor panel faces the camera
   today, so the division is correct and an intersection would be generality with no caller.
   It is written by the first card that places a panel not facing the camera, and point 4 is
   what keeps that card small.
7. **The scene view is outside this promise, and the editor says so in its own documentation.**
   An interface that is geometry costs nothing to put in a headset. A scene view is a camera's
   picture of a world, and a holographic one is two eyes, a projection per eye and a different
   idea of what *the view* even is. Whoever wants the editor in VR writes their own scene
   view; the rest of the editor follows them in unchanged.
8. **This ADR creates no plugin system.** The principal's word was *plugin*; what makes his
   answer true today is ADR-0057 — the editor is a C program, and the answer is *change the
   program*. A plugin boundary is a decision taken when there is a second party writing
   against it, and there is not one. Building an extension API for zero extensions is the
   premature generality this project refuses elsewhere.

## Blast radius

Points 3 to 6 are the editor's own code and cost nothing to revisit. Point 1 forecloses
nothing at the engine level: Option A's orthographic pixel-height camera is still a mechanism
we can add — D-180 keeps it alive for a 2D game that wants texel-exact sprites — so a future
decision to give the editor a snapped mode would be additive, not a reversal.

What becomes expensive is **point 4 if it is skipped**: letting the interface compute its own
pointer from the window spreads that assumption through every panel and every widget's hit
test, and undoing it later is a sweep rather than one function. That is the whole reason it is
written as a rule today, while there is exactly one caller.

**Reversibility: cheap.**

## Consequences

- **Editor text is crisp at the opening distance and softens away from it.** With no
  anti-aliasing and no graceful minification, a scaled-down interface looks worse than a
  scaled-up one. This is the bill for the decision and it is accepted knowingly.
- **A one-millimetre border is a hairline that can vanish.** The editor must not separate
  panels with hairlines; it separates them with gaps and backgrounds, which a dock layout
  wants anyway.
- **No pixel-exact mode means no 1:1 texture inspector and no pixel-art tooling** in the
  editor without a later decision. Named so that nobody is surprised.
- **The editor will look like a game's interface, not like a native desktop application.**
  That is what it is.
- The VR claim is now something the project can be held to for the interface, and explicitly
  cannot be held to for the scene view. Both halves are deliberate.

## Rejected options and why

**Option A** — it achieves crispness by defining the interface in terms of the screen, which
is the one property that cannot survive a camera change, and it adds a second coordinate
convention to carry beside millimetres. It also hides layout bugs instead of exposing them
(point 2).

**Option C** — two conventions is worse than either one. The snapped path would become the
tested path and the geometric one would rot, and we would discover that the day somebody
tried the headset.

## Questions this opens

- **D-178 closes** — deleted from `parked-questions-by-condition.md`. Its premise is rejected.
- **D-180 narrows** to its 2D-sprite half; the editor-viewport half is gone. It stays parked
  on *the first 2D game that wants texel-exact sprites*.
- **New: what a scene view is**, given point 7 says it is not a picture this ADR promises —
  and in particular whether a flat editor's scene view is a **hole in the panel layout**, the
  world showing through where no panel was drawn, rather than a camera rendered into a
  texture. Consumed by the first editor rendering card.
