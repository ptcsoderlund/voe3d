# 0169 The palette is derived in OKLab from one colour and two numbers, and dark mode clamps chroma

Status: accepted
Date: 2026-09-17

ADR-0097 fixed the authored set at an accent, `contrast_strength`, `surface_separation` and a mode,
and left two things to the theme card: which roles come out of it, and whether the derivation
clamps chroma harder in dark mode (D-167). Spec 006 is that card, and its criterion 4 — the mode
flips the whole editor and stays readable, the accent recolours everything, the sliders visibly
move surfaces and text apart — is a claim about the derivation that a test has to be able to make.

## Decision

**The derivation runs in OKLab and nowhere else.** Authored values are sRGB as a person wrote them;
the transfer function and the OKLab conversion are the derivation's first and last steps, and what
reaches an element record is linear (ADR-0069). No step is arithmetic on sRGB channels.

**Lightness carries the palette; hue is never rotated.** Surfaces are lightness steps from the
mode's ground, sized by `surface_separation`; text and border lightnesses are steps from the
surface they sit on, sized by `contrast_strength`; a scalar of 1.0 is the reference look, and both
are clamped to a range in which the result stays legible. Contrast is a constraint the derivation
satisfies, not a colour it emits (ADR-0097): every text role is stepped until it clears its
surface, so no pair of authored inputs produces unreadable text.

**Dark mode clamps the accent's chroma harder than light mode does, and that answers D-167.** The
bench's finding is a property of a saturated colour on a dark ground, and the accent is the one
colour a theme authors, so the clamp belongs to it. A test pins the asymmetry.

**The roles are what the widgets draw with, and no more** (rule 10): a ground, a surface, a raised
surface, a hairline border, a control and its hovered state, three text lightnesses, the accent,
and the ink that goes on the accent. A pressed control and a number box being dragged are the
accent, which is what makes ADR-0088's Windows 10 look read. There are no semantic colours
(ADR-0097).

**A hairline border is drawn by the widget, not by the shader** — the bordered rectangle is the
border colour with the fill inset by a hairline, so a panel and a button cost two element records
instead of one. Nothing in `render` changes for it.

## Rejected

- **Deriving in linear or sRGB RGB** — ADR-0087 rejected it with the argument that still holds: a
  ten-percent step is a different size per hue and the palette looks derived.
- **CIELAB** — OKLab's predecessor, worse hue linearity in blue, and no cheaper.
- **Rotating hue for the hovered and pressed states** — a state that changes hue reads as a
  different control; lightness and the accent already say it.
- **A fixed contrast ratio as an input** — one number a person cannot picture, in place of two they
  can move and look at.
- **A border in the element record, drawn by the shader** — it adds a field only one widget reads
  and puts an interface concern in `render`.

## Consequences

- Two of `ui`'s tests become tests about colour: that light mode stays legible at both ends of both
  scalars, and that the accent's chroma is clamped harder in dark than in light.
- A theme's look depends on a conversion written here, so a mistake in OKLab is a mistake in every
  interface at once. It is about a hundred lines with a test per step.
- Panels and buttons cost one more element record each, and a context's capacities move with it.
- Someone who wants a flat rectangle with no border has no role for it; the honest fix is a role
  when something asks.
