# 0097. The default theme is monochrome plus one accent, and there are no semantic colours yet

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** ADR-0094
- **Superseded by:** —
- **Amended:** 2026-09-07, same session — the synthetic-bold question is closed. Filled emphasis was judged sufficient and no bold is added.
- **Closes:** D-165

## Context

Hours after ADR-0094 fixed a default error colour, the principal asked the question
that should have been asked first:

> Why do we need error color though? Why not just do monochrome. Devs probably want
> to do their own colors on alot of things anyway. We can just use bold for
> highlighting in dev logging window?

**He is right, and ADR-0094 was premature.** Nothing in the planned stack errors.
The first pass of widgets is panel, label, button, checkbox, slider and scroll area
(the stack document, agreed the same day); none of them has an error state. Card
037's settings panel shows a value *clamped to the hardware tier*, which is
information rather than a failure.

So the error colour was **surface with no caller** — which engine rule 10 forbids by
name (*implement on demand, no "in case"*) and ADR-0055 forbids again (*nothing is
built that nothing measures*). ADR-0094 spent two authored values, opened four
register rows and imported a localisation question, all for a role nothing uses.

**The style target already said so and it was not noticed.** Windows 10's interface
is greys and one accent; red appears in a handful of dialogs and nowhere else. A
monochrome-plus-accent default is not a reduction of ADR-0088's target — it *is*
that target.

## Decision

**The default theme is monochrome plus one accent. There are no semantic colours.**

The authored set is now:

| input | kind |
|---|---|
| accent | one colour |
| `contrast_strength` | scalar |
| `surface_separation` | scalar |
| mode | light or dark |

**One colour and two numbers.** That is fewer than the principal's opening guess of
*something like 4 colors*, and it lands there after a detour rather than by
shrinking the target.

**Semantic colours return when something actually errors**, authored by whoever
needs them. The findings that were attached to them survive and are recorded below,
because they were never really about semantics.

## What survives from ADR-0094, and it is the better half

**1. The chroma asymmetry is a property of the derivation, not of error colours.**

ADR-0094 found that a saturated colour on a dark ground fringes, while the same
saturation on a light ground is comfortable — and that the principal's own hand-
picked colours showed exactly that ratio, roughly 0.22 chroma for light against 0.10
for dark, across two sessions with no theory in hand.

**That applies to the accent.** A developer who picks a saturated accent and runs
dark mode hits the same effect, and the accent is a colour this engine definitely
has. So **the derivation should clamp chroma harder in dark mode than in light**,
for the accent, and that is now a property of the derivation rather than a fact
about a role nothing uses. It is the most useful thing to come out of the bench and
it nearly got filed under the wrong heading.

**2. Contrast is a constraint the derivation satisfies, not a colour it emits.**
Unchanged and still the bench's real output.

**3. Semantic colours, when they come, are authored per mode.** Not derived from one
value, because no lightness rule connects the pair a person picks — and now also
because the right *chroma* differs per mode. Kept as a note for whoever adds them.

## On "bold for highlighting" — not available, and cheap to make available

**`text` has no bold.** Its header is explicit: *"ONE WEIGHT, ONE FONT, NO FALLBACK.
There is no bold, no italic and no second face."* ADR-0062 embedded one static
weight of Oxanium. So the suggestion cannot be taken as written today.

**But it is nearly free, and the mechanism is already in the shader.** The glyph
sheet is a distance field and `render/shaders/draw.slang:475` reads:

```
float field_alpha = step(0.5, field);
```

**Lowering that threshold thickens every glyph.** A synthetic bold costs one uniform,
adds no font file, and keeps the hard edge ADR-0078 and ADR-0079 insisted on — the
cut stays a cut, it just falls further out. It is not a real bold cut by a type
designer and at small sizes it will blunt letterforms, which is the honest limit.

**Decided the same session, and the answer is no.** The principal, having looked at
the monochrome specimen: *"Filled was a great idea. It looks really good now in
palette-bench. No need for bold."*

**So there is no bold, synthetic or otherwise, and `text` stays one weight.** That
matters beyond this decision: emphasis was the one plausible reason to pull a second
font file into the engine, and it is now closed. ADR-0062's single embedded weight
holds without an exception.

**Filled is also the cheaper mechanism at the renderer**, which is worth recording
because it was not the reason for choosing it. A filled chip is one solid element
plus its glyph elements — ordinary work for ADR-0092's element buffer, needing
nothing new. A threshold-shifted bold would have added a field to the element record
that only text ever reads.

**The mechanism stays written down** (D-166 keeps its description) so that if
something later needs emphasis where neither a lightness step nor a fill will serve
— small text is the likely case — the option is recoverable rather than rediscovered.

**What is available for emphasis today, with no new work:** the three text
lightnesses the derivation already produces (primary, secondary, disabled), the
accent itself, a filled background, and a rule. That is what a terminal had before
colour, and it is enough for a logging window.

## Blast radius

**Cheap, and cheaper than not doing it.** No code exists. This removes two authored
values from a format that is not written and deletes machinery before it was built.

What it costs later: when something does error, whoever adds the semantic role pays
for it then, with a caller in front of them — which is the correct time and the
whole point of rule 10.

Reversibility: **cheap.**

## Consequences

- **The theme's authored set shrinks to one colour and two numbers**, which serves
  ADR-0088's real target — a theme somebody writes in a minute.
- **Four register rows close or go dormant** without being answered: warning and
  success (D-152), purple sitting near the accent (D-164), which error pair
  (D-161, already closed), and the East Asian gain/loss inversion (D-163, which
  stays open on its own merits because it is about charts, not errors).
- **ADR-0094 is superseded, not edited.** Its reasoning about per-mode authoring and
  about the colour bleed is still correct and still worth reading; only its decision
  to fix a default error colour was wrong.
- **The bench is updated** to the three authored inputs, with the specimen showing
  monochrome emphasis instead of a coloured banner — so it tests the claim this ADR
  makes rather than the one it replaced.
- **The consequence I do not like:** the engine will eventually need semantic
  colours, and the work to choose them well — which the bench made cheap — is now
  deferred to a moment when somebody is busy building the thing that errors. The
  findings above are the mitigation and they are only a note.

## Rejected options and why

**Keeping the error colour because it will be needed eventually.** The textbook
form of the thing rule 10 exists to prevent, and this ADR is the correction rather
than the exception.

**Keeping semantic colours but deriving them.** ADR-0087 already rejected that — a
red derived from a blue theme is a blue-grey — and it is moot once there are none.

**Monochrome with no accent at all.** Not asked for, and it would remove the one
colour that carries selection, focus and active state, which is what makes a
Windows 10 interface legible at a glance.

## Questions this opens

- **D-166 — whether a synthetic bold is added**, by shifting the distance-field
  threshold. Cheap, no font file, blunts letterforms at small sizes. Trigger: the
  first thing wanting emphasis a lightness step cannot give — a logging window is
  the likely one.
- **D-167 — whether the derivation clamps chroma harder in dark mode.** The bench's
  finding, now aimed at the accent where it belongs. Trigger: the theme card, or the
  first saturated accent in dark mode.
