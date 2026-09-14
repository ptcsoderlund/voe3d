# 0096. The theme is an authored file in the sectioned format, and it needs no type system

- **Status:** Accepted
- **Date:** 2026-09-07
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-158

## Context

The principal, after the bench:

> Maybe we can parameterize contrast strenght and surface separation as well in our
> theme when we do that. I would like to use our ini-ish format which i dropped an
> exempel file in source assets folder "Custom_Text_Base_syntax.txt"

He had already written the section into that sketch in the same edit:

```
[Theme1]
accent="#2B2B2B"
contrast_strength=1.25
surface_separation=1.25
error_light="#B836BA"
error_dark="#D8A0D9"
```

Constraints already fixed:

- **ADR-0073 — authored data is a sectioned line format**, `[Section]` headers with
  flat `key = value` lines, chosen because an agent editing a file touches one line
  and a bad edit stays local. **Direction only; the grammar is unwritten and was
  declared blocked behind D-024.**
- **D-024 — what type description a component carries** so it can be written and
  read back as text. ADR-0073 promoted it to *the* blocker on the format, because
  the sketch carries types in the syntax (`400.12345d`) and those sigils *"only
  exist because the struct's own description is not yet reachable."*
- **ADR-0093 — `ui` is a leaf** on `render`, `text`, `math`, `base`. Not `assets`,
  not `platform`.
- **`assets` is readers, not data**, and it owns parsing.
- **Rule 14 — do not recurse over data read from a file.**
- **ADR-0087 / ADR-0094** — a theme is a few authored colours plus a derivation;
  semantic colours are authored per mode.

## The finding: a theme does not need D-024, and that unblocks the format

**D-024 blocks the format only for readers that do not know what they are reading.**
A component written generically must carry its own type description, because
nothing on the reading side knows whether `hp=20.0` is a float or a double — hence
the sigils in the sketch.

**A theme has a schema that is known at compile time.** Whoever reads
`contrast_strength` knows it is a number, because the code that consumes it was
written by hand against a fixed set of roles. **No sigil is required and no type
system is needed.**

So the format has three layers, and only the third is blocked:

1. **The parser** — sections, keys, and values as *text*. It needs no type
   knowledge at all, because it does not interpret anything. **Unblocked today.**
2. **A consumer with a known schema** — the theme, and later a project file or a
   settings file. It converts the text it asked for. **Unblocked today.**
3. **A generic consumer** — reading arbitrary components back into an ECS without
   knowing them in advance. **This is what D-024 blocks, and only this.**

**That decomposition is what this ADR is really for.** ADR-0073 declared the whole
format blocked; it is not. The theme can be the format's first real consumer, which
is a much better first test than a component would have been — small, flat, and
written by a person, which is exactly what the format was chosen for.

## Decision

1. **A theme is an authored file in the sectioned format**, one `[Name]` section per
   theme, with the principal's sketch as its shape.
2. **The two scalars are authored parameters**, not constants: `contrast_strength`
   and `surface_separation`. The bench earned them — the principal moved both off
   1.00 — and a theme that cannot set them would have thrown that away.
3. **The parser lives in `assets`** and returns sections, keys and **values as
   text**. It interprets nothing. Whether it strips quotes is lexical and its own
   business; what a value *means* is the consumer's.
4. **`ui` never reads a file.** ADR-0093 makes it a leaf on `render`, `text`,
   `math` and `base`, and that stands. **The caller loads the theme and hands `ui`
   the authored values**; `ui` derives the palette from them. This is the same shape
   as the font and the pointer — `ui` is given things.
5. **D-024 is not a blocker on any of this** and is not touched. Generic component
   round-tripping still needs it.

## What reading the sketch turned up, worth keeping

- **Comments are `//`, not `#`, and that is the right call for a reason worth
  recording.** `#` is INI's and TOML's comment character — and **an unquoted hex
  colour starts with `#`**. The sketch quotes its colours so it would have survived
  either way, but `//` removes a trap that a theme file is unusually likely to
  spring. Keep it.
- **Nesting is a dotted section name** — `[Player1.Stats]`, `[My_Unique_Name.position]`
  — not nested brackets. Two levels, no recursion, which is exactly the property
  ADR-0073 claimed made this parser cheaper than the JSON one, and it satisfies rule
  14 by construction rather than by a depth limit.
- **A section names its type** (`type="Node"`, `type="Transform"`). That is the D-024
  hook and a theme does not use it.

## Blast radius

**Cheap now, and it is the format's first real use.** Nothing is built. What this
fixes is a belief — that the format was blocked — which was costing more than it
looked, because it made every authored-data decision feel unreachable.

What becomes expensive later: the theme file's key names, once themes exist in the
wild. They are ordinary text in a file people edit, so renaming one breaks their
files silently.

Reversibility: **cheap.**

## Consequences

- **A parser card can be written now**, in `assets`, independent of every GUI card.
  It is card 038 and it is the only thing between here and a theme file.
- **ADR-0073's *blocked behind D-024* is corrected in scope**, not overturned: the
  direction stands and the grammar is still unwritten, but only the generic layer
  was ever blocked.
- **The theme is a much better first consumer than a component would have been** —
  five keys, one section, hand-written, and its failure mode is visible the instant
  you look at the screen.
- **D-047, the nesting depth `assets` refuses past, is answered for this format by
  its shape** rather than by a number, since a two-level dotted name cannot nest.
  The row stays open for JSON, which can.
- **The consequence I do not like:** the parser will be written against one
  consumer's needs and will look complete long before it is. The type sigils in the
  sketch are real and unbuilt, and a later card discovering the parser cannot
  express them is a predictable disappointment. The parser card should say out loud
  which layer it is building.

## The discrepancy this raises, unresolved

**The sketch's error colours are not the ones the principal gave from the bench.**
The file says `#B836BA` light and `#D8A0D9` dark; the bench session gave `#E60073`
light and `#E495BD` dark. Both pairs are internally consistent — darker for light
mode, lighter for dark — so it is a choice rather than a slip.

**ADR-0094 carries the bench pair as the default and this is put to the principal
rather than resolved here.** Whichever wins, ADR-0094 is amended, not superseded:
the decision it takes is *authored per mode, and not red*, and both pairs satisfy
it.

## Rejected options and why

**Waiting for D-024.** What ADR-0073 implied and what this ADR undoes. The theme
needs no type description, so waiting would have blocked a ready thing on an
unrelated one.

**`ui` reads its own theme file.** Convenient and it breaks ADR-0093's leaf
property, pulling `assets` and transitively `platform` into a folder that is
deliberately testable without either.

**A theme-specific reader rather than a general parser.** Faster to write and it
wastes the one chance to test the format against a real consumer, which is most of
why the theme is a good first one.

## Questions this opens

- **D-159 — the parser's exact grammar**: quoting, escapes, whitespace around `=`,
  arrays, whether deeper than two levels is refused or accepted. Trigger: card 038.
- **D-160 — where a theme file lives and how a program names one.** ADR-0064 put no
  settings file in the engine and D-080 already asks the adjacent question about
  persisted settings. Trigger: the theme card.
