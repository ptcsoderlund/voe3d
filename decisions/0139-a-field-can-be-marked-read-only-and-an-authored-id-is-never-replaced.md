# 0139. A field can be marked read-only, and an authored id is never replaced

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** — (narrows ADR-0136 point 4; adds a rule to the identity drain ADR-0137 placed)
- **Superseded by:** —

## Context

Writing card 059, the tech lead found that ADR-0136 point 4 — integers get a number box —
makes the identity's `id` draggable, since it is the only integer field the first editor
meets. Nothing would stop two entities in one file ending up with the same id, and every
saved reference would then point at the wrong thing. The principal, 2026-09-11: *"Yes,
readonly id. we want unique ids."*

Fixed by earlier decisions. ADR-0125 point 5: ids are unique **within their file**. ADR-0125
point 6: loading a template fifty times puts fifty copies of the same authored ids in one
world — so **a check that no other entity in the world has this id would be wrong**.
ADR-0137 point 4: choosing ids and keeping them unique belong to whoever creates authored
entities. ADR-0132: the inspector names no component type. ADR-0122: a description sits
beside its struct, written once, and the struct is identical in every build.

## Options considered

### Option A — the field list marks a field read-only
The declaring folder marks `id` where it lists the fields; `base`'s field record carries the
mark; the inspector shows a marked field as a label.

### Option B — integers are read-only in the first inspector
A change to ADR-0136's table and nothing else. Right today only because `id` is the one
integer; the first integer that should be draggable becomes silently uneditable.

### Option C — the identity drain refuses an id change
A replace that changes the id gets the current one back, with a warning. Alone it is a
number box that snaps back every time; beside A it is a backstop.

## Decision

**Options A and C together**, the principal's choice on the tech lead's recommendation. The
deciding factor: **how a field is edited is written once by the folder that declares it**,
which is ADR-0122's rule carried to one more property — and C costs one rule in a drain
that is being written anyway.

1. **A field list may mark a field read-only.** `base`'s field record carries the mark. The
   struct is unchanged; the mark exists only in the description.
2. **The inspector shows a read-only field as a label, whatever its kind.** This narrows
   ADR-0136 point 4, which otherwise stands.
3. **The identity's `id` is marked read-only.**
4. **The identity's drain puts back the entity's current id on any replace that changes it**
   and reports a `warning:` (ADR-0138) — the nearest valid value is the id it already has.
   An id is set when the entity is created, which is a direct call and never a replace, so
   this blocks nothing legitimate.
5. **Uniqueness stays within a file and stays the creator's to keep.** Nothing compares ids
   across a world, because templates make duplicates there correct.

## Blast radius

**Moderate.** Every field list takes the new shape — one today, the transform's. Removing
the mark later means reshaping them again. Point 4 is one rule and cheap either way.

## Consequences

- **A `base` card comes first**, and card 055 and card 059 wait on it.
- **Once created, an id cannot change through any door that replaces a row** — the inspector,
  a bridge, a script. ADR-0125's merge answer, *renumber on merge*, therefore needs a tool
  that writes ids some other way; that is its card's problem when two people edit one file.
- **The mark is the first property of a field beyond its shape.** It is where the next one —
  hidden, a range, a unit — would go, and each of those is its own decision.

## Rejected options and why

**B — integers read-only.** A rule by kind that happens to hit the right field today and a
wrong one tomorrow, silently.

**C alone.** A control that always refuses, with a warning every frame of every drag.

## Questions this opens

None.
