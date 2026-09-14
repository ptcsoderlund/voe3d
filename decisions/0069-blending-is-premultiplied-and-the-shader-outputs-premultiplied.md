# 0069. Blending is premultiplied, and every shader outputs premultiplied colour

- **Status:** Accepted
- **Date:** 2026-09-05
- **Deciders:** Human, Tech Lead
- **Supersedes:** —
- **Superseded by:** —
- **Closes:** D-069

## Context

ADR-0061 chose **non-premultiplied** source-alpha blending —
`src.a · src + (1 − src.a) · dst` — on one stated ground: glTF base-colour alpha
is non-premultiplied, so a single non-premultiplied state keeps the importer
honest. It parked the alternative as D-069 with the trigger *"glyph atlas
filtering showing fringes on the text card"*.

**That trigger names the weakest case, and the tech lead's first argument for
this ADR made the same mistake and is corrected here.** A glyph atlas is
monochrome: white everywhere, coverage in the alpha channel. Bilinear filtering
of a constant colour channel produces that constant, so a white-on-transparent
atlas does not fringe whether it is stored premultiplied or not — *provided the
transparent texels are also white*. Fringing is what happens when the colour
channels **vary** and the filter averages a visible colour with whatever was left
in the transparent gaps. So the text card is not the case that decides this.

**And the fringe belongs to a different decision than this one.** Texture
filtering happens in the sampler, before the fragment shader runs, so what fixes
a fringed sprite sheet is how the *texture* is stored — premultiplied, or with
its edge colours bled outward into the transparent gaps. This ADR decides the
blend state and what the shader outputs, and it cannot reach back into the
sampler. Saying otherwise would be claiming a fix this does not deliver; the
texture-side question is D-094 and it arrives with the sprite card.

What decides this ADR is the case that the blend state genuinely owns:

- **A target that is drawn into and then composited again** — the offscreen UI
  panel that ADR-0070 leaves to card 023, and the editor viewport after it. An
  intermediate target holding non-premultiplied colour cannot be composited
  correctly at all: its partially covered pixels have already been blended
  against the panel's own background, and there is no way to recover the surface
  colour in order to blend them a second time. Premultiplied is not the better
  convention here, it is the only one that works.

The decision is therefore due now — before several shaders write to the target
and a convention has to be migrated — and its reasoning is compositing, not
glyphs and not sprites.

Constraints already fixed:

- **ADR-0061's pass shape** — one blended pass after opaque, same colour and
  depth target, depth test on, depth write off, sorted per object. Unchanged
  here; this ADR changes only the blend equation and what the shader hands it.
- **One shader draws everything** (`render/shaders/draw.slang`), whose layout is
  asserted against the C structs in `render/src/descriptors.c`.
  `voe_render_shading_values` already carries `float reserved_a[2]` immediately
  after `metallic` and `roughness` — two slots, on the right boundary, for an
  alpha mode and a cutoff, with no size change and no assert to update.
- **The frame is an offscreen target** (ADR-0051) and the swapchain prefers
  `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR` (`render/src/swapchain.c:112`).
- **Textures are uploaded as the file wrote them** — `assets` decodes and does
  not convert, and `voe_render_texture_kind` says only whether the bytes are
  colour or numbers.
- **No premature generality** (ADR-0008, ADR-0020). Two blend states where one
  will do is exactly what that forbids.

## Options considered

### Option A — non-premultiplied, as ADR-0061 wrote it
Nothing changes today. It is correct for an opaque or cutout surface and for a
blended one drawn straight into the final frame, which is everything card 021a
and card 021b will draw. It becomes unfixable at the offscreen panel: a non-premultiplied intermediate target is not
a convention that can be composited, so card 023 would have to change the blend
state for everyone anyway, after two more folders had been written against the
old one.

### Option B — premultiplied blend state; the shader premultiplies at output
The pipeline blends `src + (1 − src.a) · dst`, and the fragment shader multiplies
colour by alpha as its **last** act. Textures stay exactly as they were decoded
and are sampled non-premultiplied; the whole convention is one line at the end of
one shader. For everything drawn today the result is algebraically identical to
Option A — the same product, computed one step earlier — so nothing about glTF's
meaning or the importer changes. What it makes permanent is a contract:
**anything that writes into this target outputs premultiplied colour.**

### Option C — two blended pipelines, one per convention
Model surfaces blend one way, text and UI the other. Nothing has to be reasoned
about, and it costs a second blended pipeline on the first card that blends, plus
a choice handed to every later drawing folder that they will not all make the
same way — the variant explosion ADR-0061 asked us to watch for, arriving
immediately and for no gain.

## Decision

**Option B**, taken now rather than on D-069's trigger. The deciding factor: the
two conventions are algebraically identical for everything the engine draws
today, and they stop being identical at the offscreen panel, where
non-premultiplied is not merely worse but incorrect — so this is a free
correction that only looks like a trade-off.

Precisely:

- **The blended pipeline's state is `src + (1 − src.a) · dst`** — source factor
  one, destination factor one-minus-source-alpha, for both colour and alpha.
- **The fragment shader's last act is `colour.rgb *= colour.a`.** One line, in
  the one shader, on every path through it.
- **Textures are sampled non-premultiplied and are stored exactly as decoded.**
  The premultiplication happens once, at output, and nowhere else. **A texture
  that already held premultiplied values would be multiplied by its coverage
  twice** — which is the one way to get this wrong, and the reason the rule is
  "the shader premultiplies" rather than "we use premultiplied textures".
- **The alpha mode decides alpha before that multiply, the way glTF specifies:**
  - **opaque** — alpha forced to `1`, ignored entirely, exactly as the glTF
    specification says an `OPAQUE` material's alpha is ignored. **This line is
    what makes the unconditional multiply safe**: without it an opaque material
    whose base colour carries `a = 0.5` writes half its colour into a pass that
    does not blend, and simply goes dark.
  - **cutout** — discard below the cutoff, then alpha forced to `1`.
  - **blended** — alpha as computed.
- **The alpha mode and the cutoff live in `voe_render_shading_values.reserved_a`**,
  which exists for exactly this and needs no layout change.
- **A coverage atlas stores white in the colour channels and coverage in alpha**,
  transparent texels included. That is what keeps a filtered glyph edge clean,
  and it is a property of how the atlas is *written* rather than of the blend
  state. Stated here because the two are easy to confuse and were confused once
  already.

## Blast radius

**Cheap in code, load-bearing as a contract.** The pipeline state is two enum
values and the shader change is one line; reversing them is the same afternoon.

What is not cheap is the contract, and it should be stated where a shader author
will read it: **the colour target holds premultiplied colour, and every shader
that writes into it outputs premultiplied colour, exactly once.** A later
shader — a post-process, the editor viewport's composite, card 023's panel — that
forgets it produces a picture that is wrong only where alpha is strictly between
zero and one, which is the hardest class of error to see. It belongs in
`draw.slang`'s header and in `render`'s public header, not only here.

## Consequences

- **The opaque pass is unaffected in appearance**, because opaque forces alpha to
  one and `rgb * 1` is `rgb`. The change is invisible until something blends.
- **The offscreen panel becomes possible later without changing anything.** That
  is most of what this ADR buys, and it is bought before three folders are
  written against the other convention.
- **The target's alpha channel now means something.** It did not before. Nothing
  reads it: the frame is an offscreen target (ADR-0051) and the swapchain takes
  `VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR` when it is offered
  (`render/src/swapchain.c:112`), so the compositor ignores it.
- **On a surface that offers *only* pre-multiplied composite alpha**, that
  fallback would hand the compositor a frame whose alpha is real, and the window
  would go partially see-through wherever something blended. Nobody here has such
  a surface, the swapchain prefers three other modes ahead of it, and the honest
  answer is to notice it here rather than guess at a fix — the same treatment
  `render/src/texture.c` gives a surface with no sRGB format.
- **The importer stays non-premultiplied and glTF keeps its meaning.** No texture
  is repacked on the way in, and a base-colour texture with an alpha channel is
  stored exactly as the file wrote it.
- **D-069 closes without the observation it was waiting for**, and with its
  premise corrected: the trigger it named would not have fired.

## Rejected options and why

- **Option A — non-premultiplied.** Rejected on the offscreen panel, not on
  glyphs. It is correct for everything the engine draws today, which is exactly
  why leaving it would be a trap: it would be discovered wrong on the card that
  can least afford to change a global convention.
- **Option C — a pipeline per convention.** Rejected as premature generality with
  a recurring cost: two blend states means every future drawing folder picks one,
  and they will not all pick the same.
- **Premultiplying at import.** Would change what a loaded texture *is*, so a
  picture would no longer match the file it came from, and a texture shared
  between a blended and an opaque material would be wrong for one of them. The
  shader is the only place that knows the alpha mode.

## Questions this opens

- **D-090** — where the premultiplied-output contract is enforced rather than
  merely documented, once a second shader exists. Nothing today could catch a
  shader that forgets it, or one that premultiplies twice. Trigger: the second
  shader that writes to the colour target.
- **D-094** — **how a filtered colour texture with varying alpha avoids fringing**,
  which this ADR explicitly does not solve because the sampler runs before the
  shader. The candidates are premultiplied texture storage — which would need the
  material to say which convention a texture is in, so the shader does not
  multiply twice — or bleeding edge colours outward into the transparent texels at
  upload. Trigger: the sprite card, which is the first with a soft-edged colour
  atlas. The glyph atlas sidesteps it by being white everywhere.
