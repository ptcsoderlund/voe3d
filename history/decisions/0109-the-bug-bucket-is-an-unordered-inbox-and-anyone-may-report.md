# 0109. The bug bucket is an unordered inbox, and anyone may report into it

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amends:** ADR-0108 (a bug report is a numbered file in its own bucket, and it is not a card)
- **Closes:** D-206. Opens D-207, D-208.

## Context

ADR-0108 was written hours earlier from the coder's report 001 and **it assumed one
kind of reporter**: an agent inside the repository, holding a card, able to match
counts against another platform and to say which cards a fault does not impeach. The
principal has widened it:

> coders and end users should be able to report bugs. We could store them unsorted
> and unordered in bugs folder before we decide which order to fix them in todo.

**Two things follow and only the first is a real change.**

- **The bucket is an unordered inbox.** ADR-0108 already said nobody works out of
  `bugs/` and that a decided fix becomes a card; this states the positive half —
  **arrival order is the only order the bucket has, and priority is decided at the
  moment a card is written, not before.** That is a clarification, and it is worth
  writing down because a numbered bucket invites being read as a queue.
- **An end user cannot produce an ADR-0108 report.** They have no counts, no second
  platform, no view of the cards, and no idea what a folder is. ADR-0108's eight
  required sections would make a valid complaint into a malformed file, and the
  predictable result is that nobody reports anything. **This is the substantive
  change.**

## Decision

### 1. A report arrives raw, and is investigated in place

**Intake is deliberately almost free.** What a report must carry on arrival:

- **what happened**, in the reporter's own words, kept verbatim;
- **who reported it and when**;
- **anything they already have** — a screenshot, what they were doing, which build.

**Nothing else, and no section headings.** A one-paragraph complaint with a
screenshot is a valid report and goes straight into the bucket.

**ADR-0108's eight sections are what *investigation* produces, not what intake
requires.** An agent picks a raw report up, reproduces it or fails to, and grows the
file into the full shape — same file, same number. That is the amendment to
ADR-0108 rule 4: its sections are now required of an **investigated** report and of
nothing else.

### 2. The reporter's own words are never rewritten

An investigation adds; it does not replace. **The complaint stays in the file
verbatim, under the reporter's name**, because it is evidence: what somebody
believed they saw is the record, and an agent's paraphrase of it has repeatedly
turned out to be the thing that was wrong (ADR-0056 is this project's own example).
Where the investigation contradicts the complaint, both stand and the file says which
is which.

### 3. `status` carries the state, and there are five

`new` → `investigated` → `card NNN` → `closed`, plus `not a bug`.

- **`new`** — arrived, nobody has looked. The inbox's normal state and not a
  reproach.
- **`investigated`** — reproduced or not, with ADR-0108's sections filled in and an
  experiment named. **`could not reproduce` is a legitimate resting place**, written
  as a finding with what was tried, not as a limbo.
- **`card NNN`** — a card exists in `todo/`. The report is no longer the live thing.
- **`closed`** — the principal's act, as with `complete/`. The file does not move
  (ADR-0108 rule 8).
- **`not a bug`** — with the reason, and **courteously**, because an end user
  reporting their own misuse has still told us something: the third such report on
  one behaviour is a usability finding about the engine, not three mistakes by three
  people.

### 4. Numbers are arrival order and mean nothing else

Not severity, not priority, not the order of fixing. **The order things get fixed
lives in `todo/` and nowhere else**, and it is the principal's to set when cards are
written. A bucket of forty `new` reports is a healthy inbox and not a backlog
failure.

### 5. How an end user's report physically arrives is not solved here

**Today the principal is the intake.** He owns every remote (`CLAUDE.md`), so an
outside report reaches the bucket because he or an agent writes it in, **attributed
to the reporter rather than to whoever typed it**. There is no issue tracker, no
in-app reporter and no mailbox, and this ADR invents none — see D-207. What it fixes
is that the *shape* no longer blocks such a report the day a channel exists.

## Blast radius

**Cheap.** No code, no new bucket, no state machine anywhere but a `status:` line.
ADR-0108's file layout, numbering, no-work-out-of-bugs rule, no-fixing rule,
does-not-impeach rule and closing rule all stand untouched.

Reversibility: **cheap.**

## Consequences

- **The bucket will grow and will look untidy, and that is the intended state.** An
  inbox whose length is a source of pressure gets triaged into `not a bug` to shorten
  it, which is how real faults get lost. Length is not a signal here.
- **`voe3d/CLAUDE.md`'s one-platform rule said the other platform's findings become
  *a new card*.** Under ADR-0108 they become a **report** — a card would be undecided
  work on the board. Corrected in that file as part of this decision.
- **An end user's report will often be unreproducible**, because their machine, driver
  and build are not ours. D-205 is exactly why: our Linux checking runs on a software
  rasteriser, so an outside report from a real driver may be the *only* evidence a
  fault exists. **An unreproducible report from a real driver is worth more than a
  reproducible one from ours**, and must not be closed for being inconvenient.
- **The consequence I do not like:** the intake is now cheap enough that a report can
  sit at `new` for ever with nobody obliged to look, and nothing measures that. The
  board has no mechanism for *oldest unlooked-at thing*, and I am not inventing one
  for a bucket with one file in it. D-208 carries it, and the trigger is the bucket
  reaching a size where the principal cannot see it all at once.
- **Fourth convention in a week that rests on somebody remembering it** rather than on
  a check. Stated plainly rather than absorbed.

## Rejected options and why

**Two buckets — a raw inbox and an investigated one.** The obvious shape and
rejected: a report's number is its identity and moving files between buckets breaks
the link to its screenshots for no gain, when a `status:` line carries the same
information and is visible in the file rather than in its path.

**Requiring ADR-0108's sections at intake, with a helper to fill them.** Rejected
because the cost lands on the reporter at the exact moment we want them to bother.
The sections are cheap for an agent and expensive for a stranger.

**Ordering the bucket by severity.** Rejected as the principal's own instruction —
*unsorted and unordered* — and because it would make the bucket a second priority
list competing with `todo/`, which is the one the plan actually runs on.

## Questions this opens

- **D-207** — the channel an end user's report actually arrives through.
- **D-208** — that nothing surfaces a report sitting unlooked-at.
