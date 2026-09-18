# 0120. A folder summary is a present-tense map, and it delegates its subfolders

- **Status:** Accepted
- **Date:** 2026-09-11
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

`voe3d/render/render.md` is 194 lines against a 120-line ceiling and has been the
only `OVER` in `tools/hot.sh` since ADR-0114 set the ceilings. The secretary
surveyed it on 2026-09-11 under errand 3 and reported two measured facts, neither
of which she could act on because both turn on meaning:

1. **The file is one folder's map with a card-keyed changelog accreted into it.**
   Eleven card references; the `include/render/device.h` entry alone is 42 lines —
   22% of the file — of which lines 31–63 are past-tense clauses of the form
   *"Card NNN added X; its header says…"*, one clause per card, never folded back
   into the present-tense description the rest of the file is made of. All ten
   cards named are in `kanban/complete/`, which is cold, append-only and already
   the record of that history. The same pattern sits in `dev/dev.md`, at 117 of
   120 and about to go over for the same reason. It is absent from `3d`, `ui`,
   `text` and `scene`. The two affected files are the two every card touches.
2. **No single cut closes the gap.** The preamble is 21 lines and answers exactly
   what the rule asks. The 28 entries are 171 lines. Folding the `device.h`
   changelog leaves ~161. At `3d`'s density of 4.6 lines an entry, 28 entries plus
   a preamble is ~150 whatever is trimmed.

The hard constraints. ADR-0114 gives every always-loaded document a ceiling and
says the write that heats one thing cools what it replaces. ADR-0115 says length
is a symptom and never the test — a long hot file that is all live content is
correct — and that an `OVER` is something to investigate, not a quantity to trim.
ADR-0116 keeps the secretary to structure and off meaning. And the engine already
holds the answer's second half as an unremarked precedent: `render/vulkan/` carries
its own `vulkan/vulkan.md` and costs `render.md` exactly one line. It is the only
subfolder summary in the engine.

## Options considered

### Option A — Raise `render`'s ceiling
Declare 194 lines correct for the folder that is bigger than the others and move
on. Costs nothing today. ADR-0115 explicitly permits it where the content is live.

### Option B — Split the `render` module
Carve the element path, or the tests, out into a sibling folder so each half has
its own page. Structural, and it is a module-boundary decision rather than a
filing one.

### Option C — A folder summary is a present-tense map, and it delegates subfolders
Two edges of one rule about what kind of document a folder summary is.

**Present tense.** A folder summary describes what the folder *is* now. It does not
narrate how it got there. `kanban/complete/` is that record, it is cold, and a
clause in a hot file that repeats it is the re-heating ADR-0114 exists to prevent.
A card that changes a folder **rewrites** the affected entry; it does not append a
sentence beginning *Card NNN added*.

**Delegation.** A subfolder large enough to crowd its parent's page takes its own
summary and the parent keeps one line pointing at it, exactly as `vulkan/` already
does. For `render` that is `tests/` (6 entries, 37 lines) and `shaders/` (3 entries,
22 lines). Both are read by a different reader than the one `render.md` is for.

Together: 194 − 37 − 22 + 2 pointer lines = ~137, and folding the `device.h`
changelog back into present tense takes it to ~102.

## Decision

**Option C.** The deciding factor is that the secretary's measurement found real
cold content in a hot file — a changelog duplicating `complete/` — which settles
the question ADR-0115 says to ask of every `OVER`: this one is not a long file that
is all live content, so the ceiling is right and the file is wrong.

The rule is general and applies to every folder summary in the engine, not only to
`render`. `dev/dev.md` is at 117 with the same changelog and is the next one over.

## Blast radius

Cheap. Delegation is reversible by concatenation; a subfolder summary merges back
into its parent with no code touched. The present-tense rule costs a coder nothing
per card — rewriting an entry and appending to it are the same edit at the same
moment — and where it was not followed, the history it would have kept is already
in `complete/` and is not lost. Reversibility: **cheap**.

## Consequences

- **One card does the whole of it, and errand 3 closes without the secretary acting.**
  Card **047**: create `render/tests/tests.md` and `render/shaders/shaders.md`, and
  fold the card-keyed changelog in `render.md` and `dev.md` back into present tense.
  Splitting it — filing to the secretary, folding to a coder — would put two roles in
  the same submodule at the same time, which ADR-0116 rule 3 forbids and D-222 says
  nothing currently coordinates. The filing half is small and the card names the files
  exactly, so nothing is gained by the split and a collision is risked by it. **Where a
  document's structure and its meaning are the same edit, it is a card.**
- **The thing we lose.** The card-keyed clauses were a genuine convenience: a coder
  reading `device.h`'s entry could see which card introduced a given call without
  opening `complete/`. After this they must look it up. That is the trade ADR-0114
  already made everywhere else, applied here too.
- A subfolder summary is now the normal answer to a crowded folder page, so expect
  more of them. `vulkan/` stops being an anomaly.
- This does not license splitting a summary to dodge a ceiling: `src/` is not a
  candidate, because the files under it are what `render` *is* and a reader opening
  the folder needs them on the page they opened.

## Rejected options and why

**A — raise the ceiling.** Rejected because the investigation ADR-0115 mandates
found cold content, which is the case the ceiling is for. `render` is also not
meaningfully bigger than its peers: 28 entries over 28 files against `3d`'s 23 over
24 in 106 lines. Raising it would say the folder is exceptional when the measurement
says the document is.

**B — split the module.** Rejected because there is no architectural reason. `render`
is one coherent thing and its own preamble says so — the only folder that names
Vulkan, with no abstraction over it and no second graphics API coming. Splitting a
module because its documentation is long is deciding by document, and it would put a
real boundary where none is wanted to solve a filing problem.

## Questions this opens

None that are not already open. D-222 — who tells the secretary the board is quiet —
is the gate on errand 3 as it is on every errand in `voe3d/`, and this ADR does not
close it; the board being empty on 2026-09-11 is a fact for that session, not a rule.
