# 022 — 2D and sprites

status: todo
claimed-by: -
blocked-by: 021a

> **STILL PROVISIONAL, but less so.** Written far ahead at the principal's
> request. Blending, the sort and the unlit material below it are decided now, so
> what remains open here is genuinely this card's own. Expect to refine it, not
> rewrite it.

**`blocked-by` changed from `021` to `021a`** when the old text card was split
(ADR-0068). Sprites need the blended pass, not glyphs — so this card no longer
waits on text and can be pulled forward ahead of it.

**Amended by the tech lead 2026-09-06** for ADR-0074, the layer decision — an
exception to *the human writes the card* given for that decision and not
standing. `blocked-by` is unchanged: sprites still need only the blended pass.

## Goal

2D sprites, as billboarded quads in 3D space.

## The shape is already fixed

**Billboarding, not a flat drawing path** (ADR-0049 — the principal's own words:
*"2D games have to do billboarding sprites as textures"*). A sprite is a quad in
the world facing the camera. There is no orthographic screen-space renderer and
there will not be one — **reaffirmed on 2026-09-06 against the tech lead, who
proposed one and was overruled** (ADR-0074).

**And now, three more things are fixed that were open when this card was
written:**

- **Blending exists** (ADR-0061, card 021a): three alpha modes named as glTF
  names them — opaque, cutout with a cutoff, blended — with blended drawn in a
  second pass, depth test on, depth write off, sorted back-to-front per object on
  the CPU. A sprite picks a mode; it does not invent one.
- **Unlit is a material flag** (ADR-0071), which is what a sprite usually wants.
  No shader and no pipeline for this card to add.
- **The shader outputs premultiplied colour** (ADR-0069). A sprite's texture is
  stored as decoded and is **not** premultiplied — see the fringe question below,
  which is this card's to answer.
- **There is a layer, and a sprite may be in it** (ADR-0074, built by card 024).
  An object says whether it belongs to the world or above it; above it means the
  world is drawn, depth is cleared, and the overlay is drawn into the same
  picture, **through the same camera and still in metres**. A sprite used as a
  health bar or a crosshair goes there and stops being covered by walls; a sprite
  that is a character in the world does not. **Both are ordinary and neither is
  the default** — if this card finds itself treating one as the normal case, that
  is a finding. Nothing about billboarding, blending or the sort changes inside
  the layer.

## Rough scope

- A quad facing the camera. **Decide which billboarding**: full spherical (faces
  the camera entirely) or cylindrical (rotates about up only). They look
  different and the difference matters for anything standing on ground.
- Texture atlas or texture array for sprite sheets.
- **Batching**, because this is the first card where the object count is
  naturally large. *Instancing and batching* is on the *later* capability list —
  check whether this card is what promotes it.

## The three questions this card was always going to inherit

Named now so they are not discovered:

- **D-070 — when the per-object CPU sort stops being free.** Card 021a sorts
  blended objects back-to-front every frame over a handful of them, where it
  costs nothing. This is the first card where the count is naturally large, and
  ADR-0025 says a per-frame rendering cost is argued in advance rather than
  measured in surprise. Sorted insertion, a radix on a quantised key, or leaving
  it alone with a number attached — but with a number.
- **D-071 — blended geometry and the single indirect draw.** Blended objects are
  outside the pooled single-draw path by ADR-0061. Hundreds of blended billboards
  is the case that makes that matter.
- **D-094 — fringing on a soft-edged sprite sheet.** Texture filtering happens in
  the sampler, before the shader, so the premultiplied *output* decided by
  ADR-0069 does not fix it. Either the texture is stored premultiplied — and then
  the material has to say so, or the shader multiplies by coverage twice — or the
  edge colours are bled outward into the transparent texels at upload. The glyph
  atlas dodged this by being white everywhere; a sprite sheet cannot.

## Known unknowns

Whether a 2D game built on this wants a fixed pixel-to-world scale and where that
lives; whether sprite animation is a component or a texture-coordinate trick.
Neither is answerable yet.

**The pixel-to-world question got sharper on 2026-09-06 and is worth restating.**
ADR-0074 refused an orthographic layer measured in pixels, so there is nowhere in
this engine that a sprite is *natively* a number of pixels — an overlay sprite has
a position and a size in metres like everything else. A 2D game that wants one
texel to be one pixel therefore needs that relationship worked out from the
camera and the target size, by something, somewhere. **This card does not have to
answer it**, but it is the first card in a position to notice what the right shape
would be, and noticing is worth more than guessing.
