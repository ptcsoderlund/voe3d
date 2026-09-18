# 0079. A distance field is filtered, because a distance is not a colour

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —

## Context

**This is a defect, found by the principal's eye, in work accepted the same day.**
It is the third text row in three days opened by him reporting a symptom rather
than by a card reaching a written trigger, and that pattern is now worth more
attention than any one of the three.

The report, in his words: *"an O ... the left vertical line is what seems like 3
units wide (pixel-ish) and the right vertical line is 2 units, relative to each
other the right side is 30% thinner"* — and, decisively, **the same at any
distance**: *"It doesnt go away when it covers the screen."*

That last sentence is the whole diagnosis. A hard cut against the *screen* pixel
grid produces an error that shrinks as the letter grows and disappears when it
fills the display. An error that holds its ratio at every size is measured
against a grid that scales with the letter, and there is only one such grid: the
sheet's own texels.

### What was found

`render/src/texture.c` gives **both** sampling modes `VK_FILTER_NEAREST` for
magnification and minification, and `text/src/font.c:386` asks for one of them
for the glyph sheet. The sheet is therefore point-sampled at **32 texels to the
em** (`ATLAS_EM`, `font.c:131`).

A distance field is resolution-independent for exactly one reason: bilinear
interpolation between neighbouring distances reconstructs a continuous field, so
the shader's threshold can cross it anywhere. **Point-sampled, the field is a
plateau per texel and the threshold can only fall on a texel boundary.** The
sheet stops being a field and becomes what ADR-0076 replaced: a low-resolution
bitmap of the letter, scaled up nearest-neighbour.

The arithmetic matches the report exactly. Card 025 measured Oxanium Regular's
stems at **about two and a half texels** at 32 to the em. Two and a half cannot
be expressed on a whole-texel boundary, so each stem rounds to two or to three
according to where it happens to sit against the grid, and the two sides of an O
sit at different phases. **Three and two. Thirty-three per cent.** He said thirty.

### How it got in, which is not the way I first guessed

Card 026 removed anti-aliasing engine-wide on the principal's direct instruction
during the review of card 025 — *"Everywhere. We dont want it, period. It is
irrelevant if it makes it look like an engine from 1995"* — and its own table has
seven rows. **Six of them are right and were instructed.** Replacing text's
`smoothstep` with `step(0.5, field)` is one of the six: it was meant, it is what
made the text crisp, and this ADR does not touch it.

The seventh row is *texture magnification, `LINEAR` to `NEAREST`*, applied
uniformly to every texture including the glyph sheet. **The card's own reasoning
refutes it.** Its section *What card 025 turns out to have been for* argues that a
hard cut through a field puts the edge on the outline at any size, where a hard
cut through the coverage atlas that preceded it *"would have put the edge on the
nearest texel and the same letter would have come out in blocks the size of the
sheet's texels."* That argument is correct and it is the argument for this ADR:
it holds only while the field is interpolated. Point-sampled, the field behaves
as the coverage atlas it is being contrasted with, and the blocks arrive.

Two smaller things corroborate that this row was a sweep rather than a judgement:

- **The enum's own documentation still describes what was intended.**
  `render/include/render/device.h:227` says of the one-level mode: *"Linear
  magnification and minification of one level. No chain is generated, uploaded or
  sampled."* Linear of one level is precisely correct for a distance field — the
  mipmap chain was the fault, the interpolation was not. The implementation was
  changed and the comment left describing the opposite.
- **ADR-0076 anticipated the distinction in as many words**: *"interpolating a
  distance is meaningful in a way interpolating a colour beside a transparent
  texel is not."*

**Card 026's verification is the one claim in it that is not true.** It reports a
screenshot of *"one letter magnified until it fills much of the screen"* showing
*"straight lines with one-pixel steps rather than texel blocks."* That is the
exact image the principal is now describing as texel blocks. The screenshot was
taken and the wrong conclusion drawn from it — which is worth recording, because
it is not a coder being careless, it is a coder checking for the artifact the card
told it to expect and finding the card's own prediction where the defect was.

### The principal's worry, which decides the shape of this

*"i am worried we go back to the bluriness which we had before we removed the
filters."* It cannot happen, and the reason is structural rather than a matter of
tuning. The shader's line is `float field_alpha = step(0.5, field);` — **`step`
returns 0.0 or 1.0 and there is no third value.** The filter runs before the
threshold and the threshold discards everything but the sign, so no sampler
setting can produce a partially-covered pixel. There is no code path to one.

The blur that was removed had two causes and **linear filtering was neither**: a
generated mipmap chain, and a sheet that stored a picture of coverage. The chain
is deleted, not disabled, and the picture no longer exists.

## Options considered

### Option A — raise the sheet's resolution
More texels to the em shrinks the quantisation without removing it: at sixty-four
the stems land at five and five-and-a-half instead of two and three, so the O gets
less wrong and stays wrong, **at every distance, for the same reason**. It costs
sheet memory as the square, and it puts back the thing the distance field exists
to escape — a sheet that must guess how large the text will be on screen. That
guess is what had `font.c` halving itself from eighty to forty and writing the
reasoning down as a fact about text.

### Option B — filter the distance field, and nothing else
Linear magnification and minification of one level, for the glyph sheet only.
Costs nothing: bilinear is the texture unit's fixed-function path and is the same
price as nearest. The threshold is untouched, so the picture stays a hard cut.

### Option C — accept it
Defensible only if the cost of the fix were real. It is not.

## Decision

**Option B, scoped narrowly.** The deciding principle, and it is the sentence the
next sweep has to read: **a texture whose texels are numbers is filtered; a
texture that is a picture is not.**

- **The engine-wide rule stands unweakened.** Nothing that is a picture is
  filtered, no mipmap chain is generated anywhere, and the 1995 look is kept. This
  is not card 026's *"a card that reintroduces one filter just here"* — that
  sentence guards the **appearance**, and the appearance does not change, because
  alpha has exactly two possible values before this and after it.
- **The exception is carried by a third value, `VOE_RENDER_SAMPLING_FIELD`** —
  linear magnification and minification, one level, no chain, `CLAMP_TO_EDGE`. The
  glyph sheet asks for it and nothing else does. `SMOOTH` and `SHARP` keep
  `NEAREST` and every existing call site is untouched.
- **The exception is unforgeable rather than a default.** A caller has to name the
  field mode to get filtering, and the mode's name and its comment say why it
  exists. A sprite sheet arriving on card 022 asks for `SHARP` and gets nearest,
  which is right.
- **The hard cut is not reopened.** `step(0.5, field)` stays. What changes is
  *where* the boundary falls — on the outline instead of on the nearest texel —
  and not how hard it is.
- **The narrow scope was chosen over the broad one** by the principal, on a
  recommendation. Broad would have made one-level sampling linear generally, which
  is what its documentation already claims. Narrow keeps the strong rule strong
  and writes the reason on the exception itself, where the next sweep hits it.
- **This ADR pairs with an amendment to ADR-0078**, which claims text is
  unaffected by the removal of anti-aliasing. That was true when written and was
  superseded hours later by the principal's own instruction. The amendment records
  the instruction; it is not a new decision.

## Blast radius

**As small as a change gets.** One enum value, one sampler description, one call
site in `font.c`, and the comments in four files that currently argue for the
opposite. Reversing it is the same edit backwards.

Nothing becomes expensive to change. The one thing to watch is the opposite of a
risk: if a future texture is *also* data rather than a picture — a lookup table, a
normal map treated as vectors — it asks for the same mode, and the rule already
covers it without amendment.

Reversibility: **cheap**.

## Consequences

- **The O comes out symmetrical**, and stem widths stop depending on where a glyph
  happens to land against the sheet's grid.
- **Text stays exactly as crisp.** Every pixel is fully letter or fully
  background, at every size, as it is today.
- **A stem is still quantised to whole *screen* pixels.** At a size where a stem
  covers about two and a half pixels the hard cut still rounds it to two or three,
  so two stems can differ by one pixel. That is inherent to having no
  anti-aliasing, it is the trade the principal took deliberately, and it is
  distinguishable from the defect by the test he ran: it shrinks as the letter
  grows and vanishes when the letter is large.
- **Small and distant text is not fixed by this.** The sheet still cannot hold a
  stem thinner than about one and a half texels, and strokes still break into
  specks rather than blurring. That is D-104 and this ADR does not move it.
- **The distance-field error-correction row becomes live for the first time**, and
  its evidence has to be re-read. D-112 was recorded on the observation that
  nothing in the shipped range showed the interpolated-median artifact — but with
  point sampling **there is no interpolation**, so that observation proved nothing
  about it. The artifact class was unobservable, not absent. Whether it appears is
  now an open question with a real chance of an answer.
- **The sampling enum now names combinations across two axes rather than a choice
  on one**, and one combination does not exist. **That is D-103, whose written
  trigger was “the third sampling mode” and which this ADR fires**; card 026 found
  the smaller version of it independently.
- **The card that introduced this is not reopened.** It did what it was
  instructed, its reasoning is sound, and the one row that was wrong was wrong in
  the same direction as an explicit, correct, engine-wide instruction. This is the
  next card, per ADR-0068.

## Rejected options and why

- **Option A — raise the sheet's resolution.** Rejected as treating the symptom.
  It buys a smaller error with memory that grows as the square, never removes the
  error, and restores the resolution-dependence that ADR-0076 exists to end. Worth
  recording that the principal's instinct to *"calibrate this texel thing"* points
  at a real subject — the spread, the corner angle, and the thinnest stem the sheet
  can hold — but that subject governs distant text, is open on its own row, and
  tuning it now would be compensating for a sampling defect and would have to be
  undone afterwards.
- **Option C — accept it.** Rejected because there is nothing to trade. The
  principal asked for *"precision over performance"* and this costs no
  performance; bilinear filtering is fixed-function hardware at the same price as
  nearest.
- **Broad scope — one-level sampling becomes linear generally.** Not wrong, and it
  is what the enum's documentation already claims. Rejected in favour of the narrow
  carve-out so that the engine's rule continues to read absolutely, with the reason
  for the single exception written on the exception.

## Questions this opens

- **D-103 — fired, not opened.** Its trigger was written as *"the third sampling
  mode"*, and this is that mode. The row asks whether sampling and addressing stay
  bundled in one named set; with a third value the set names three combinations of
  two independent axes — addressing (`REPEAT` / `CLAMP_TO_EDGE`) and filtering
  (nearest / linear) — and the fourth has no name because nothing wants it. Card
  026 found the smaller form first and declined to act, correctly, because renaming
  is an API change and a decision. **Worth recording how this ADR met the row**: it
  was drafted opening a new question and the row already existed, unfound because
  it is worded about bundling rather than about names. That is a search failing on
  a register of a hundred rows, and it is the second time in two days the record
  has been ahead of the session reading it.
- **Nothing else.** In particular this does not reopen the mip chain, which card
  026 deleted engine-wide and which is a separate subject with a separate cost.
