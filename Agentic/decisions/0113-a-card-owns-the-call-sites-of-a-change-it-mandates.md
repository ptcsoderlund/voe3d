# 0113. A card owns the call sites of a change it mandates; the rule lives in the repository

- **Status:** Accepted
- **Date:** 2026-09-10
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-218.

## Context

The coder of card 041 stopped before writing anything and asked:

> Card 041 changes `voe_ui_container.pad` from one number to four, which mechanically
> breaks two lines in `dev/src/interface.c` (`.pad = INTERFACE_INSET` and
> `.pad = 4.0f`). The card scopes itself to `ui`, and the coder rule says a
> sibling-folder edit is reported, not made — but without it `check.cmake` cannot go
> green and the card cannot move to `review/`. How should I handle it?

**He was right to stop and his reading of the rule was defensible**, which is what
makes this a decision rather than a correction. The rule he cited reads *One card,
one folder. An edit that seems to require another folder is reported, not made:
`BLOCKED: <folder>, <why>`*, with *silent scope escape into a sibling folder* named as
an anti-pattern. Taken literally it forbids the only edit that lets the repository
compile.

**Two facts make the answer easy and a third makes the rule's absence the real
finding.**

- **`dev` is downstream of `ui`, not a sibling — and the edge data says so in as many
  words.** `cmake/voe.cmake:127` reads *"dev may depend on anything, app included: it is
  the one program … It is a leaf and it stays one — dev appears in no other row."*
  Nothing in the repository depends on `dev`. The edit adds no `DEPENDS`, reverses
  nothing and reaches into no sibling's source directory, so rules 1 and 2 of
  `voe3d/CLAUDE.md` are untouched by it.
- **The tree already expected the edit.** `dev/src/interface.c:126` has carried a
  committed comment since card 034 — *"The spacer card 041 removes. See the header."*
  The project has been saying in its own source for days that card 041 edits that
  file. Only the card failed to say so.
- **The rule is not in the repository.** *One card, one folder*, `BLOCKED:` and
  *silent scope escape* exist only in `~/.claude/skills/kanban-coder/SKILL.md`, which
  is machine-local and gitignored. `voe3d/CLAUDE.md` contains no such sentence. So the
  rule binding every coder fails two statements written in the root `CLAUDE.md`: **the
  repository is the only memory**, and **a coder should need only `voe3d/CLAUDE.md`,
  the card, and the code it names.** On the principal's other machines the rule does
  not exist at all.

**This is the second coder in two days to lose a turn to one shape.** On 2026-09-09 a
coder read a scope fence written *inside card 039* as a project law and stopped on
card 040; that was answered with *decided, not absorbed* — the project objects to a
repair **absorbed** into a card with nobody deciding, and writing the work into the
card beforehand is what removes the objection. That answer lived only in the register.
By the project's own standard — a rule suspended by explicit exception three times in
two days is the wrong rule (the 2026-09-06 card-writing change) — twice in two days is
the signal to write it down rather than to answer it a third time.

**Hard constraints.**

- **`cmake -P check.cmake` exits zero before a card moves to `review/`** (rule 8). A
  repository that does not compile between two cards is one whose only gate cannot be
  run. Any reading that produces that state is refuted by this rule alone.
- **Folder dependencies point one way and a change that needs a new edge is reported,
  not made** (rule 2). Whatever is decided here must not become a licence to add an
  edge.
- **Cards arrive already decided** (root `CLAUDE.md`). A coder deciding for himself how
  far a card reaches is the thing the board exists to prevent — so the answer must be a
  rule he can apply without judgement, not an invitation to weigh it each time.

## Options considered

### Option A — a card owns the call sites of a change it mandates, downstream only

One sentence in `voe3d/CLAUDE.md` under *Work*: a card that changes a folder's public
surface also updates the call sites that change breaks, in any folder downstream of the
one it names. Anything else in another folder is still `BLOCKED:`.

Costs: one sentence, and the word *downstream* has to be understood — but it already is,
because the dependency arrows are data in `cmake/voe.cmake` and a wrong guess fails
configuration. Makes easy: the coder applies it without weighing anything, and the tree
compiles at every card boundary. Makes permanent: nothing — it is a sentence, and a
later ADR could narrow it.

### Option B — leave the strict reading and split every such change into two cards

`ui` lands first and breaks the build; `dev` lands second and repairs it.

Costs: the repository does not compile between the two, so the first card cannot pass
its own gate and cannot reach `review/`. The two must therefore land together, which is
one card wearing two numbers. It also doubles the ceremony for two-line edits, which is
the trade 040 already refused a day earlier.

### Option C — answer it per card, in the card

What was done for card 041 and for card 040 before it: the tech lead names the edit in
the card, and the general rule stays unwritten.

Costs: it works every time and it costs a coder's turn every time, because the coder
cannot know in advance whether this card is one of those. Twice in two days is the
evidence. It also leaves the *only* statement of the coder's scope rule in a gitignored
file on one machine.

## Decision

**Option A, the principal deciding**, on being shown that the rule the coder obeyed is
not in the repository at all. The deciding factor: **the rule as written produces a
repository that cannot pass its own gate**, and no reading that does that can be right —
so the choice is only between writing the correct rule down and answering it by hand for
ever.

**1. The sentence, in `voe3d/CLAUDE.md` under *Work*.** A card that changes a folder's
public surface also updates the call sites that change breaks, in any folder
**downstream** of the one the card names. That is not a scope escape; it is the change
the card ordered. Everything else in another folder is still `BLOCKED: <folder>, <why>`.

**2. Downstream is the whole of the licence, and it is checkable.** The allowed edges are
data in `cmake/voe.cmake` and a wrong one fails configuration, so *is this folder
downstream of mine* is a question with a mechanical answer and not a judgement. **No new
edge, no reversed arrow, no sibling's source directory** — those stay `BLOCKED:` exactly
as before, and this ADR grants nothing there.

**3. It covers what the change breaks and nothing else.** A call site that no longer
compiles is the card's; a bug noticed in the same file is not, and is reported. **The
test is mechanical**: would the tree build without this edit? If yes, it is not covered.

**4. The card still names it, and that duty is the tech lead's.** This rule says a coder
who finds an uncovered call site may fix it rather than stop — it does not excuse a card
that failed to name one. A card that changes a public field lists its call sites, and
card 041's omission was the tech lead's error, corrected in flight.

**5. `~/.claude/skills/kanban-coder/SKILL.md` is updated to match**, because a skill that
contradicts `voe3d/CLAUDE.md` is worse than one that is silent. The skill is the
enforceable form; **the repository is the record**, and where they differ the repository
wins.

## Blast radius

**Small and bounded by rule 2.** The expensive thing this could have become is a general
licence to edit outside the card's folder, and *downstream, and only what the change
broke* is what keeps it from being one. Reversing it means going back to Option C, which
costs a coder's turn per occurrence and nothing structural.

What is genuinely load-bearing is **point 5's ordering** — repository over skill. Getting
that backwards is how a project ends up with rules that only one machine has, which is
the fault this ADR exists to repair.

Reversibility: **cheap.**

## Consequences

- **A card's diff can now cross a folder boundary, and reviews must read it that way.**
  The principal reviewing card 041 will see `dev/src/interface.c` in the diff and that is
  correct, not a smell. Named here so it is not read as one.
- **The tech lead has a new duty on every card that changes a public field:** list the
  call sites. Cheap to do and easy to forget, and forgetting it is what happened here.
- **A coder must now distinguish *broken by this change* from *broken*.** Point 3's test
  is mechanical, but it is a judgement the previous rule did not ask for. The honest
  reading of that cost: the previous rule avoided the judgement by producing a
  repository that does not build, which is not a trade.
- **One rule now lives in two places** — `voe3d/CLAUDE.md` and the skill — and they can
  drift. Point 5 says which wins; nothing enforces it, and there is no check that could.
- **It does not fix the general problem.** *One card, one folder* and the rest of the
  coder's scope rules are still only in the skill. This ADR writes down the one sentence
  the two blocked coders needed; the wider question of what else the repository is
  missing is D-219 and is not answered here.

## Rejected options and why

- **Option B, two cards** — rejected. It leaves the tree broken between them, so the
  first card cannot pass `check.cmake` and cannot move to `review/`; the two would have
  to land together, which is one card by another name. It also doubles the ceremony for
  a two-line edit, the same trade card 040 refused on 2026-09-09.
- **Option C, per-card answers** — rejected, though it is what was done twice and it
  produced the right outcome both times. It costs a coder's turn each occurrence, it
  cannot be applied by a coder who has not asked, and it leaves the scope rule in a
  gitignored file on one machine — which is the part that made this an ADR rather than a
  third card amendment.
- **Writing the sentence only in the skill** — rejected in one line: the skill does not
  travel, and the principal works from several machines.

## Questions this opens

- **D-219** — What else binding a coder exists only in `~/.claude/skills/`, and whether
  the skill should be reduced to a pointer at `voe3d/CLAUDE.md`. This ADR moved one
  sentence; nobody has audited the rest.

**Closes D-218.**
