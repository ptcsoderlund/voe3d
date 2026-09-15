# 0072. Transparency is a content-priced capability, and its roadmap is staged by what bites first

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

The principal, on reading how text starts: *"We just can't live without
transparency, performance or not. So we need to plan for it."*

Two things are being said and they need separating. The first is a position on
priority — transparency is not an optional extra to be traded against frame rate.
The second is a request: the minimal form decided by ADR-0061 was scoped to
unblock text, and what it *cannot* do has been recorded only as scattered
consequences and register rows. There is no plan, only a decision and a list of
refusals.

**And there is an apparent collision to resolve first.** ADR-0066 says nothing
with a per-frame cost is on at startup, and a program that wants it turns it on.
Transparency has a per-frame cost — a second pass and a sort. Read naively, the
two decisions are in conflict and the principal's *"performance or not"* is
overriding one of them.

Constraints already fixed:

- **The minimal blended form** (ADR-0061, card 021a): three alpha modes named as
  glTF names them, one blended pass into the same target, depth test on, depth
  write off, sorted back-to-front per object on the CPU. OIT, per-triangle
  sorting, transparent shadows, refraction, dual-source blending,
  alpha-to-coverage and a separate transparent target all refused by name.
- **Premultiplied output** (ADR-0069) and **unlit as a material property**
  (ADR-0071).
- **Back-face culling is unconditional** — `render/src/device.c:756` sets
  `VK_CULL_MODE_BACK_BIT` with no material input, and `assets/model.h` records
  that `doubleSided` is deliberately not carried (D-089).
- **There is no MSAA anywhere** — every attachment and both pipelines are
  `VK_SAMPLE_COUNT_1_BIT` — and image polish, including anti-aliasing, is on the
  *later* capability list.
- **Capability is frozen in named tiers at device creation** (ADR-0063) and **a
  setting is a request clamped to the tier** (ADR-0064).
- **Nothing costly is on unless the program asks** (ADR-0066).

## Options considered

### Option A — exempt transparency from the performance rule
Write down that transparency is a correctness capability and the performance
philosophy does not apply to it. Honours the principal's sentence directly. What
it costs is the rule: an exemption granted by assertion invites the next one, and
"this feature is too important to be off by default" is an argument every feature
can make.

### Option B — no exemption, because there is no collision; and stage the rest
Observe that ADR-0066 forbids the engine spending on the *program's* behalf, and
that the blended pass spends only what the world contains. A program with no
blended material pays for no second pass and no sort — the cost is priced by
content, not by the frame. Nothing needs exempting. Then split the remaining
transparency work by which side of that line it falls on, and stage it by when
each gap actually bites.

### Option C — plan the whole of transparency now, to its end state
Decide OIT, transparent shadows, refraction and two-sided materials in one pass
so the destination is known. It is the most complete answer and the one with the
least evidence behind it: every one of those is a decision whose right answer
depends on content that does not exist, and ADR-0020 and ADR-0055 both forbid
generality with nothing to measure against.

## Decision

**Option B.**

### 1. The line that resolves the principal's sentence

**A capability priced by content is not the same as a feature priced by the
frame, and only the second is what ADR-0066 turns off.** The test is: *does a
program that never uses this pay for it?*

- **Content-priced** — the cost scales from zero with what the world holds. The
  blended pass, the sort, two-sided materials. A program with no blended material
  pays nothing. **These are always available, are never settings, and ADR-0066
  does not reach them.**
- **Frame-priced** — the cost is paid whether or not the program leans on it. OIT
  needs its accumulation and revealage targets and its resolve every frame.
  Transparent shadows cost per light. Refraction wants a copy of the frame.
  **These are opt-in capabilities under ADR-0064**, off by default, requested by
  the program, clamped to the tier.

So transparency is not exempt from the performance philosophy; **most of it never
engages it**, and the part that does is exactly the part that should be opt-in.
The principal's *"performance or not"* is correct and costs the rule nothing.

### 2. What is v1, and it is already decided

Stage 0 is card 021a and nothing changes about it. The capability table's
transparency row stays at v1.

### 3. The staged plan, ordered by when each gap bites

**Stage 1 — the gaps that make transparency usable rather than merely present.**
None needs new theory and all are content-priced.

- **Two-sided materials.** The nearest gap and the one not currently filed under
  transparency at all. Culling is unconditional, so a glass pane, a window, a
  bottle, a leaf and a sheet of cloth all lose their far side. For closed opaque
  geometry this is invisible, which is why it has never mattered; **for
  see-through geometry it is the normal case, not the exception.** The cost is a
  cull mode driven by the material. The subtlety to write down when it is taken:
  a two-sided *blended* object cannot be ordered against itself by a per-object
  sort, and the cheap answer is to draw it twice — back faces, then front faces —
  which stays per-object and needs no new machinery.
- **Sorting at scale** (D-070) and **the indirect draw** (D-071) — both already
  aimed at the sprites card.
- **Texture fringing on a soft-edged colour atlas** (D-094) — the sprites card.

**Stage 2 — the quality ceiling, and it is the one that will be noticed.**

**Transparency's visible quality is capped by the engine having no
anti-aliasing.** Text escapes it because its atlas carries its own smoothing, and
that escape is unique to text. **Cutout does not escape it**: a leaf, a fence, a
grate or a chain-link has a hard edge that crawls when the camera moves, and
there is nothing in the engine that can soften it — alpha-to-coverage is the
standard answer and it requires MSAA, which does not exist here.

This is recorded as **the first real argument for pulling anti-aliasing off the
*later* list**, and it belongs to whichever comes first of a card with foliage
and the principal deciding the picture is not good enough. It is a frame-priced
capability and lands under ADR-0064 as a setting. Named here so that when cutout
foliage looks bad, the cause is already understood and is not mistaken for a bug
in the cutout.

**Stage 3 — the correctness limits, and the ladder for each.** Each is refused
today, each has a trigger, and none is worth taking before its trigger fires:

| Limit | The ladder, cheapest first | Trigger |
|---|---|---|
| Two intersecting blended objects sort wrong | split the mesh → sort per part → per-triangle → OIT | content where it is visible and cannot be authored around |
| Many overlapping blended layers | per-object sort → OIT (frame-priced) | particles, smoke, dense foliage |
| No transparent shadows | opaque-caster approximation → alpha-tested shadow → full | the first shadow card |
| No refraction | none → a frame copy and a thickness term | the first card wanting glass that bends |

**The failure mode of a per-object sort is deliberately visible rather than
subtle**, which is what makes waiting safe: nobody has to measure to discover it.

### 4. What this ADR does not do

It takes no decision from stage 1, 2 or 3. It fixes the *shape* — the pricing
line, the order, and the trigger for each — so that each arrives as its own
decision with a card behind it, rather than as an argument in the middle of one.

## Blast radius

**Cheap.** A roadmap is reversible by writing a different one, and the only thing
here with teeth is the content-priced versus frame-priced line.

That line is worth guarding because it is what keeps ADR-0066 enforceable. If
"important enough" becomes an accepted argument for switching something on by
default, the performance philosophy stops being a rule and becomes a preference,
and the next feature to claim it will have a worse case than transparency does.

## Consequences

- **ADR-0066 gains a test rather than an exception**, and it is a test that can
  settle an argument: does a program that never uses this pay for it? That is a
  partial answer to D-085, which asks what "costly" means well enough to settle
  one — narrower than D-085 wants, and real.
- **Two-sided materials are promoted from a passing note to the next transparency
  gap.** D-089 stops being a question with a weak trigger.
- **Anti-aliasing has an argument attached to it for the first time**, and it is
  the cutout edge rather than general image quality. That is a stronger and more
  specific case than "the picture would look nicer".
- **Nothing on the board changes today.** Card 021a is unaffected, and 022 and 023
  already name the rows this stages.
- **The capability table's footnote is now incomplete** — it names what
  transparency does not include, and this ADR adds two-sidedness and the
  anti-aliasing ceiling to that list. `STATUS.md` is amended with it.
- **A frame-priced transparency feature will be the first real consumer of the
  settings decision**, which currently has an empty knob list. OIT is the likely
  first knob.

## Rejected options and why

- **Option A — exempt transparency.** Rejected because the exemption is not
  needed and is expensive to grant. The principal's position is right and is
  better served by showing there was never a conflict: an exemption would concede
  that the rule *would* have turned transparency off, which is not true of a
  content-priced cost.
- **Option C — plan it to the end state now.** Rejected on evidence. Every stage-3
  entry's right answer depends on content that does not exist; deciding OIT today
  means choosing between weighted blending and a per-pixel list with nothing to
  measure and no scene to look at. The ladders above are the useful half of that
  planning and cost nothing to write down.

## Questions this opens

- **D-095** — whether the **two-sided cull mode is a second pipeline or dynamic
  state**, and whether a two-sided blended object is drawn twice (back faces,
  then front) or accepted as unordered against itself. Trigger: the two-sided
  card, or card 021a if the principal folds it in.
- **D-096** — **when anti-aliasing leaves the *later* list**, and which kind:
  MSAA with alpha-to-coverage, which is what cutout foliage actually wants, or a
  post-process, which is cheaper and blurrier. Frame-priced, so it lands as a
  setting under ADR-0064. Trigger: the first card with cutout foliage, or the
  principal judging the picture insufficient.
