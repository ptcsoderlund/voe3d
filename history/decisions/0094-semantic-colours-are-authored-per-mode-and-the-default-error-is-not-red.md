# 0094. A semantic colour is authored once per mode, and the engine's default error colour is not red

- **Status:** Superseded by 0097
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** ADR-0097, 2026-09-07 — the same day. The principal asked why the engine needs an error colour at all when nothing in the planned stack errors, and he was right: this ADR fixed a default for a role with no caller, which rule 10 forbids. **Its reasoning survives and is worth reading** — per-mode authoring, the colour bleed, and the chroma asymmetry are all still correct, and ADR-0097 carries them forward. Only the decision to fix an error colour now was wrong.
- **Amended:** 2026-09-07, same session — the hue family moved from rose to purple and the values were computed rather than picked. The decision itself is unchanged.
- **Amends:** ADR-0087 (semantic colours: one authored value becomes one per mode)

## Context

The palette bench ran and the principal came back with settings and one finding
that is not a setting:

> I think Near-black would suit us, at least the editor gui. 1.25 on contrast and
> same on surface separation. I am sensitive to red, i get color bleed no matter
> what i choose. This is good for me in dark theme #E495BD and this is good on
> light theme (near-black) #E60073.

Two separate things came out of that sentence, and only the first is a preference.

**The settings** — near-black accent, contrast strength 1.25, surface separation
1.25 — are a partial answer to D-129 and are recorded there.

**The finding** is that **red does not work**, and the two colours offered instead
are not red: `#E60073` and `#E495BD` are both in the rose/magenta family, around
330–335° of hue. And crucially **they are not one colour adjusted for two modes** —
they are two different authored colours, one per mode. ADR-0087 assumed a semantic
colour was authored once. It is not enough.

Constraints already fixed:

- **ADR-0087** — a theme is a few authored colours plus a derivation, composing
  nearest-wins; **semantic colours are authored, never derived**, because a red
  derived from a blue theme is a blue-grey.
- **ADR-0088** — the style target is Windows 10, which uses red for error.
- The bench derives a mode-appropriate variant of the authored semantic colour by
  moving its lightness. **That is what turns out to be insufficient.**

## On the colour bleed, factually

Saturated red on a dark ground is a known and ordinary perceptual problem, not an
unusual one. Long-wavelength light focuses slightly differently from short, so a
strongly saturated red against a dark ground can read as sitting at a different
depth from the surface it is on, with a soft fringe at its edges. It is more
pronounced for some people than others and it is not correctable by choosing a
different red — which is exactly what the principal reports.

**So this is a general default worth taking, not an accommodation bolted on.** Rose
and magenta keep the *meaning* of red — the alarm reads the same — while sitting at
a hue where the effect largely disappears. The engine loses nothing by defaulting
there.

Worth stating plainly, because ADR-0091 just accepted having no screen readers:
**that decision was about structural accessibility for a games engine, and it does
not extend to colour.** A default palette nobody can read is a defect in the
palette, and this one is now better than it was.

## Decision

1. **A semantic colour is authored once per mode**, not once and adjusted. A theme
   carries a light value and a dark value for each semantic role. The derivation
   does not move them.
2. **The engine's default error colour is not red.** It is rose/magenta, with the
   principal's values as the defaults: **`#E60073` in light, `#E495BD` in dark.**
3. **A program may still author red** if it wants it. This is a default, not a
   prohibition, and the theme format has to allow anything.
4. **This is the exception, not the pattern.** Everything else stays derived and
   mode-aware from one value. **Only roles the derivation cannot reach get two**,
   and today that is the semantic set — error, and whatever warning and success
   turn out to be.

## Amendment, same session: purple, and the values are computed

The principal, asked which of his two pairs should be the default:

> I dont know, everything bleeds for me now. I have stared too long on them.
> Calculate what is appropriate against the background. Lets use purple instead of
> red (isnt red good in korea and china?).

**The values are therefore derived from the grounds rather than picked by eye**, and
the hue family is purple. Both authored values change; nothing else in this ADR
does.

### The defaults

| | hex | L | C | H | worst contrast on any surface |
|---|---|---|---|---|---|
| `error_light` | **`#6A00A9`** | 0.42 | 0.218 | 305° | 7.1 : 1 |
| `error_dark` | **`#D3ADFF`** | 0.81 | 0.120 | 305° | 9.2 : 1 |

Same hue in both modes; they differ in lightness and in **chroma**, and the chroma
difference is the substantive part.

### What his own picks revealed, and it is the best evidence in this ADR

Measuring the four colours he had chosen by eye, across both his pairs:

| | chroma |
|---|---|
| light mode picks (`#B836BA`, `#E60073`) | 0.220, 0.241 |
| dark mode picks (`#D8A0D9`, `#E495BD`) | 0.100, 0.107 |

**He picked roughly twice the chroma for light mode as for dark, consistently,
across two independent sessions, without reference to any theory.** That is exactly
what the physics predicts: the fringing is a *saturated colour on a dark ground*
effect, and on a light ground the same saturation is comfortable.

So the derivation above matches his revealed preference rather than overriding it —
0.218 for light, 0.120 for dark — and simply buys contrast headroom he was not
getting.

**It also justifies per-mode authoring a second time.** The original reason was that
no lightness rule connects the two values. The stronger reason is that **the right
chroma is different per mode**, which no derivation from a single value would ever
have produced.

### One of his picks was not legible

`#E60073` — the light-mode value from the bench session — measures **4.25 : 1**
against the light surface, below the 4.5 : 1 needed for text. It would have been a
slightly unreadable error message. `#B836BA` passes at 4.59 : 1, barely. Both
replacements clear it with room.

### On red in China and Korea — half right, and it does not carry the argument

**True:** red is auspicious there — prosperity, celebration — and there is a real,
practical inversion in software: **in Chinese, Japanese and Korean markets red means
price *up* and green means *down*, the opposite of the Western convention.** That is
a genuine localisation trap.

**But it is not this trap.** For *error and danger states*, red is the convention in
CJK software too; the Western mapping was adopted broadly there. So the cultural
point does not argue for moving away from red for errors.

And purple is not culturally neutral either — it is a mourning colour in Thailand
and in parts of Latin America. **Swapping does not buy neutrality; nothing does.**

**So the argument for purple is the eye argument, and it does not need help.** It is
sufficient on its own and it is much better evidenced. Recorded this way so nobody
later finds a decision resting on a reason that does not hold.

**The real trap gets its own row (D-163):** red/green for gain and loss is inverted
in East Asia, and that would bite a chart or a numeric readout, not an error banner.

### A constraint purple brings that red did not

Red sits far from most accent colours. **Purple does not** — a program whose accent
is purple now has an error colour in the same family, and *this is the alarm* stops
reading. With the near-black accent it is a non-issue. It is a real limit of the
default and it belongs in the theme card's notes.

## Blast radius

**Cheap.** No code exists. It changes the theme's authored shape from *N colours*
to *N colours, semantics doubled*, which is a field-level change to a format that
is not written.

The thing it makes slightly more expensive: every future semantic role costs two
authored values rather than one, so the *authoring speed* target ADR-0088 set is
very slightly worse. Two colours for error is still a theme somebody writes in a
minute.

Reversibility: **cheap.**

## Consequences

- **ADR-0087's "semantic colours are authored" gets sharper**: authored *per mode*.
  The reason is the same one that ADR gives — a derivation cannot reach these — and
  it turns out to be true across modes as well as across hues.
- **The bench is updated** to take an error colour per mode and to load these
  values, so what is being judged from here on is what is being decided.
- **Warning and success are not chosen yet** and should be picked the same way,
  against the same eyes, rather than assumed from convention. Amber and green have
  no known problem here, but nobody has looked.
- **The default palette is now shaped by one person's vision.** That is honest and
  it is also fine: it is a *default*, every value is authored and replaceable, and
  a default that one real person verified beats one nobody looked at.
- **The consequence I do not like:** the engine now ships a Windows 10-styled
  interface whose error colour is not the one Windows uses. Anyone matching the
  target closely will notice, and they will change it, which is a one-line change
  and exactly what a theme is for.

## Rejected options and why

**Keep one authored value and derive the mode variant.** What the bench does today
and what ADR-0087 assumed. Rejected on the evidence: the two colours the principal
picked are not a lightness step apart — `#E60073` is vivid and `#E495BD` is soft
and much lighter — so no lightness rule produces one from the other.

**Keep red and let anyone who needs to change it.** Rejected because it makes the
out-of-the-box experience worse for a known, ordinary perceptual effect, in
exchange for matching a reference we are deliberately not cloning.

**Treat this as one person's setting rather than a default.** Rejected for the same
reason: the effect is general, and a default nobody had looked at is not obviously
better than one somebody has.

## Questions this opens

- **D-152 — warning and success**, chosen the same way and against the same eyes.
  Trigger: the theme card.
- **D-153 — whether any other role needs two authored values.** The rule as written
  says only the ones a derivation cannot reach; nothing else qualifies yet.
  Trigger: the second role that does.
