# 0115. A hot document holds only what its reader can act on now, and the arrival condition is the hinge

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** — (completes ADR-0114; changes no decision it records)
- **Superseded by:** —

## Context

ADR-0114 gave every always-loaded document a ceiling in lines and cut the tech lead's
unasked reading from about 350 KB to 100 KB in a day. One morning later the ceiling was
already being evaded in the file that matters most.

Measured 2026-09-11:

| | |
|---|---|
| `docs/prestudy/decision-register.md` | 168 lines against a ceiling of 200 — and 78 KB, ~20k tokens |
| Share of everything a tech-lead session loads before the principal speaks | 78% |
| Open rows | 126, average 600 characters, 71 of them over 400 |
| The longest single table row | 3,339 characters — five paragraphs of argument in one cell |

The file reported itself healthy because the ceiling counts lines and ADR-0114's rule
*one line each* is not what a line is. This is the ordinary failure of a proxy measure:
the cost is tokens, the check was lines, and prose flows sideways.

The principal rejected the obvious repair — a character cap per row — on the grounds
that **some topics genuinely need more description and some do not, and a number cannot
tell them apart.** He asked for a semantic rule instead: content already settled, or
unrelated to what is being worked on, should not be read at all.

**The hinge for that rule was already in the file, unused.** Every register row carries
a *blocked by* column, and it had never been bookkeeping — it is an arrival condition
written in plain words: *the first world texture at a grazing angle anyone complains
about*; *the principal's next `cmake -P check.cmake` on Windows*; *the second caller*;
*the theme card*; *no consumer, and no company yet*. Twelve rows say *such a card*.

**113 of the 126 rows name a condition that has not happened.** Of the thirteen naming
none, most are deferred or dormant with the condition written into the status cell
instead. Rows genuinely answerable today: about nine.

Two further measured facts decided the shape:

- **125 of the 126 open rows already have their full text preserved verbatim** in
  `register-archive/2026-09-10-before-the-split.md`, findable by id. The long rows are a
  second copy of cold content sitting in the hot file. Only the row opened by ADR-0114
  itself is new.
- **Exactly one row is self-declaredly settled.** ADR-0114's eviction rule is holding.
  The entire leak is the other kind: questions that cannot be answered yet, read every
  session by an agent who can do nothing with them.

Constraints already fixed: `STATUS.md` is the principal's only view (ADR-0114); ADRs are
the record and are append-only; the register's ids are never renumbered or reused; the
repository is the only memory across machines.

## Options considered

### Option A — A character cap per row, machine-checked
A row must fit one printed line, about 220 characters; `tools/hot.sh` reports the longest
line in every hot file. Fully mechanical, and the file cannot drift back. Costs the
rewriting of 71 rows, and destroys the descriptions in them — including several that are
the best thinking in the repository. It also answers the wrong question: a long row is
not wrong because it is long, it is wrong because nobody can act on it.

### Option B — A byte budget beside the line budget
Each hot file gets a size ceiling as well. Measures what actually costs, and lets rows be
whatever length they need. But it reports that a file is over without saying which row to
move, so the work it triggers is a re-read of the whole file — the cost it was meant to
prevent.

### Option C — Partition on the arrival condition each row already names
A row is hot only while the condition it names has arrived. Everything else moves to a
cold file at **full length, verbatim**, grouped by the condition rather than by id or
date, so *I am about to write a card in `render`* is a lookup. A row returns hot when its
condition fires and goes cold again unspent if the answer defers. No cap, no number, no
truncation.

## Decision

**Option C**, and no cap of any kind is added.

The deciding factor: the partition cuts about 95% of the file **and truncates nothing**,
because the condition is already written on every row and the full text of all but one is
already preserved cold. A cap would have paid for the same saving by destroying exactly
the complex descriptions the principal was protecting.

**1. The rule.** A hot document holds only what its reader can act on now. Length is
never the test; actionability is. A hot document that is long because all of it is live is
correct and is left alone.

**2. Cold is unbounded.** A parked question keeps every word, because the reason to move
it was never its size. Cold is filed for lookup: named for its content, grouped by what
consumes it, indexed.

**3. What makes a register row hot — and the one sub-rule worth stating.** A row is hot if
it waits on nothing, **or if it waits on the principal.** A row waiting on a fact from the
world — a complaint, a second caller, a machine, a card not yet written — is cold. The
principal arrives every session; the world does not. This is what makes the hot register
useful rather than merely small: it is the list of questions actually waiting on him.

**4. The ceilings from ADR-0114 stay, unchanged, as a backstop.** They no longer do the
work. A hot file that is over its ceiling is now a symptom to investigate, not a
quantity to trim, and `tools/hot.sh` keeps reporting them.

**5. Filing is not the tech lead's job.** The execution of this rule belongs to the
secretary role, ADR-0116. The tech lead judges whether a condition has arrived — that is
a status, and a status is content. Everything after that judgment is filing.

## Blast radius

Small and reversible. The move is verbatim and the cold file is one file; reversing the
decision is concatenating two files and deleting one heading level. No code, no board, no
engine convention depends on it. Nothing is deleted at any point.

Reversibility: **cheap.**

## Consequences

- The tech lead's unasked reading drops from about 100 KB to roughly 25 KB, and the
  register stops being 78% of it.
- The hot register becomes readable as what it is: the questions waiting on the
  principal. It can be quoted into `STATUS.md` without editing.
- **The one consequence to dislike, and it is real: nothing watches the cold file.** A
  question parked behind *the first world texture at a grazing angle anyone complains
  about* is only remembered if something looks when that happens. The conditions fall into
  three kinds, and only one of them is a genuine risk:
  1. *Waiting on a card the tech lead writes.* The trigger is the tech lead's own act, and
     the grep belongs in writing a card. Low risk.
  2. *Waiting on a fact somebody brings* — a Windows check, a complaint, a second caller.
     The grep has to happen when the fact lands, and nobody is guaranteed to do it. **This
     is where work can be silently forgotten.**
  3. *Waiting on something that will never arrive* — dormant, no consumer, no company yet.
     Losing track is the correct outcome; it will be re-raised when it matters.
- Filing cold is deliberately biased: a row wrongly parked returns when its condition
  fires, a row wrongly kept hot costs every session. When in doubt, cold.
- Grouping by condition means a row can be filed under a condition that later turns out to
  be the wrong one. That is a filing error, cheap to fix, and it is the secretary's to fix.

## Rejected options and why

- **A, the character cap.** Rejected by the principal, and he was right. It optimises
  bytes, which is a proxy; the repository's problem was relevance. Its cost falls
  precisely on the rows worth keeping — a question needing five paragraphs is usually a
  question that has been thought about hardest.
- **B, the byte budget.** Not wrong, merely useless here: once Option C is in force the
  register is a few kilobytes and no budget is near binding. Kept in reserve if a hot file
  is ever long with genuinely live content, which is the case ADR-0114's ceilings already
  cover.

## Questions this opens

- Whether anything ever surfaces a parked question whose condition has fired, given
  consequence 3 above — a habit at the moment a fact arrives, or a mechanism.
