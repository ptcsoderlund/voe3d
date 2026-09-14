# 0061. Transparency in v1 is one blended pass, sorted per object

- **Status:** Accepted
- **Date:** 2026-09-04
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-057

## Context

ADR-0024 put transparency on the *later* capability list. ADR-0049 then removed
the escape hatch: there is no screen-space path, so text and UI are quads in the
world, and quads carrying glyphs or widgets are alpha-blended by nature. Both
ADR-0049 and ADR-0059 recorded the same consequence without deciding it — the
first text card wants blending. Card 021 is blocked in substance on this row, and
it unblocks the moment card 017 lands, so the decision is due now.

Constraints already fixed:

- **No screen-space overlay pass** (ADR-0049). Text and UI are geometry in the
  world, subject to everything else geometry is subject to.
- **Reversed depth, one viewport Y flip, right-handed +Y up −Z forward**
  (ADR-0033). Not open, and a blended pass gets no exemption.
- **Solid geometry lives in shared pools, one draw per mesh, indirect-ready**
  (ADR-0059), which states in its own consequences that anything blended must
  leave that path and be sorted.
- **glTF materials are parsed at card 018 and forbidden from adding
  transparency** — the card says so explicitly. glTF already carries the
  vocabulary: `OPAQUE`, `MASK` with a cutoff, `BLEND`.
- **Rendering is written for performance; loading only has to work**
  (ADR-0025). A per-frame sort is a rendering cost and is argued as one.
- **`render` grows by-id functions on demand, driven by a caller in `3d`**
  (ADR-0060). A second pipeline needs no new decision, only a caller.
- **No MSAA exists**, so alpha-to-coverage is not an available answer.

## Options considered

### Option A — transparency stays *later*; text is cutout only
A discard against a cutoff in the shader. Glyph quads stay in the pooled opaque
path, depth writes stay on, no second pass and no sort. Free today, and nothing
becomes permanent. What it costs is the glyph edge: with no blending and no MSAA
there is no anti-aliasing anywhere in the engine, so text — the first thing a
person looks at, starting with the statistics readout — is aliased, and small
text is bad rather than merely imperfect.

### Option B — blending in, in the minimal honest form
One blended pass after the opaque one into the same colour and depth target;
depth test on, depth write off; blended objects sorted back-to-front per object
on the CPU; the mode carried by the material and spelled the way glTF spells it.
Order-independent transparency, per-triangle sorting, transparent shadows and
refraction are refused by name so nobody builds them on the way past. Costs a
second pipeline, a sort, and blended objects stepping out of the single-draw
path — which ADR-0059 already wrote down as the price.

### Option C — transparency as a capability, with order-independent transparency
Weighted-blended OIT so draw order stops mattering. Removes the sort and the
correctness limit in one move, and costs an accumulation and revealage target, a
resolve pass, and a shader path nobody can check by eye.

## Decision

**Option B.** The deciding factor: text is the engine's most visible surface long
before any interesting use of blending exists, and hard-edged glyphs would be a
permanent quality tax paid every day to avoid one pass and one sort — while the
minimal blended pass undoes nothing already built.

What that means, precisely:

- **The material carries an alpha mode with three values, named as glTF names
  them:** opaque, cutout (with a cutoff), blended. This is the vocabulary for the
  whole engine, not a glTF import detail — text and UI materials use the same
  three words.
- **Opaque and cutout draw in the existing pass and stay in the pooled,
  indirect-ready path.** Cutout is a discard against the cutoff, depth writes on,
  no sorting. It is not a lesser blending; it is the right answer for foliage and
  for anything with a hard silhouette.
- **Blended draws in a second pass, after opaque, into the same colour and depth
  target.** Depth test on (reverse-Z, greater), **depth write off**, standard
  non-premultiplied source-alpha blending: `src.a · src + (1 − src.a) · dst`.
- **Sorting is per object, back-to-front, on the CPU, and lives in `3d`** — the
  folder that already walks the tables (ADR-0059). The key is view-space depth of
  the object's origin. **Not per triangle**, and not a spatial structure.
- **Blended objects stay in the same pools and the same component tables.** The
  alpha mode selects which pass walks them; there is no separate storage and no
  second set of buffers.
- **`render` gains a second pipeline and the entry point to draw with it**, added
  under ADR-0060 by its caller in `3d`. No new decision.

**Refused by name, so a passing card does not adopt them quietly:**
order-independent transparency, per-triangle or per-fragment sorting, transparent
objects casting or receiving transparent shadows, refraction and thickness,
dual-source blending, alpha-to-coverage, and any second render target for
transparency. Each is a later decision with its own trigger.

**Card 019 carries it** — the material-shading card, not the text card. The tech
lead's recommendation, agreed by the principal the same day: that is where the
material's alpha mode arrives from the importer, and a blended quad in front of a
cube can be checked by eye before glyph rasterisation adds its own unknowns. Text
then consumes a mechanism that already works.

**Writing the card is the principal's act** (root `CLAUDE.md`), and two
card-level consequences follow from the placement: 019's scope grows by the three
alpha modes, the blended pass and the sort, and **021's `blocked-by` becomes 017
and 019** — today it names 017 alone, so text is claimable before the blending it
needs exists.

## Blast radius

**Moderate, and mostly cheap.** The pipeline state, the second pass and the
shader's cutoff branch are additive and reversible in an afternoon. Two things
are not:

- **The alpha mode as a material field with three values** is the vocabulary the
  importer, the shader, the sort and every later authoring surface use. Changing
  it later touches all of them.
- **The promise that blended objects leave the pooled single-draw path.** When
  the single indirect draw is switched on it covers opaque and cutout only, and
  anything that assumed "one draw for everything solid" means *solid*.

Moving to OIT later is additive: the pass exists, the material vocabulary is
unchanged, and the sort becomes dead code rather than an obstacle.

## Consequences

- **Card 021 unblocks in substance.** Its remaining unknowns — atlas versus
  offscreen panel, the font reader, hinting, subpixel positioning — are its own
  and unaffected by this.
- **Two pipelines where there was one**, and a shader that branches on the
  cutoff. The first non-trivial pipeline variation in the engine, which is where
  a variant explosion starts if nobody watches it.
- **A sort per frame.** Over a handful of blended objects it is free. Sprites are
  the first card where the object count is naturally large, and back-to-front
  sorting of many billboards is exactly the per-frame cost ADR-0025 says to think
  about in advance — it lands on that card, not on this decision.
- **Blended geometry does not get the indirect collapse.** A card that draws
  hundreds of blended billboards will need its own answer.
- **A correctness limit we accept and can see:** two large intersecting blended
  objects sort wrong, because the key is one point per object. This is the normal
  behaviour of every engine that sorts per object, and the failure is visible
  rather than subtle.
- **Depth write off means blended objects do not occlude each other in depth**,
  which is what makes the sort load-bearing rather than cosmetic.
- **Anti-aliasing arrives only inside the glyph atlas.** There is still no MSAA
  and no post-process AA, so world geometry edges stay hard; blended text will
  look better than the cube it sits in front of.
- **This does not answer how a UI quad sits above the world.** That stays open
  (D-056) and is still a depth-and-pass question, not a blending one — blending
  makes the quad see-through, not frontmost.

## Rejected options and why

- **Option A — cutout only.** Not rejected as a technique: cutout is adopted as
  one of the three modes and is the right answer where a silhouette is hard. It
  is rejected as *the whole answer* because it makes text permanently aliased in
  an engine with no other anti-aliasing, and the thing it saves — a pass and a
  sort — is exactly what ADR-0059 already predicted we would add.
- **Option C — order-independent transparency.** Premature generality with
  nothing to measure against, which ADR-0020 and ADR-0055 both forbid, and the
  wrong tool for the motivating case: UI wants exact ordering, and weighted
  blending approximates it. It also costs two extra targets and a resolve before
  a single glyph has been drawn.
- **A separate transparent render target composited afterwards.** Solves nothing
  the shared depth target does not already solve, and doubles the memory ADR-0024
  already accepted once.
- **Premultiplied alpha as the blend convention.** Deliberately not taken now:
  glTF base-colour alpha is non-premultiplied, so a single non-premultiplied
  state keeps the importer honest. It becomes the right answer if glyph atlas
  filtering shows dark fringes — a one-line pipeline change plus how the atlas is
  written. New register row rather than a guess taken today.

## Questions this opens

- **D-069** — whether the blend convention becomes premultiplied alpha. Trigger:
  glyph atlas filtering showing fringes on the text card.
- **D-070** — how many blended objects the per-object CPU sort carries before it
  stops being free, and what replaces it. Trigger: the sprites card.
- **D-071** — how blended geometry is drawn once the single indirect draw is
  switched on for opaque and cutout. Trigger: the first card that draws hundreds
  of blended objects.
- D-056 (a UI quad above the world) and D-062 (how a material instance reaches
  the shader) are both touched and neither is closed. D-062 now has one more
  field to carry.
