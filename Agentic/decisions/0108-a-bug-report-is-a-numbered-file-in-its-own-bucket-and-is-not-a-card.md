# 0108. A bug report is a numbered file in its own bucket, and it is not a card

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amended by:** ADR-0109, 2026-09-10, the same day — **anyone may report, so the eight sections below are what *investigation* produces and not what intake requires.** A raw report is what happened, who saw it and when, plus whatever they already have; an agent grows it into this shape in place. Rules 1, 2, 5, 6, 7 and 8 stand exactly as written; rules 3 and 4 apply from `investigated` onward. The bucket is also stated to be an unordered inbox, which this ADR implied and did not say.
- **Amended by:** ADR-0110, 2026-09-10, the same day — **rule 8 is replaced.** A report does not stay here for ever: it moves to `bugs/archive/` the moment it becomes work (a card exists) or is finished without work (`closed`, `not a bug`), so the inbox holds only reports still awaiting a decision. Its number and evidence travel with it and the move reverses if a card is abandoned. Every other rule below stands.
- **Closes:** D-202. Opens D-203.

## Context

**The convention was named as a direction four days before it existed and is now
being settled by an example rather than by a discussion.** `CLAUDE.md` has carried,
since 2026-09-06: *"Bug reports will enter the board in a similar fashion. Stated by
the principal as a direction, not yet a convention; the shape is undecided and is a
register row, not something to improvise."*

On 2026-09-10 the coder of card 034 improvised it anyway — correctly. He created
`voe3d/kanban/bugs/`, wrote `001-element-surface-blank-on-windows.md` with two
screenshots beside it, and the principal's instruction is that **this is the standard
way from now on.** What follows is that shape written down, plus the two rules the
example implies but does not state.

**This is the third time in a week that a convention arrived as a working artifact
before it arrived as a rule** — the theme's sectioned file and the standing card
grant were the others. Recorded because it is becoming the project's actual method:
the rule is extracted from a good instance rather than guessed in advance.

## Decision

### 1. `voe3d/kanban/bugs/`, one file per report, `NNN-slug.md`

Screenshots and any other evidence sit **beside** the report under the same number
(`001-…-annotated.png`). The bucket keeps a `.gitkeep.md` explaining itself so an
empty board still carries the convention.

### 2. The numbering is the bucket's own and starts at 001

**A bug has no place in the plan's sequence**, so it does not take a card number.
This is the coder's own reasoning and it is right: card numbers come from the
roadmap, and interleaving faults into them would make the plan unreadable and the
card-spin-off rule (`021a`, `021b`) ambiguous.

### 3. A report opens with a header of facts, and severity is a sentence

`status`, `found-by` (who, when, and on what), `reported-by`, `folder`, `severity`.

**Severity is prose, not a scale.** The example — *"everything mapped onto the window
is invisible. The world is unaffected"* — says more than any P1 ever has, and it says
the second half, which a scale cannot: **what is *not* affected**. A severity that
names the blast radius on both sides is what a reader needs to decide whether to stop
what they are doing.

### 4. The sections that earned their place

Required, because each one answers a question a reader will otherwise ask:

- **What happens**, and **what should happen** — separately, so the expectation is on
  file even if the fault later turns out to be the expectation.
- **Evidence that the code ran** — the example's element and draw counts, matched
  Windows against Linux in a table, proving the records went in and the commands were
  recorded. **This is the section that stops a fault being mistaken for a stale build
  or a missing call**, and it is the one most reports elsewhere lack.
- **Where it isolates to** — what is the same between the working and failing cases,
  and what is the one thing that differs.
- **The suspect, labelled a hypothesis** — see rule 6.
- **What confirms or kills it, in one line** — the cheapest experiment that
  discriminates, written so somebody else can run it.
- **Why it was not caught before** — usually the one-platform rule working as
  written, and saying so stops a blameless gap being read as negligence.
- **Verified on** — every platform tried, *including the ones where it does not
  reproduce*, with toolchain and window size. **"Does not reproduce here" is a
  finding and is reported as one.**
- **Notes** — anything in the evidence that looks like the fault and is not. The
  example's `mouse locked` line is exactly this, and it saves the next reader a
  wrong turn.

### 5. A report is not a card, and nobody works out of `bugs/`

**Nothing is ever claimed from `bugs/`.** A report is a finding. When the fix is
decided, **a card goes into `todo/` naming the report**, and the report is what the
card's reasoning points at. Two reasons, and the first is the load-bearing one:

- **A fix is frequently a decision** — see rule 6 — and cards arrive already decided
  (`CLAUDE.md`). A bucket that could be worked directly would be a second board where
  work starts undecided, which is the one property this board exists to prevent.
- Reports and cards have different lifetimes. A card is done and moves to
  `complete/`; a report is **evidence** and stays findable by its number for ever.

### 6. A bug report does not fix the bug, and this is the rule to keep

The coder found the suspect line, did not touch it, and said why: it is in another
folder, and reverse depth with the near plane at 1.0 is a settled engine convention,
so *"moving it is a decision and not a patch"*.

**That restraint is made binding.** A reporter may investigate as deeply as they
like and must stop at the edit when the fix would change a decision, cross a folder
their card did not name, or is not obviously the only candidate. **The report ends
at the experiment that would settle it.**

This is ADR-0068's shape applied to faults — a homeless decision gets a new card,
never a reopened one — and it is the second time in five days the same coder has
stopped at a boundary and reported instead of crossing it.

### 7. A report says which cards it does *not* impeach

**The example does this unasked and it is the most valuable paragraph in it:** card
034 is not at fault, must not be reopened, and the fault is older than it and would
be visible with it backed out. Without that sentence a fault found while working a
card reads as that card's failure. **A report that touches a card in `review/` says
plainly whether the review is affected.**

### 8. The principal closes a report, and it stays where it is

A report's `status` moves to closed by the human, as with `complete/`, and **the file
does not move.** There is no `bugs/closed/`: the bucket is evidence rather than a
queue, reports are few, and a closed one is the record that the fault was real. The
card that fixed it names the report's number, so the two are findable from each other.

## Blast radius

**Cheap.** Nothing is built and no code is touched: one bucket that exists, one
naming rule, and eight paragraphs of shape. The board gains no state machine.

Reversibility: **cheap.** A different shape is a new ADR and the existing reports are
still readable under it.

## Consequences

- **`CLAUDE.md`'s "not yet a convention, do not improvise" clause is spent** and is
  replaced by a pointer to this ADR.
- **The bucket has no in-progress state and deliberately so.** A report is either
  open or closed; anything being *worked* is a card in `todo/` or `review/`, which is
  where work is visible. This is D-017's question — an in-progress bucket — answered
  for bugs without answering it for cards.
- **The consequence I do not like:** rule 6 is another convention nothing checks.
  Third in a week. A reporter who patches the suspect line anyway produces a
  green check and a silent decision, which is precisely the failure the rule exists
  to prevent and precisely the one no script can see. **What makes it survivable is
  that it is an agent-facing rule in a repository agents read every session**, and
  that the human reviews every card — but a report is not a card and gets no review,
  so the gap is real. D-203.
- **Numbering can collide** when two agents write a report in the same session on
  different machines, exactly as card numbers can. The same answer applies: the
  principal owns the remotes and resolves it, and a colliding file is removed rather
  than kept (ADR-0056).

## Rejected options and why

**Bugs as cards in `todo/`, in the card sequence.** Rejected on the coder's own
argument: the plan's numbering means something and a fault has no place in it. It
also puts undecided work on the board, which rule 5 exists to stop.

**A bug bucket that agents work out of directly.** Rejected as the second board
described in rule 5. The attraction is real — a fault with an obvious one-line fix
wants to be fixed — and the answer is that such a fix is a card, written that
morning, which costs a minute and keeps one path onto the board.

**A severity scale.** Rejected: it compresses out what the sentence carries, which is
what is unaffected.

## Questions this opens

- **D-203** — that rules 5 and 6 have no machine check.
