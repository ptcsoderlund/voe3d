# 0082. A sprite sheet does not fringe, because nothing that is a picture is filtered

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-094

## Context

D-094 asked how a filtered colour texture with varying alpha avoids fringing. It
was opened by ADR-0069, which corrected its own premise on the way: premultiplied
*output* from the shader does not fix a fringed texture, because **the sampler
runs before the shader.** The row named two candidate answers — store the sheet
premultiplied and have the material say so, or bleed edge colours outward into
transparent texels at upload — and aimed both at card 022, on the reasoning that
the glyph atlas dodged the problem by being white everywhere and a sprite sheet
cannot.

**The row was written on 2026-09-05 and the engine changed underneath it on
2026-09-06.** Three decisions in one day removed its premise:

- **ADR-0075** made sampling a property of the texture, chosen at creation from a
  named set, and noted in its own consequences that *"a sprite sheet will want
  `SHARP` too, for the same reason a glyph sheet does — neighbouring texels belong
  to different sprites."*
- **ADR-0078**, on the principal's instruction, removed anti-aliasing engine-wide.
  Card 026 implemented it as a sweep: every texture became `VK_FILTER_NEAREST` and
  mipmapping was deleted everywhere.
- **ADR-0079** put back exactly one exception, and stated the rule that governs it:
  **a texture whose texels are numbers is filtered; a texture that is a picture is
  not.** It says in its own decision text: *"A sprite sheet arriving on card 022
  asks for `SHARP` and gets nearest, which is right."*

## Why the question dissolves

**Fringing is an artifact of filtering, and there is no filtering.**

A fringe appears when the sampler averages an opaque coloured texel with a
transparent one and hands the shader a colour that was never in the sheet — the
transparent texel's RGB, usually black or white, bleeds into the edge. Every step
that can produce it is gone:

| Source of a fringe | Status |
|---|---|
| Bilinear magnification and minification | **Absent.** `SHARP` is `VK_FILTER_NEAREST`; no two texels are ever averaged. |
| Mipmap generation averaging RGB across texels of differing alpha | **Absent.** `SHARP` generates no chain, and card 026 deleted mipmapping engine-wide. |
| Wrapping at the sheet's edge, pulling in a neighbouring sprite | **Absent.** `SHARP` addresses `CLAMP_TO_EDGE`. |
| The blend itself, in the shader | **Already decided** — ADR-0069, premultiplied output. |

There is no path from a sprite sheet's texels to a colour that is not in the
sheet. The two candidate answers D-094 offered — premultiplied storage, or an
edge-bleed pass at upload — are both repairs to a filter that does not run.

## Options considered

### Option A — build the edge bleed at upload anyway
A dilation pass over transparent texels, copying the nearest opaque neighbour's
RGB. Standard, cheap, and correct-if-needed. Costs: a pass in `assets` that fixes
nothing today, and — worse — it is invisible, so nobody would ever find out it was
dead code. It also makes the sheet's transparent texels no longer what the artist
authored, which matters the day someone reads them for something else.

### Option B — store sprite sheets premultiplied, and have the material say which convention it is in
Removes the fringe by construction *and* removes the shader's coverage multiply
for these textures. Costs: a convention flag on the material, a second reading of
every base-colour texture, and a permanent way to get double-multiplied colour
wrong — a trap ADR-0069 and card 021b already had to name once.

### Option C — build nothing, and write down why
Record that the sprite sheet asks for `SHARP`, that `SHARP` cannot fringe, and
that the row is closed by decisions already taken rather than by new work.

## Decision

**Option C.** The deciding factor: **the row's premise — that the sheet is
filtered — stopped being true on 2026-09-06, and a repair for a fault that cannot
occur is code nobody can test and nobody will ever delete.**

- **A sprite sheet is created with `VOE_RENDER_SAMPLING_SHARP`**, and that is the
  whole of this decision's implementation. It is one argument at texture creation,
  and card 022 does not have to argue for it.
- **The sheet is uploaded as authored — straight, not premultiplied.** The shader
  multiplies by coverage on output, exactly as ADR-0069 specifies, and there is no
  second convention and no flag saying which one a texture is in.
- **No edge-bleed pass is built**, in `assets` or anywhere.
- **The sprite sheet is a picture, so ADR-0079's rule sends it to nearest without
  an argument.** It is not a distance field and does not ask for `FIELD`.
- **What a sprite edge does instead is crawl**, and that is the stated,
  already-accepted limitation of ADR-0078 — not a defect and not this row's
  problem. A cut-out sprite has a hard edge for the same reason a leaf does.

## Blast radius

**As small as a decision gets — it is a decision not to build.** Reversing it is
writing the ADR that D-094 already sketched, against a sheet that is unchanged on
disk, at whatever cost it has then.

The condition that reopens it is precise and worth stating, because it is the one
way this decision goes stale quietly: **if any picture texture is ever filtered
again, fringing comes back the same day.** Anisotropy for world textures (D-102),
a restored mip chain, or any future softening of ADR-0079's rule from *numbers
only* to something broader would each do it. This is recorded in the register
against D-102 as well, so the row that could cause it carries the warning.

Reversibility: **cheap.**

## Consequences

- **Card 022 loses a question and gains a sentence**: create the sheet `SHARP`,
  upload it as authored, and do not premultiply it.
- **The reason is written where a coder will hit it**, which is the actual work
  this ADR does. Without it, the next person to read card 022 finds a named
  decision about fringing, believes it is unanswered, and builds one of the two
  repairs above against a fault that cannot happen.
- **ADR-0069's corrected premise is now fully discharged.** That ADR caught itself
  claiming the glyph card would show fringes, correctly identified that it would
  not, and filed the real texture-side question as D-094. D-094 turns out to have
  the same shape as the error it was extracted from: the fringe was always
  hypothetical, and it took two more ADRs to make it impossible rather than merely
  unobserved.
- **The consequence we like least:** this is the second row in two days closed by
  *the engine changed under it* rather than by a decision aimed at it. That is
  cheap when caught and expensive when not — a row whose premise has expired reads
  exactly like a row awaiting an answer, and a coder correctly refused to guess at
  this one. **The general lesson is D-115.**
- **`SMOOTH` and `SHARP` currently differ only in addressing** (D-103), so a sheet
  created `SMOOTH` by mistake would not fringe either — today. It would wrap, and
  it would fringe the moment D-103 or D-102 restores a filter. The card names
  `SHARP` explicitly rather than relying on that.

## Rejected options and why

- **Option A — the edge bleed.** Rejected as untestable dead code. There is no
  observation that would distinguish an engine with it from one without, which
  means nobody could ever justify removing it either.
- **Option B — premultiplied storage.** Rejected because it buys nothing and costs
  a convention. A per-texture *which convention is this in* flag is the exact
  shape of the double-multiply bug ADR-0069 exists to prevent, and adding it to
  solve a fault that cannot occur is paying a permanent price for a hypothetical.

## Questions this opens

- **D-115** — whether a register row should **carry the premise it depends on**, so
  that a decision invalidating the premise is caught when it is taken rather than
  when the row is next read. D-094 and D-112 both had their evidence voided within
  a day by ADRs that did not know they were doing it, and D-112's was caught only
  because ADR-0079 happened to reason about interpolation. Candidates: a *depends
  on* column, a cheap sweep whenever an ADR changes engine-wide behaviour, or
  accepting it and catching them at read time as here. Trigger: the third
  occurrence, or the first one caught late enough to cost work.
