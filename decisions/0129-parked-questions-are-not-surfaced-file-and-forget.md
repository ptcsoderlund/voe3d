# 0129. Parked questions are not surfaced — file and forget

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**Closes D-221**, opened by ADR-0115 as that decision's one bad consequence: *whether
anything ever surfaces a parked question whose condition has fired.* A question parked
behind *the first world texture at a grazing angle anyone complains about* is remembered
only if somebody goes looking when that happens.

On 2026-09-11 the tech lead brought evidence that it does not work: four strandings found
by hand in one session. Three questions blocked behind a question that ADR-0022 closed
about ninety-eight decisions earlier — including D-024, the keystone under the editor — and
one blocked behind a question closed that morning. The proposal was to extend the existing
rule so that closing a question also searches the cold file for anything blocked behind it.

The principal declined: *"We do file and forget. If a problem resurfaces we write it in a
new bugreport or task."*

## Options considered

### Option A — a search obligation on every closure
Closing a question also searches the cold file for anything naming it, in the same act. Two
lines in `CLAUDE.md`.

### Option B — file and forget
Nothing surfaces a parked question. The work that needs it finds it.

### Option C — a tool
A script over the `Blocked by` cells, reporting rows whose named condition no longer exists
as an open row.

## Decision

**Option B**, the principal's call, and on reflection the evidence brought against it did
not say what it was offered as saying.

**Every one of the four strandings was found the same way: somebody tried to do the work
and hit the blocker.** That is not the mechanism failing — it is the mechanism working. A
parked question is not a problem that needs to resurface on its own; it is a blocker on
work, and it surfaces exactly when that work is attempted, which is the only moment its
answer is worth anything. The cost of each discovery was a few minutes of reading.

**And a search obligation would be paid on every closure for a benefit collected on almost
none of them.** ADR-0115's whole argument is that a hot document holds what its reader can
act on now; a question nobody is blocked by is not actionable by definition, and finding it
early buys nothing but the temptation to answer it before its caller exists — which rule 10
already forbids.

## Blast radius

**Cheap.** Adding an obligation later costs two lines. Reversibility: **cheap.**

## Consequences

- **A parked question may sit behind a dead condition indefinitely, and that is accepted.**
  The record will contain rows that read as blocked and are not. Anyone reading a `Blocked
  by` cell should treat it as *what was true when it was written*, not as a live claim.
- **The cost falls on whoever tries the work**, in minutes, at the moment they have the
  most context to judge the answer. That is the right place for it.
- **This does not license leaving a *closed* row in the cold file.** That is a different
  fault and ADR-0114's existing rule already covers it — an ADR that closes a row deletes
  the row, and the row lives in whichever file holds it. Two instances on 2026-09-11 were
  the tech lead applying that rule to the hot file only; the fix is following the existing
  rule, not writing a new one.
- **Bug reports are unaffected.** ADR-0110's loop already ends this way and ADR-0109's rule
  that an unreproducible report from a real driver is worth more than a reproducible one
  from ours still stands.

## Rejected options and why

**A — a search obligation.** Rejected by the principal. The tech lead proposed it and, asked
to defend it, could not: the four instances offered as proof of failure were four instances
of discovery-on-demand succeeding, and none of them cost anything but reading time.

**C — a tool.** Rejected for the same reason, plus one of its own: a script over
`Blocked by` cells would report every row whose condition is a sentence about the world
rather than an id, which is most of them, so it would be mostly noise.

## Questions this opens

None.
