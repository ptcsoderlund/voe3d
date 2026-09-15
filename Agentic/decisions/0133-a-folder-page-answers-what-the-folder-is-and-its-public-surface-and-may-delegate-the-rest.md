# 0133. A folder page answers what the folder is and what its public surface is, and may delegate the rest

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human (in the coder's session), Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amends:** ADR-0120, one consequence bullet — see *Decision*, point 1.

## Context

ADR-0120's last consequence bullet reads:

> This does not license splitting a summary to dodge a ceiling: `src/` is not a
> candidate, because the files under it are what `render` *is* and a reader opening the
> folder needs them on the page they opened.

Card 047 carried it forward verbatim into *What must not change*. **The principal lifted
it in session on 2026-09-11**, after the card had been worked and the ceiling was reported
unreachable without it. `render/src/src.md` exists. The coder annotated the card in both
places, dated, explicitly recording the decision rather than making one, and raised that
the ADR and the tree now disagree. **This ADR closes that disagreement.** Raising it was
correct: a card annotation is not the record, and an ADR nobody amends is one a later
session reads as still in force.

**The arithmetic in ADR-0120 was wrong, and that is what made the prohibition look
affordable.** It predicted `render.md` at ~137 lines after delegating `tests/` and
`shaders/`, and ~102 after folding the card-keyed changelog in the `include/render/device.h`
entry — a fold worth ~35 lines. The fold was worth **three**: the entry went from 42 lines
to 39. The card's own constraint is why, and it was the right constraint — *nothing
described may be dropped*, because every *"its header says…"* clause names something the
header still explains. **What looked like a changelog was present-tense content wearing
past-tense framing.** Removing the framing removed words, not lines. Measured after the
work: the page would stand at ~136 with `src/` folded back in, against a ceiling of 120.

So the estimate that made `src/` unnecessary was off by the entire gap, and ADR-0115 had
already named the error — *length is a symptom, never the test* — in an ADR whose own
arithmetic then treated it as one.

**What the prohibition was protecting is real and is not withdrawn.** A reader opening
`render/` must not land on a page that tells them nothing and points elsewhere for
everything. The question is which part of the page carries that duty.

Constraints already fixed. ADR-0114: every always-loaded document has a ceiling. ADR-0115:
an `OVER` is investigated, not trimmed, and a long page that is all live content is
correct. ADR-0120's two edges — present tense, and delegation — both stand.

## Options considered

### Option A — restore the prohibition and raise `render`'s ceiling instead
Undo the session's work, put 17 implementation entries back on the parent page, and declare
~136 correct for `render`. ADR-0115 permits it where content is live, and this content is.

### Option B — withdraw the prohibition and say nothing in its place
Delegation is judged case by case. Cheapest to write; leaves the next folder page with no
test beyond taste, and leaves ADR-0120's real concern unrecorded.

### Option C — replace the prohibition with what it was protecting
Name the duty the parent page owes its reader, and let anything not owed be delegated when
it crowds the page. `src/` stops being a named exception; `include/` stops needing to be
one.

## Decision

**Option C.** The deciding factor: the prohibition named a folder, and what it was actually
protecting was a *question the reader arrives with* — so the rule should name the question.

1. **ADR-0120's `src/`-is-not-a-candidate bullet is withdrawn.** The rest of ADR-0120 —
   present tense, and delegation of a crowded subfolder — stands untouched and is not
   reopened.
2. **A folder page must answer two things on its own**: what the folder is for, and what
   its public surface is. In `render` that is the preamble and the
   `include/render/device.h` entry, which is where they still are.
3. **Everything else may be delegated to a subfolder summary when it crowds the page**,
   with one pointer line in the parent, as `vulkan/` established. `src/` is an ordinary
   candidate under this test. **`include/` is not**, because the public surface is the
   parent's own duty under point 2 — which is the same reason ADR-0120 gave, now attached
   to the thing that earns it.
4. **Crowding is the trigger, not tidiness.** A page under its ceiling delegates nothing.
   No folder splits `src/` because `render` did.
5. **An estimate in an ADR is not a finding.** Where a decision turns on a number nobody
   has measured, the card that measures it may contradict it, and that is the card working
   — not a deviation. It comes back as an amendment like this one.

## Blast radius

Cheap, and unchanged from ADR-0120's own assessment: a subfolder summary merges back into
its parent by concatenation, with no code touched. What point 2 makes expensive to reverse
is the duty itself — if a folder page ever stops answering what the folder is and what its
public surface is, delegation has become indexing, and the reader who opens a folder to
find out what it does has nowhere to land. Reversibility: **cheap.**

## Consequences

- **`render/` as built is correct**, and card 047 is reviewable as it stands. The tree and
  the record agree again.
- **The cost the prohibition foresaw is real and has been paid.** What each implementation
  file in `render` does is now one click down from the folder page. The coder verified the
  page still answers on its own what the folder is for and named this as the amendment's
  cost; recording it here is the honest version of that trade, not a claim it was free.
- **More folder pages will delegate `src/` in time**, and that is expected rather than
  regretted. None should until its own page is over.
- **`render/include/` stays on the parent page** permanently, by point 3 rather than by
  omission.
- **ADR-0120 keeps its number and its status.** It is amended in one bullet, not
  superseded; a reader who opens it for the present-tense rule finds it intact.
- **The lesson generalises past folder summaries.** ADR-0120's reasoning was sound and its
  spreadsheet was not, and nothing between the two would have caught it before a coder did
  the work. That is an argument for cards being allowed to report numbers back, which
  point 5 now says outright.

## Rejected options and why

**A — restore the prohibition, raise the ceiling.** It would throw away work that is done
and correct, to honour a bullet whose supporting arithmetic is now known to be wrong by
about thirty-four lines. ADR-0115 does permit raising a ceiling for live content — but the
test it sets is whether the *reader* needs it on that page, and seventeen implementation
entries are read by somebody who has already decided to work inside `render`, not by
somebody finding out what `render` is. The delegation is right on the merits, not only on
the line count.

**B — withdraw and say nothing.** It would leave the next person with a withdrawn sentence
and no rule, and would lose the one true thing the prohibition contained: that a folder page
owes its reader an answer and not a table of contents.

## Questions this opens

None. D-222 — who tells the secretary the board is quiet — is untouched, and this ADR
orders no filing work.
