# 0034. Implement on demand — no speculative API

- **Status:** Accepted
- **Date:** 2026-08-30
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal's instruction: *implement things when we need them, not before.
No "in case".*

The immediate trigger was the draft `math` card, written by the tech lead. It
specified `slerp`, `from_euler`, `orthographic`, `perspective_far`, `transpose`,
`vec2` and `vec4` — none of which any card, folder or decision in this project
requires. It was a maths library written because maths libraries have those
functions. That is the same failure this pre-study has rejected six times under
other names: an allocator interface with one allocator, a render abstraction
with one backend, a plugin system with no plugins.

The pre-study already carried the principle as an anti-pattern in the tech-lead
role. It was not written down as a rule that binds a card, which is why a card
violated it.

## Options considered

### Option A — On demand, strictly
A function is written when a call site for it exists in the same change or an
earlier one. Folders start close to empty and grow when something asks.

### Option B — Complete-the-obvious-set
Write the natural closure of whatever is needed: if `vec3_add` is needed, write
`vec2` and `vec4` too, and `sub` while there. The argument is that the marginal
cost is minutes and the batching avoids repeated small cards.

### Option C — Case-by-case judgement
No rule; the tech lead decides per card.

## Decision

**Option A.** Deciding factor: speculative code is code nobody has a reason to
get right, and its tests are written against imagined use, so it is the
least-trustworthy code in any repository while looking exactly like the rest.

The rule, as a card must be able to apply it:

- **A function is written when something calls it.** The caller exists in the
  same change or already. "It will obviously be needed" is not a call site.
- **A type is defined when something stores or passes it.**
- **A card that specifies unused surface is over-specified**, and saying so is
  the same act as saying a card is under-specified: report it, do not implement
  it.
- **The set is not completed for symmetry.** `vec3` without `vec2` is correct if
  nothing has a `vec2`. An incomplete-looking folder is a true report of what
  the engine currently does.
- **This binds cards, not decisions.** See below.

**What this rule does not touch: deciding early.** A decision and an
implementation are different things, and this rule is about the second. ADR-0033
fixes reverse-Z and the coordinate conventions before any code uses them, and
ADR-0024 constrains rendering to an offscreen target before anything renders,
precisely because both are free to take now and a rewrite to retrofit. Deciding
late is expensive; implementing early is waste. This ADR says nothing against
the first, and citing it to defer a decision is a misreading.

The test: **does taking it now change what already-written code looks like?**
If yes, it is a decision and it may be taken early. If it only adds code, it
waits for a caller.

## Blast radius

**Reversibility: cheap, and it is a policy rather than a structure.** Abandoning
it costs nothing already built. Its effect accumulates in what is *absent*, and
absence is free to reverse.

## Consequences

- **The build order changes, and this is the real consequence.** ADR-0022's
  first wave was `base` + `math`, justified by both depending on nothing and
  proving the build on both platforms before anything hard. **Card 001 already
  proved that**, with placeholders, on Linux and Windows. So the justification
  is spent — and under this rule `math` has no consumer at all until something
  needs a camera. `math` therefore waits, and the next card is `base`, followed
  by `platform`. The module map (ADR-0022, 0030) is unchanged; only the order in
  which folders are filled in.
- **`base` shrinks too.** The first `base` card is the assert/fatal path and the
  arena (ADR-0032), because `platform` will need memory. No dynamic array, no
  string type, no containers — nothing stores a variable number of things yet.
- **Folders will look unfinished for a long time.** Accepted, and it is honest:
  the folder reflects what the engine does today.
- **More, smaller cards.** A function arrives with its caller instead of in a
  batch. This suits the card workflow rather than fighting it, and each card
  carries a real call site, which is the thing that makes the design decisions
  in it answerable rather than aesthetic.
- **A function written to serve one call site may be shaped by it.** This is the
  genuine cost, and it is the argument Option B had. It is accepted because the
  alternative — shaping it against an imagined call site — is worse, and because
  a second caller arriving is an ordinary, cheap edit in a repository with tests.
- **The tech lead's cards are subject to it.** The rule exists because a
  tech-lead-written card broke it.

## Rejected options and why

- **Option B** — the marginal cost is indeed minutes, and that is what makes it
  dangerous: it is always individually reasonable, and it is how a folder
  accumulates a hundred functions with eleven callers. The imagined use case
  also writes the test, so the untrustworthy code arrives pre-validated.
- **Option C** — the principle already existed as tech-lead judgement, and
  tech-lead judgement produced the card that triggered this ADR.

## Questions this opens

- **Amends the working order, not the map.** `math` moves behind `platform`;
  the register's wave note is updated. ADR-0022 and ADR-0030 stand as written.
- The engine's `CLAUDE.md` needs one sentence carrying this rule, since it binds
  coders and skills do not reach a fresh clone (ADR-0003). Requires the
  principal's permission to edit — noted as a task.
- The draft `math` card is withdrawn, not trimmed.
