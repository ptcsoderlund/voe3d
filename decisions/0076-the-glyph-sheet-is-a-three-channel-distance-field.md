# 0076. The glyph sheet is a three-channel distance field, not a picture of coverage

- **Status:** Accepted
- **Date:** 2026-09-06
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Amended 2026-09-06, same day:** a new section, *Where the sign comes from*, after the decision, and a paragraph in *Consequences* on how much error correction was built. Written after card 025 implemented this and its coder challenged one sentence of the card as unimplementable. **No part of the decision changed** — the challenge was right, the sentence it corrects was the card's and not this ADR's, and what is added here is the mechanism this ADR always meant.

## Context

D-091 asked whether the glyph atlas becomes a signed distance field, and named
its trigger as *"the first card that wants text in the world at a scale the atlas
was not built for."* No such card exists. **The principal met the question as a
defect instead** — text is blurry, and the ask was crisp at any size, *"even if
it's not totally correct."* That is the second time a text-shaped row has been
closed by the principal reporting a symptom rather than by a card reaching it
(ADR-0074 was the first), and it is worth noticing that the trigger conditions
written on these rows keep firing later than the principal's eye does.

Two faults were producing the blur. **ADR-0075 is the other one** and is not
restated here: the engine had one sampler, built for a flown-around cube, and the
glyph sheet was being read out of a mipmap chain. That is fixed there. This ADR
is about what remains once it is: **a stored picture of coverage cannot be crisp
at a size it was not baked for**, and there is no size it was baked for, because
ADR-0049 puts everything in metres through a perspective camera with no pixel
snapping and no screen-space path. Text is permanently resampled at a fractional
offset. Raising the sheet's resolution does not fix this and, before ADR-0075,
made it worse.

Constraints already fixed:

- **The sheet is the only anti-aliasing in the engine.** No MSAA, no
  post-process; ADR-0072 found this and D-096 carries it. Whatever replaces
  coverage must carry its own smoothing or text gets worse, not better.
- **One text block is one mesh, one material, one draw** (ADR-0070). Load-bearing
  and untouched by this.
- **Text is built once and cannot change** (ADR-0070, D-092). Untouched: the
  sheet is still built when the font is first asked for.
- **Premultiplied output** (ADR-0069): the shader's last act is
  `colour.rgb *= colour.a`.
- **Unlit is a material property** (ADR-0071), and `draw.slang` carries **two**
  uniform branches with a standing watch to reconsider at the fourth (D-093).
- **Write it ourselves** (ADR-0023). There is no library to reach for; the
  builder is ours.
- **The outline is already flattened to line segments** by `text/src/raster.c`,
  at a stated tolerance, with non-zero winding.

## Options considered

### Option A — keep the coverage bitmap, and let ADR-0075 be the whole fix
Free. Removes the mip-chain blur and the oblique blur, which is a large part of
what the principal is seeing. Leaves magnification exactly where it is: a letter
stored at forty pixels and walked up to until it covers two hundred is bilinearly
soft, and after ADR-0075 the same letter far away now aliases. It answers the
complaint at one distance band and makes another worse.

### Option B — a single-channel signed distance field
Each texel stores distance to the nearest outline edge instead of coverage. The
shader thresholds it and takes its smoothing half-width from the screen-space
derivative of that distance, so the edge is mathematically sharp at any
magnification and correctly softened at any minification, out of one small sheet.
The cost is inherent and well known: **one distance per texel cannot describe two
edges meeting**, so sharp corners are rounded off at roughly the texel scale.
Cheapest of the three that actually answer the question.

### Option C — a three-channel distance field, median in the shader
The same distances, carried in three channels, with outline edges assigned to
channels such that the two edges meeting at a sharp corner land in *different*
ones. The shader takes the median of the three, which reconstructs the corner
exactly where two of the three agree. **Identical runtime cost to Option B** —
one sample, one median, one smoothstep — and an identically sized sheet. The
whole difference is in the builder: an edge-colouring pass before the distances
are computed.

## Decision

**Option C.** The deciding factor: the fidelity the principal offered to give up
is hinting and metric perfection, not blunted corners — and Oxanium is a
squared-off face with flat-cut terminals, so rounded corners is precisely the
artifact he would be looking at on a heads-up line every frame, having asked for
the opposite.

- **The sheet holds distances, and distances are data.** It is uploaded as
  `VOE_RENDER_TEXTURE_DATA`, **not** `VOE_RENDER_TEXTURE_COLOUR`. Getting this
  wrong runs the sRGB decode over the distances and puts every edge in slightly
  the wrong place — wrong subtly and everywhere rather than obviously, which is
  the bad kind of mistake.
- **Alpha is computed in the shader, not sampled.** Median of the three channels,
  thresholded with a smoothstep whose half-width comes from the screen-space
  derivative of that median. The colour is the material's tint. The standing
  premultiply at output (ADR-0069) then applies unchanged, and ADR-0069's
  reasoning survives for a better reason than before: interpolating a distance is
  meaningful in a way interpolating a colour beside a transparent texel is not,
  so there is nothing for a filtered edge to pick up.
- **This spends the third uniform branch in `draw.slang`**, where D-093's watch
  reconsiders at the fourth. Said here so the watch is not surprised: the branch
  is *is this base colour texture a distance field*, and it sits beside alpha mode
  and unlit.
- **Distances come from the flattened segments, not from a distance transform
  over a bitmap.** `raster.c` already produces the segments; distance to a segment
  is exact and simpler than transforming an image, and it is the reason Options B
  and C share most of their machinery.
- **The spread — how far out from the outline distance is encoded, in texels —
  replaces `ATLAS_PAD`.** Glyph packing changes with it.
- **The sheet gets smaller, not bigger.** Resolution independence is the point;
  the sheet no longer has to guess the screen's scale, so it is sized to hold the
  *shape* rather than the pixels. `font.c`'s `ATLAS_EM` of forty and the long
  comment defending it are superseded whole — that comment describes a fault
  ADR-0075 removes, and leaving it standing would have the next reader re-derive
  a workaround for something that no longer happens.
- **The error correction is a sign check and nothing more.** msdfgen ships a
  larger pass that also hunts for texels where the *interpolated* median between
  two individually correct neighbours produces an artifact. Card 025 built only
  the per-texel sign repair described above, and nothing in the shipped range
  showed an interpolation artifact. That is a deliberate stopping point, not an
  oversight: the fuller pass is its own body of work with its own threshold to
  justify, and it is recorded as D-112 so the next person meets it as a known
  omission rather than rediscovering the subject from a symptom.
- **Anti-aliasing still lives in the sheet.** The engine still has no other kind,
  cutout foliage still has none, and D-096 is untouched by this.

## Where the sign comes from — added by amendment, 2026-09-06

Card 025 told its coder that *"the existing flattener and the existing non-zero
winding rule give the magnitude and the sign respectively."* **Read literally
that sentence destroys the decision above**, and the coder said so rather than
building it: if all three channels take their sign from one inside/outside test
per texel, the three differ only in magnitude, the median is a magnitude wearing
a shared sign, and every corner rounds off exactly as a one-channel field does —
which is Option B, the option this ADR rejected.

**The sign is per edge, and the channels are allowed to disagree.** Each
channel's sign is which side of its own edge's line the texel falls on, taken
from that edge's direction along the contour. That is the same fact the non-zero
winding rule is built on — a contour's direction is what makes inside inside, and
`cross()` in `raster.c` reduces it to `+1` or `-1` for both halves of the file —
but it is asked per edge and not per texel. **The disagreement near a corner is
the mechanism, not an error in it**: two channels agree on the correct side while
the third is the one that would have rounded the corner, and the median discards
it.

**The winding rule is still used, once per texel, as a repair.** Where the
median's sign disagrees with the fill rule by more than half a texel, the three
channels are replaced by the plain signed distance. Contours that overlap can
otherwise leave three one-sided fields agreeing on a side the shape is not on,
which shows up as a hole or a blob inside one letter. The half-texel deadband
exists so that texels sitting *on* the outline — the ones that carry the edge —
are never touched by the repair.

## Blast radius

**Moderate, and it lands in two places that change together.** What the sheet
contains and how the shader reads it are one decision and must be reversed as
one. Reversing means going back to coverage, restoring a pixels-per-em tuning,
and — because of ADR-0075 — deciding again what to do about minification.

What is **not** at risk, and this is most of the folder: the TrueType reader, the
outline flattener, the layout, the metrics, the mesh, the one-block-one-draw
rule, and the premultiplied contract. This changes what goes into the sheet and
four lines of shader. It is a smaller change than its effect suggests.

## Consequences

- **Text is sharp at any size and any angle**, from one sheet, with no second
  representation and no re-baking.
- **A base colour texture now means one of two things**, and a material that sets
  the flag wrongly renders badly in a way that looks like a shading bug: a
  distance field read as coverage is a grey smear, coverage read as a distance
  field is a hard-edged mess. Two textures cannot be told apart by looking at
  them, only by the flag.
- **Very small text still degrades, and a distance field does not save it.** When
  a stem is thinner than a texel the distance field has nowhere to put it and the
  stem thins or breaks. This is the case the mip chain would have helped with and
  ADR-0075 removed. It is D-104 and it is a real limit, not a tuning matter.
- **The engine gains a shader that reads screen-space derivatives for shading**
  rather than only for choosing a mip level. Nothing else does this yet.
- **The edge-colouring pass is the hardest code in `text`** — harder than the
  composite-glyph walk that was the last card's named trap. Assigning three
  channels so that every sharp corner is spanned by two of them, without two
  adjacent edges colliding on a long smooth contour, is subtle and it fails
  *quietly*: a mis-coloured corner is a small notch, not a crash. It needs a test
  that is a corner, not a letter.
- **The whole thing is invisible in a program that draws no text**, unchanged
  from ADR-0070: the sheet is still built when a font is first asked for.
- **Card 021b in `review/` is not reopened.** It did what it was briefed to do and
  its atlas reasoning was correct against the sampler it was given. This is the
  next card, per ADR-0068.

## Rejected options and why

- **Option A — sampler fix only.** Rejected as an answer, kept as half the work.
  ADR-0075 lands either way. On its own it improves one distance band, leaves
  magnification exactly as blurry, and makes distant text alias where it used to
  blur — so shipping it alone would trade a complaint for a worse one.
- **Option B — single-channel distance field.** Rejected on the specific font and
  the specific consumer. It is the right answer for a humanist face at small
  sizes and it is cheaper to build. Against a squared-off geometric face on a
  camera-locked line, the corner rounding is the first thing the eye lands on,
  and it costs the same at runtime as the version that does not have it.
- **Glyph outlines tessellated into triangles** — raised by the principal and
  rejected before this decision was framed. It trades content-priced smoothing
  for a frame-priced one: the sheet's coverage is the only anti-aliasing in the
  engine, geometric edges have none, and buying it back means MSAA — a cost on
  every pixel of every frame, which is what ADR-0066 exists to prevent. It also
  turns each letter into tens of sliver triangles, which is the shape a rasteriser
  handles worst, and needs a robust triangulator for real font outlines that we
  would be writing ourselves.
- **Analytic curve evaluation in the fragment shader** (the Loop-Blinn and Slug
  family) — genuinely exact rather than approximate, resolution-independent,
  corners perfect, and still one quad per glyph so ADR-0070 survives it. Rejected
  as the *first* answer under ADR-0034: it is considerably more shader work and a
  per-glyph curve buffer, to fix an error the three-channel field does not
  visibly make. **It is the right second move** if the distance field is ever
  judged insufficient, and it is written down here so that is not re-derived.

## Questions this opens

- **D-104** — what happens to text small enough that a stem is thinner than a
  texel. A distance field does not solve minification below its own resolution,
  and ADR-0075 removed the mip chain that was blurring it into something
  inoffensive. Candidates: a mip chain on the distance field, which is more
  defensible than on coverage but breaks the median at exactly the corners
  Option C was chosen for; a distance-to-alpha widening at small scales; or
  accepting it. Trigger: the first text placed far enough away to matter.
- **D-105** — the two numbers that decide the picture: the corner angle above
  which the edge colouring treats a join as sharp, and the spread in texels. Both
  are builder constants that cannot be derived from anything in this ADR and will
  be arrived at by looking. Trigger: the card, and worth stating in the code the
  way `ATLAS_EM` was.
- **D-106** — whether the offscreen panel on card 023 draws its text through this
  same material path or through the panel's own. The panel is rendered once at a
  known size, which is the one place in the engine where coverage would have been
  correct all along. Trigger: card 023.
