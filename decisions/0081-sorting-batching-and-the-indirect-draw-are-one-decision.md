# 0081. Sorting at scale, batching and the indirect draw are one decision, and the sprites card is not where it is taken

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-070

## Context

Card 022 named three of its four blocking questions around scale:

1. **D-070** — when the per-object CPU sort stops being free. ADR-0025 requires a
   per-frame rendering cost to be *argued in advance, with a number*, rather than
   discovered by measurement.
2. **D-071** — how blended geometry relates to the single indirect draw.
3. Whether this card **promotes batching and instancing off the *later* list.**

The card's premise was that sprites are the first case where the blended object
count is naturally large, so all three fire here. That premise deserved a number
before it was accepted, and the number changes the answer.

Constraints already fixed:

- **Blended objects are outside the pooled single-draw path** (ADR-0059,
  ADR-0061). They leave it and are sorted; that was written down as the price at
  the time it was paid.
- **The single indirect draw is not switched on for anything** — D-067 is still
  deferred, its trigger (*the first card drawing hundreds of objects*) unfired.
  **There is no indirect draw for blended geometry to relate to.**
- **Transparency's roadmap is already staged** (ADR-0072), which puts sorting at
  scale and the indirect draw together in Stage 1 and aims both at *the first card
  drawing hundreds of blended objects*.
- **Instancing is on the *later* capability list**, and is also the candidate
  answer to D-092 (text that changes).
- **Performance is a rendering requirement** (ADR-0025); **the engine spends
  nothing the program did not ask for** (ADR-0066); **a content-priced cost is not
  what ADR-0066 turns off** (ADR-0072).
- **A sprite is a plane and the engine does not billboard** (ADR-0080), so a
  sprite adds no per-object CPU work of its own beyond its transform.

## The number, which is what decides this

Per frame, at **1,000 blended objects**, on one thread:

| Work | Cost |
|---|---|
| Sorting back-to-front — a compact array of (float key, `uint32` index), 8 bytes per entry, ~4 KB and resident in L1 | **0.02 – 0.2 ms** (~10,000 comparisons; the range is an inlined sort on a POD key against a `qsort` through a function pointer) |
| Submitting the same objects as individual draws — per-object transform and material update, then `vkCmdDrawIndexed` | **1 – 3 ms** |

**Submission costs ten to fifty times the sort at the same object count.** The
sort does not threaten a 16.6 ms frame until roughly **10,000** blended objects,
where a naive `qsort` reaches 1–3 ms — and at 10,000 objects per-object
submission has already spent 10–30 ms and the frame is gone twice over.

Two things follow, and the second is the finding:

- **The sort is never what breaks first.** At every count where the frame still
  works, it is noise.
- **The sort only becomes a live question once batching removes the submission
  cost.** Submission is linear in object count and the sort is `n log n`, so the
  sort does eventually dominate — but only on the far side of the very change
  that would make it dominate. **Instancing is what makes D-070 live.**

## Options considered

### Option A — card 022 carries scale
Sprites, plus batching, plus hundreds of them. Fires D-067, D-070 and D-071
together. Costs: the card triples, and the indirect-draw decision for the *whole*
engine — opaque geometry included — gets taken on the evidence of a sprite demo.

### Option B — card 022 carries capability only
A quad, a sheet, an alpha mode, both layer placements, a handful in a `dev` scene.
Per-object draws, with the ceiling stated on the card. Sorting, batching and the
indirect draw stay deferred **as one bundle**, now with the number above attached
so the next card that meets them is not starting from zero.

### Option C — split now into a capability half and a scale half
Write both cards today, 022a and 022b. Costs: the scale card is written with no
content to size it against, which is Option A's guess filed under a second number.

## Decision

**Option B, and the principal chose it.** The deciding factor: **D-071 asks how
blended geometry relates to a single indirect draw that does not exist and has no
card, so answering it now means designing an interface against an imaginary
counterpart** — which is exactly the premature generality ADR-0020 and ADR-0055
forbid.

- **D-070 closes**, and it closes as *the sort is not the problem* with the number
  above as its argument. ADR-0025's requirement is met: the cost is argued in
  advance, in writing, before the card that would have paid it.
- **D-071 is re-aimed, not answered.** Its trigger stops being *the sprites card*
  and becomes **the same card as D-067** — because they are one question. A
  decision about how blended geometry is submitted at scale cannot be taken before
  there is a decision about how opaque geometry is.
- **Batching and instancing are not promoted.** They stay on the *later* list.
- **Card 022 draws sprites one per draw call, and says so on its face.** The
  ceiling is roughly a thousand blended objects before submission costs a
  millisecond or two — comfortable for anything card 022 will show, and a wall for
  a particle system. The card states the number rather than implying an
  unlimited sprite count.
- **The three rows travel together from here.** D-067, D-070's successor question
  and D-071 are one decision with one trigger: the first card that genuinely draws
  hundreds of objects — particles, grass, a crowd — with real content behind it.

## Blast radius

**Cheap.** This is a decision about which card takes a decision, plus a number
recorded in advance. Nothing is built and nothing is foreclosed: the scale
decision is unchanged in substance and is merely still ahead of us, with better
evidence than it had this morning.

The one thing that could go wrong is a card being written later that needs
hundreds of sprites and finding this ADR read as *sprites do not scale*. It does
not say that; it says the scaling work is one job and has not been done. A card
wanting it should say so in its `blocked-by` and fire the bundle.

Reversibility: **cheap.**

## Consequences

- **Card 022 becomes claimable**, which was the point. With ADR-0080 and ADR-0082
  it has no unanswered decisions left in it.
- **The sort is left exactly as card 021a built it**, and this ADR is the record
  of why that is not an oversight.
- **A number now exists on the board for per-object draw submission**, and it is
  reusable: it is the same number that will price D-092's per-glyph-quad candidate
  and any particle card.
- **The consequence we like least:** a thousand-object ceiling is low for a 2D
  game, which is the very thing card 022 exists to serve. Someone will hit it.
  They will hit it with a clear cause and a written next step, which is the best
  available outcome short of doing the work now on no evidence.
- **D-072's Stage 1 list shrinks by one row.** Sorting at scale is answered;
  two-sided materials and the indirect draw remain.

## Rejected options and why

- **Option A — carry scale.** Rejected because the indirect draw's premise is
  missing. The engine has no indirect draw for opaque geometry, so the question
  *how does blended relate to it* has no second term. Taking it on a sprite demo
  would also decide the opaque path's future on the least representative content
  in the repository.
- **Option C — split now.** Rejected because it writes a card nobody can size.
  The naming rule would make it 022b, and a 022b written today would contain the
  same guesses as Option A with a card number lending them authority.

## Questions this opens

- **D-114** — **when per-object draw submission stops being affordable, and what
  replaces it**: instancing, batching by material, or the single indirect draw
  (D-067). This is D-070's successor and it is the real question the sprites card
  was reaching for. It arrives bundled with D-067 and D-071 — one decision, three
  rows, one trigger. Trigger: the first card drawing hundreds of objects with real
  content behind it, or D-092's text-that-changes card if it takes the
  one-quad-per-glyph route, which is instancing under another name.
