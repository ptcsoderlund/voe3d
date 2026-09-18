# 0087. Themes are derived from a few authored colours and compose nearest-wins — direction, and a spike

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-128

## Context

The principal stated a second destination in the same breath as ADR-0086's:

> I also want us to have composable themes. Meaning we have a simple theme which
> is something like 4 colors (we need to test our way) and then calculates the
> rest of the colors. Then we can do overrides for a control which affects all
> children. So we can combine multiple themes, since its only 4 colors or so its
> simple to create a new one and add to a control (recursively). Look at something
> like jetbrains rider, its too complex to do a custom theme. Godot is also
> borderline, its too much hassle to create a general theme.

**This is entirely new ground.** No ADR touches theming, the `ui` folder does not
exist, and card 023 is a scope sketch nobody has cut into cards. So this is
direction in ADR-0073's exact sense: taken now because it changes how a later
decision is *designed*, and taken as direction only — no format, no API, no card.

The named failure mode is the useful part of the brief. Rider's theming is a
large structured document; Godot's is a tree of per-control style resources. Both
are *complete* and both are too much work to author, so in practice almost nobody
makes a theme and everyone ships the default. **The target is not expressiveness.
It is that a person can make a new theme in a minute.**

Constraints already fixed:

- **ADR-0023 — write it ourselves.** No colour library. The maths involved is tens
  of lines.
- **Colour crossing the render boundary is linear**, and sRGB lives only at the two
  ends, inside the GPU (`render/include/render/device.h`). A theme's authored
  colours are what a person picked; what reaches a shader is linear.
- **ADR-0073 — authored data is a sectioned line format**, `[Section]` headers with
  flat `key = value` lines, chosen because an agent editing a file touches one line
  and a bad edit stays local. A theme is authored data and is exactly that shape.
  Its grammar is still blocked behind D-024.
- **ADR-0083 — a theme produces things to draw, so it is a module**, not a switch.
  No conflict, and nothing in the core owes it anything.
- **ADR-0034 / ADR-0055** — on demand, and nothing built that nothing measures.

## Direction

1. **A theme is a few authored colours plus a derivation.** The authored part is
   small enough to write by hand in a minute; everything else — hover, pressed,
   disabled, borders, elevated surfaces, the text colour that sits on each of them
   — is computed. The *count* is not fixed by this ADR; see the spike.
2. **Themes compose, and the rule is nearest-wins per role.** A theme applied to a
   control replaces the one above it for the roles it sets, and roles it does not
   set fall through to the theme above. Applying a theme to a control applies it to
   that control's whole subtree. Recommended because it is what a cascade already
   means to everyone who has met one, and because it makes *combining* themes mean
   *stacking* them, which needs no merge algorithm.
3. **Derivation happens in a perceptual colour space, not by arithmetic on RGB.**
   This is the technical heart of it and the reason the idea works at all — see
   below.
4. **Semantic colours are authored, not derived.** Error, warning and success do
   not follow from a brand palette; a red derived from a blue theme is a blue-grey.
   They are part of the small authored set or they are wrong.
5. **Nothing is built until a card needs it**, and the first thing that happens is
   a spike, not a card.

## Why the colour space is the whole trick

**"Four colours calculates the rest" works or fails on this one point**, so it goes
in the direction rather than being left to an implementation.

Lightening a colour by scaling its RGB channels changes its hue and its apparent
lightness by different amounts for different hues — the classic result is a
palette where the blues look flat, the yellows look washed out, and a "10% lighter"
step is visibly a different size depending on which colour it was applied to. That
is what makes derived palettes look cheap, and it is why the systems that do this
well do it in a perceptually uniform space (OKLCH is the current one; CIELAB is
its predecessor).

In such a space, *lighten by one step* means the same thing everywhere, and the
derivation becomes a handful of honest rules — step lightness for states, rotate
nothing, pick text colour by lightness contrast — rather than a pile of per-colour
fudges. The conversion is short arithmetic and we write it ourselves under
ADR-0023.

## The concern, stated once

**Four is probably not the number, and the reason is not that four is too few.**

Rider and Godot are complex because real interfaces need things a derivation
cannot produce: the state colours are derivable, but semantic colours are not
(point 4 above), and *contrast* is a constraint rather than an output — derived
text has to stay readable on a derived background, which is a rule the derivation
must satisfy, not a colour it can emit.

So my honest position is that the authored input is a small set of **roles**, not a
count of swatches, and it likely includes one or two scalars — how strong the
contrast steps are, how much surfaces separate — alongside the colours. That is
still a theme somebody writes in a minute, which is the actual goal, and it is
still nothing like Rider.

**This is not settleable by argument**, which the principal said first and better:
*we need to test our way*. It is the textbook case for a spike, so this ADR
commissions one instead of guessing.

## The spike

`spikes/theme-palette/` — throwaway, never promoted, per root `CLAUDE.md`.

**The named question:** *what is the smallest set of authored inputs from which a
complete, readable control palette can be derived, and does deriving in OKLCH
actually hold up across hues a person will really pick?*

- Time-boxed. It answers a question; it does not become the theme system.
- It produces **pictures to look at**, not a library. The principal decides by
  looking, because that is the only way this question is decidable.
- Success is a number and a list of roles, written into a register row. Failure —
  derivation not holding up, needing per-colour special cases — is an equally good
  outcome and kills the *calculated* half of the direction early, while it is free.

## Blast radius

**Nothing, today.** No code, no folder, no card, and the `ui` folder does not
exist. This is a destination written down so that the GUI cards are cut against it
rather than against nothing.

What it makes expensive later is the composition rule: once controls carry themes
and programs rely on the cascade, changing what *nearest-wins* means changes every
interface built on it. That is why point 2 is a recommendation written into
direction rather than left to the first card — it is cheap to fix now and not
later.

Reversibility: **cheap.**

## Consequences

- **The GUI cards gain a constraint they would otherwise have invented**: a control
  takes its colours from a cascade, not from parameters passed at each call site.
  That shapes the widget API from its first line.
- **A theme is authored data**, so it is a consumer of ADR-0073's sectioned line
  format and inherits its blocker, D-024. It is also an unusually good first test
  of that format — small, flat, and human-written, which is exactly what the format
  was chosen for.
- **The engine owes a perceptual colour conversion** in `math` or wherever the
  first caller puts it. Short, testable, and written by us.
- **The consequence I do not like:** *nearest-wins* is recommended from familiarity
  rather than from anything measured here, and it is the part with the longest
  reach. If it is wrong, it is wrong for every interface at once.
- **A second one:** this ADR asks for a spike before a card, which is the first
  time in the engine phase that has happened. That is correct for a question only
  looking can answer, and it is worth noticing that it is a departure.

## Rejected options and why

**A full style document per control, Rider's or Godot's shape.** Rejected by the
principal by name, and the reason is not that it is bad design but that nobody
authors one. A theme system whose themes do not get written has failed whatever
its expressiveness.

**Deriving everything, semantic colours included.** Rejected on point 4: it
produces a wrong red and there is no derivation that fixes it.

**Deriving in linear or sRGB RGB.** Rejected on the colour-space argument above.
It is the cheap implementation and it is why most derived palettes look derived.

**Fixing the number at four now.** Rejected as undecidable by discussion, which is
what the spike is for. Four may well be the answer.

## Questions this opens

- **D-129 — the authored set: which roles and how many**, and whether scalars sit
  beside the colours. **Answered by the spike**, not by argument.
- **D-130 — what a control's theme override attaches to**, given there is no widget
  tree yet and no decision on whether the layout model is retained or immediate
  (card 023 names that as a real decision of its own). *Recursively to children*
  presumes a tree; an immediate-mode GUI has a stack instead, and the cascade means
  something slightly different there. Trigger: the retained-versus-immediate
  decision, which must be taken first.
- **D-131 — where the perceptual colour conversion lives.** `math` is the obvious
  home and it is the folder that exists to hold arithmetic; against that, `math` is
  spelled the way Slang spells it (ADR-0035) and a colour space is not a Slang
  concept. Trigger: the first caller.
- **D-132 — whether the theme reaches the shader as computed colours per draw, or
  as an index into a palette buffer.** Bears on D-062, material instances, which is
  already open and asks the same question in a different costume.
