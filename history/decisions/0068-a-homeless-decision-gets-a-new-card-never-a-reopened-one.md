# 0068. The blended pass gets a card of its own; a closed card is never reopened to carry it

- **Status:** Accepted
- **Date:** 2026-09-05
- **Deciders:** Human, Tech Lead
- **Supersedes:** — (ADR-0061 stands; only its card placement is replaced)
- **Superseded by:** —
- **Closes:** the placement gap opened by ADR-0061

## Context

ADR-0061 decided transparency and, in the same breath, decided *where the work
goes*: **card 019 carries it** — the material-shading card — so that a blended
quad in front of a cube could be checked by eye before glyph rasterisation added
its own unknowns. That placement required two card edits, both the principal's
act under the root `CLAUDE.md`, and **neither was made.**

Card 019 was then claimed, implemented and moved to `complete/` without any of
it. The tree says so in three places:

- `render/src/device.c:787` — *"Written, not blended. Everything this engine
  draws is opaque and there is nothing underneath it but the clear; transparency
  is its own decision and it starts with sorting."*
- `render/shaders/draw.slang:371` — *"The alpha is carried through untouched.
  Nothing blends yet."*
- `3d/include/3d/draw_system.h` — *"Table order is safe because everything here
  is opaque… nothing on this card is blended."*

And `assets/include/assets/model.h` records the matching gap on the reader's
side: `doubleSided` and the alpha mode are deliberately **not carried**, because
*"a field that is read and ignored reads as a feature"*, with the note that the
card that blends is the card that adds them.

So the state is: **a decision that is accepted, correct, and unimplemented, whose
host card is closed.** Meanwhile card 021's `blocked-by` still reads `017` alone,
017 is complete, and text is therefore claimable *now* on top of blending that
does not exist — which is exactly the failure ADR-0061 was taken to prevent,
reached by the other route.

Constraints already fixed:

- **ADRs are append-only and superseded, never edited in place** (tech-lead
  discipline, ADR-0056's spirit). ADR-0061's *decision* is not in dispute and
  must not be rewritten to point somewhere new.
- **A card in `complete/` has been reviewed by the principal** (root `CLAUDE.md`,
  rule 4). What it says is what was reviewed.
- **A coder reads the card, not the planning** (root `CLAUDE.md`, rule 3). A
  card that does not name the work does not get the work done, however clearly an
  ADR names it.
- **Implement on demand** (ADR-0034) and **the analyser-clean, check-green bar
  per card** (engine `CLAUDE.md`, rule 8). Both are per-card properties, so the
  unit of work is the card and not the ADR.

## Options considered

### Option A — reopen card 019, move it back to `todo/`, grow its scope
The placement ADR-0061 chose, honoured late. One card, no new number, and the
work lands beside the material code it touches. What it costs is the board's
honesty: 019 was reviewed and completed on the text it had, and a completed card
that later grows scope means `complete/` no longer records what was reviewed. It
also re-opens a card whose *Verify* section has already been signed off, so
either the whole card is re-verified or the folder starts lying about which parts
were.

### Option B — a card of its own, ahead of text
A new card carrying exactly what ADR-0061 describes and nothing else: the three
alpha modes through reader, importer, component and shader; the second pipeline;
the sort in `3d`. 019 stays closed and true. The number is a suffixed one — 021a
— so the backlog keeps its reading order without renumbering 022 and 023 and
without a card number that implies it came after the GUI. Costs one more card
boundary, and the blended quad is checked against the cubes that already exist
rather than against the lit model card.

### Option C — fold it into card 021 and let text carry its own blending
No new card at all. What it costs is precisely what ADR-0061 spent its deciding
paragraph avoiding: the first blended pixel in the engine arrives in the same
card as a hand-written TrueType reader, a rasteriser, an atlas and a layout pass,
so when the text looks wrong there are five candidate causes and no way to
bisect them.

## Decision

**Option B**, and the general rule it implies.

**1. The blended pass becomes card 021a**, written now, ahead of text. Its scope
is ADR-0061's decision and nothing else, verified by eye against geometry that
already exists.

**2. Card 021 is split and rewritten** into 021a (the blended pass) and 021b
(text), and 021b's `blocked-by` names 021a. The old 021 is removed rather than
kept beside them, on ADR-0056's principle: two files claiming the same number is
a state where the reader has to guess which one is live.

**3. The general rule, which is the part worth keeping:** when a decision's host
card closes without it, the work gets a **new card**, and a card in `complete/`
is never reopened to grow scope it was not reviewed with. `complete/` is a record
of what was reviewed, and a folder that records something else is worse than a
slightly longer backlog.

**4. The trigger that should have fired, named so it fires next time:** an ADR
that assigns work to an existing card creates an obligation on the board, and it
is not discharged until the card's text says so. **An ADR whose card edits have
not been made is not finished**, and the register's session note is where that is
visible. This one went unnoticed for a day because the decision was recorded
perfectly and the board was never touched.

**Not decided here:** anything about how transparency works. ADR-0061 is
unchanged and is the authority; this ADR moves its work and nothing else.

## Blast radius

**Cheap, and almost entirely on the board rather than in the tree.** Card numbers
with a letter suffix are new — nothing before 021a used one — and that is the one
thing that propagates: the register's card table, 022's and 023's `blocked-by`
lines, and any later split. Reversing it means renumbering, which is an
afternoon of text and no code.

The part that is *not* cheap to reverse is rule 3, and it is deliberately the
part with the least code in it: once `complete/` is trusted as a record of what
was reviewed, a single reopened card undoes that trust for all of them.

## Consequences

- **Text is honestly blocked again.** 021b names 021a, so a coder cannot claim
  text before blending exists. That is the restoration of a block that ADR-0061
  put there and that the board lost.
- **A card number now has a letter in it.** The convention is: a suffix means a
  card was split after being written, and the letters run in dependency order.
- **The engine gets its first pipeline variation on a card whose whole subject is
  that variation**, which is the right place for the variant-explosion watch
  ADR-0061 asked for.
- **Three folders change on 021a that card 019 never touched** — `assets` gains
  the alpha mode on the reader, `3d` gains the sort, `render` gains the second
  pipeline — so the card is wider than 019's amendment would have been and
  correspondingly clearer to review.
- **`complete/` still says `status: review` on ten of its eleven cards.** That is
  a separate, older piece of housekeeping, and it is fixed in the same pass as
  this ADR because it is the same failure in miniature: the file and the folder
  disagreeing about what happened.

## Rejected options and why

- **Option A — reopen 019.** Rejected on the board's honesty, not on effort. It
  is the cheaper option in the tree and the more expensive one in the record:
  either the card is fully re-verified, including the shader and the four GPU
  tests the coder could not run, or `complete/` stops meaning reviewed.
- **Option C — text carries its own blending.** Rejected for the reason ADR-0061
  named when it chose 019 in the first place, which has not changed: the first
  blended pixel should be checkable against a cube, not against a glyph.

## Questions this opens

- **D-089** — whether a material's `doubleSided` is carried alongside its alpha
  mode. `assets/include/assets/model.h` names the two together as the same shape
  of gap, and the engine culls back faces whatever a material says. Blended
  geometry is where two-sidedness first becomes visible, so the question arrives
  with 021a even though the answer may be "not yet".
