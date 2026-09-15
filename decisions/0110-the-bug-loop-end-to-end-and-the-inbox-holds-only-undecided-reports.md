# 0110. The bug loop, end to end — and the inbox holds only undecided reports

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amends:** ADR-0108 — **rule 8 is replaced.** A report no longer stays in `bugs/` for ever; it is archived the moment it becomes work or is closed. ADR-0108's other rules and all of ADR-0109 stand.
- **Closes:** D-209. Opens D-210.

## Context

The principal restated the loop back and asked for it to be confirmed and written
down as one thing:

> So the workflow is that coders and humans can report bugs into kanban/bugs.
> tech-lead decides on how to tackle the bug and creates todo notes in kanban. Bug
> report gets archived in this step. Then its the normal review complete workflow.
> So kanban coders only hand in bugreports. Yes?

**He is right, and one clause is a change rather than a restatement.** ADR-0108 rule
8 had reports staying in `bugs/` for ever with a `status:` line carrying their state.
*Archived in this step* is better and the reason is his own earlier instruction:
**the bucket is an unordered heap, so its contents should be only what has not yet
been decided.** Under rule 8 the heap accumulated everything ever reported and a
reader had to open files to find out what was still live. Archiving on the way out
makes the heap's *length* mean something.

**Third ADR on bug reports in one day, two of them amending the first.** Recorded in
the project's habit: the shape was right from the first one and the policy took
three passes. **Nothing is built against any of them**, which is what makes it free.

## Decision

### The loop, in one place

```
report        anyone — a coder, the principal, an end user
                  │      raw is fine: what happened, who, when, what they have
                  ▼
kanban/bugs/  the inbox. Unordered. Nothing is ever claimed from it.
                  │      an agent may investigate a report in place (ADR-0109)
                  ▼
root          the tech lead decides how to tackle it
                  │      an ADR only if the fix changes something settled
                  ▼
kanban/todo/  one or more cards, naming the report
                  │
                  ▼      and in the same act:
kanban/bugs/archive/   the report leaves the inbox
                  │
                  ▼
              coder implements ──► review/ ──► human ──► complete/
```

### 1. The inbox holds only reports awaiting a decision

**A report leaves `bugs/` for `bugs/archive/` on either of two events**, and no
others:

- **it became work** — one or more cards exist in `todo/`; or
- **it is finished without work** — `closed`, or `not a bug`.

Its number, its filename and its evidence travel with it, so
`bugs/archive/001-…md` keeps every screenshot beside it exactly as before. **The
`status:` line still carries the state**; the folder now carries the one bit a
reader wants first — *is this still mine to decide?*

### 2. The archive is not a graveyard, and the move reverses

A card can be abandoned. If the card that consumed a report dies without fixing it,
**the report comes back to `bugs/`** with its status reset and a dated line saying
what happened. The archive is *decided*, not *dead*.

### 3. "Coders only hand in bug reports" — yes about fixing, no about investigating

**This is the one place the principal's sentence needs splitting, and it matters
because report 001 exists.**

- **Yes, and it is absolute: a coder never fixes a bug except through a card.**
  ADR-0108 rule 6 stands. No patch on the way past, no matter how small, no matter
  how obviously right.
- **No, a coder is not limited to handing in a bare complaint.** Report 001 is an
  investigation — counts matched across platforms, nine pixels measured, a named
  suspect and a one-line experiment — produced by a coder who then stopped at the
  edit. **That is the behaviour we want and forbidding it would be the wrong lesson
  from a good report.** A coder who trips over a fault investigates it as far as the
  boundary and reports; they do not fix it, and they do not need a card to
  investigate something they have already found.

### 4. A raw report that cannot be decided gets an investigation card

**Where the previous three ADRs had a hole.** An end user's report may arrive with
nothing the tech lead can decide on — no repro, no platform, no idea which folder.
Somebody has to dig, and digging is work with an unknown outcome.

**That is a card in `todo/` whose deliverable is the investigation, not a fix.** It
keeps the invariant intact — **coders work from `todo/` and nowhere else** — instead
of quietly making `bugs/` a place work happens. Such a card says plainly that it may
conclude *not a bug* or *cannot reproduce*, and that either is a success.

### 5. A fix does not always need an ADR, and the report is the record when it does not

**Cards arrive already decided (`CLAUDE.md`), and for a bug the decision is usually
the report plus the tech lead's judgement.** An ADR is written when the fix changes
something settled — bug 001's near plane is exactly that case, since backwards depth
with the near plane at 1.0 is an engine convention. **For a fix that changes nothing
settled, the report is the written decision** and the card cites it. Otherwise every
misplaced pixel would demand an architecture record, which would devalue the ones
that matter.

## Blast radius

**Cheap.** One new folder, one rule about when a file moves, and a diagram. No code,
no card, no tooling.

Reversibility: **cheap.** Flattening the archive back into `bugs/` is a `git mv`.

## Consequences

- **`bugs/` becomes glanceable, which is the whole point.** Its length is now the
  number of things waiting on the principal or the tech lead, so it is a signal
  rather than a total.
- **D-208 gets easier and does not go away.** *Nothing surfaces a report sitting
  unlooked-at* is a smaller problem in a folder that holds only live reports, but the
  folder still has no notion of age.
- **The archive will hold the project's real defect history**, which nothing else
  does — cards record work, not faults. Worth knowing before somebody proposes
  tidying it.
- **The consequence I do not like:** point 3 draws a line between *investigating* and
  *fixing* that is a matter of judgement at the margin — reading code is
  investigation, changing one character is a fix, and the interesting cases are in
  between (a temporary edit to test a hypothesis, reverted). **The rule is: an edit
  that survives the session is a fix and needs a card**; a probe that is reverted
  before reporting is investigation and is reported as such. That is still a judgement
  and nothing checks it. Fifth such rule this week.

## Rejected options and why

**Keeping ADR-0108 rule 8 — reports never move.** Rejected on the principal's own
instruction and for the reason that motivates it: an unordered heap that accumulates
everything is not a heap you can look at, and a `status:` line inside a file is not
visible from a folder listing.

**Archiving only on close, not on card creation.** The tidier-sounding alternative,
and it keeps a report visible while it is being fixed. Rejected because *being
fixed* is already visible — the card is in `todo/` or `review/` — so the report in
the inbox would be a second copy of the same status, and the inbox would fill with
things nobody is being asked to decide.

**A `bugs/` subfolder per state.** Rejected as more structure than a project with one
report needs; `status:` inside the file carries the fine grain, and the folder
carries only the one bit that changes who is responsible.

## Questions this opens

- **D-210** — whether the archive ever needs subdividing (by year, by folder, by
  outcome). Not now, at one file.
