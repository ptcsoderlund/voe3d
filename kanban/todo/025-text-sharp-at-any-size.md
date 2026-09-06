# 025 — text sharp at any size

status: todo
claimed-by: -
blocked-by: -

Written by the tech lead. The principal made card-writing for decisions a
**standing** grant on 2026-09-06, together with the rule that **a card spun off
from another says so in its name**. This one is not a spin-off: it is not carved
out of 022, 023 or 024, and card 021b is complete and is not reopened by it. So
it is plain `025`.

**This card exists because the principal reported text as blurry**, and the
answer turned out to be two faults rather than one. Two ADRs stand behind it —
**0075** (sampling is chosen at texture creation) and **0076** (the glyph sheet
is a three-channel distance field) — and both are summarised here, so this card
can be implemented without reading either.

## Goal

Text is sharp at any size and any angle. Walk up to the sign until a letter fills
the screen and its edges are clean; fly away and it does not crawl.

## The one thing to understand before starting

**Two independent things were blurring text, and fixing either one alone makes
something worse.**

1. **`render` has exactly one sampler** and every texture gets it: linear,
   **full mipmap chain**, repeat, anisotropy off. It was written for a cube being
   flown around and it is right for that. For a glyph sheet it means text is read
   out of pre-blurred levels — and because everything is in metres through a
   perspective camera with no pixel snapping (there is no screen-space path and
   there is not going to be one), text is **permanently** a fraction into that
   chain rather than resting on level zero.
2. **A stored picture of coverage cannot be sharp at a size it was not baked
   for**, and there is no such size here.

**The halves must land together and this card is not finishable in halves.** The
mip chain was doing one useful job: hiding aliasing on distant text. Fix the
sampler alone and distant text goes from soft to **crawling**, which is worse
than the fault being fixed. The distance field is what makes unmipped text safe,
because it recovers its own edge width from screen-space derivatives. Do not put
one half in `review/` and call it progress.

## A correction to make, not a bug to fix

`text/src/font.c` carries a long, careful comment arguing that a sheet stored
*finer* than the screen comes back *blurrier*, and drops `ATLAS_EM` from eighty
pixels to the em to forty on that reasoning, recording that eighty "is visibly
soft at the sizes dev draws".

**The argument is correct and it describes the sampler, not text.** It is a right
diagnosis applied to the wrong object, and card 021b is not at fault for it — it
was reasoning correctly about the filter it was given. This card removes the
fault, which makes the comment false.

**Replace that comment; do not leave it standing and do not merely delete the
number.** A reader who finds it later will re-derive a workaround for something
that no longer happens. Say what the sheet's resolution now means and why it is
no longer a function of the screen.

## Scope — `render`

- **`voe_render_texture_create` takes a sampling mode** beside the kind it
  already takes. Two values, and the enum is closed until something asks for a
  third:
  - **`VOE_RENDER_SAMPLING_SMOOTH`** — exactly what exists today: linear
    magnification and minification, `VK_SAMPLER_MIPMAP_MODE_LINEAR` over a
    generated chain, `REPEAT`, `maxLod` unclamped.
  - **`VOE_RENDER_SAMPLING_SHARP`** — linear magnification and minification,
    **no chain**, `maxLod` 0, `CLAMP_TO_EDGE`.
- **The mode decides mip *generation*, not only mip sampling.** A `SHARP` texture
  uploads **one level**. Clamping `maxLod` alone would leave the levels built,
  blitted and resident for nothing — `mip_levels_for` and `can_generate_mipmaps`
  are gated on the mode, not just the format.
- **`CLAMP_TO_EDGE` on `SHARP` is not decoration.** An atlas is not tiled, and
  `REPEAT` lets a glyph at the sheet's edge wrap into one on the far side.
  Padding hides that today.
- **One sampler per mode, created once** at device start beside the existing one.
  The descriptor write at `render/src/texture.c` already reads a **per-slot**
  `sampler` field — the shape for more than one sampler is already there and has
  only ever had one value put in it. The slot remembers its mode; the descriptor
  path does not change shape.
- **Every existing call site passes `SMOOTH`** and nothing in the world changes.
  This is checkable by screenshot and should be checked that way.
- **Anisotropy stays off and stays unrequested.** It is a device feature that must
  be asked for at device creation and nothing has asked; it is also moot for a
  texture with no chain to select from. If this card finds itself wanting it,
  that is a finding to report, not a feature to take.

## Scope — `text`

- **The sheet holds three signed distances per texel, not coverage.**
- **Distances come from the flattened line segments `raster.c` already
  produces** — distance from a texel centre to a segment, exact — **not** from a
  distance transform over a rasterised bitmap. The existing flattener and the
  existing non-zero winding rule give the magnitude and the sign respectively.
  This is why the three-channel version is only a little more work than the
  one-channel version would have been: the distance machinery is shared.
- **Edge colouring is the hard part.** Each outline edge is assigned one of three
  channels such that **the two edges meeting at a corner sharper than a stated
  threshold land in different channels**. The shader then takes the **median** of
  the three, which reconstructs the corner exactly where two of the three agree.
  Along a smooth run the assignment may persist; it is only corners that must
  differ. The corner threshold is a constant this card chooses and **states in
  the code the way `ATLAS_EM` was stated**.
- **The spread — how far out from the outline distance is encoded, in texels —
  replaces `ATLAS_PAD`.** Padding between glyphs must be **at least** the spread
  or neighbouring glyphs' fields run into each other. Packing changes with it.
  The spread is the second constant this card chooses and states.
- **The sheet uploads as `VOE_RENDER_TEXTURE_DATA`, not
  `VOE_RENDER_TEXTURE_COLOUR`.** Distances are numbers. Uploading them as colour
  runs the sRGB decode over them and puts every edge slightly in the wrong
  place — wrong subtly and everywhere rather than obviously, which is the bad
  kind of wrong.
- **The sheet asks for `VOE_RENDER_SAMPLING_SHARP`.**
- **The sheet gets smaller, not bigger.** It no longer has to guess the screen's
  scale, so it is sized to hold the *shape*. Pick the number, state why, and do
  not carry forty forward out of habit.
- **Nothing else in `text` changes.** The TrueType reader, the composite-glyph
  walk, the UTF-8 walk, layout, metrics, the mesh, four vertices and six indices
  per glyph, one block one mesh one draw — all untouched. This is a smaller change
  than its effect suggests.

## Scope — the shader and the material

- **A third uniform branch in `draw.slang`:** whether the base colour texture is a
  distance field. There is a standing watch to reconsider this whole approach at
  the **fourth** branch; this card spends the third knowingly. Do not add a
  fourth without reporting it.
- **Alpha is computed, not sampled.** Median of the three channels → signed
  distance; the smoothing half-width comes from the **screen-space derivative** of
  that median (`fwidth`, or explicit derivatives); `smoothstep` across it gives
  alpha. Colour is the material's base colour factor. The standing
  `colour.rgb *= colour.a` at output then applies **unchanged** — do not add a
  second multiply, and do not premultiply the sheet.
- **The flag rides in `reserved_c`**, which has three spare words —
  **same offsets, same size, `render/src/descriptors.c` untouched**, the trick
  cards 021a and 021b already used on `reserved_a` and `reserved_b`.
- **`voe_render_shading_values`'s own comment currently says the base colour
  texture is always uploaded as `VOE_RENDER_TEXTURE_COLOUR`** and warns that
  putting a data texture in a colour slot is a picture subtly too dark. That
  invariant now has one exception, governed by this flag. **Update that comment**
  — leaving it is worse than never having written it.

## Where this card is likely to go wrong

- **The edge colouring, and it fails quietly.** A mis-assigned corner is a small
  notch, not a crash, and it will survive every test that is a whole word. **The
  test must be a corner, not a letter.**
- **Shipping half of it.** See above. Distant text that crawls is a regression.
- **Uploading the sheet as colour.** Every edge lands slightly wrong, uniformly,
  and it looks like the spread being off rather than like a format mistake.
- **Multiplying by alpha twice**, exactly as card 021b warned: text that is thin
  and washed out rather than obviously broken. The shader computes alpha and then
  premultiplies once.
- **Leaving `font.c`'s eighty-versus-forty comment in place.** It becomes false
  the moment `SHARP` exists.
- **Assuming a texture kind implies a sampling mode.** They are two independent
  properties. A data texture is not automatically sharp, and the metallic-
  roughness and occlusion textures stay `SMOOTH`.
- **Very small text will still be poor.** When a stem is thinner than a texel the
  field has nowhere to put it and the stem thins or breaks — and the mip chain
  that was blurring that into something inoffensive is now gone. **This is a
  known open question and it is not this card's to solve.** Note what you see and
  move on; do not invent a fix for it here.

## Verify

- **A letter walked up to until it fills much of the screen has clean edges** —
  this is the whole point of the card.
- **Sharp corners are sharp.** Oxanium's flat-cut terminals and squared joins are
  what to look at; a single-distance field would round them and that is the
  failure this card's three channels exist to prevent.
- **Text at a distance does not crawl or shimmer** as the camera moves. This is
  the check that the two halves landed together.
- **Text at a steep angle is legible and not softened in both directions.**
- **Nothing in the world changed.** Screenshots of the cubes, the models and the
  blended quads before and after, with every other texture on `SMOOTH`.
- **The heads-up line and the sign both still work**, in the overlay layer if 024
  has landed by then, in the world if it has not.
- **Text is still not lit** and still arrives at the tint colour it asks for — the
  double-multiply check from card 021b, which this card is capable of
  reintroducing.
- **A program that draws no text still builds no sheet.**
- **The atlas is still built once**; a second block with the same font builds no
  second one.
- **The corner threshold and the spread are stated in the code with their
  reasoning**, and `font.c`'s superseded comment is replaced rather than deleted.
- `cmake -P check.cmake` exits zero. Windows is the principal's.

## Refused, each a later card rather than a judgement call

Anisotropic filtering — a device feature nobody has requested. A third sampling
mode. Hinting. Subpixel or LCD rendering, wrong the moment a quad turns.
Sub-texel positioning. Text that changes after it is built — still blocked, so
**the statistics readout stays on the console** and this card does not move it.
Edge smoothing anywhere else in the engine: text carries its own before and after
this card, and cut-out foliage still has none. Analytic curve evaluation in the
fragment shader — genuinely exact, resolution-independent, and written down as
**the right second move** if this proves insufficient; it is not this card, and
starting it would be a much larger one.

## Report when this lands

- **What the corner threshold and the spread ended up as**, and how they were
  arrived at. They cannot be derived from the decision and were always going to
  be found by looking.
- **How bad small text actually is** now that the mip chain is gone, with a
  screenshot. This feeds the open question and is the only evidence anybody will
  have.
- **The third branch in `draw.slang` is spent.** Say so, so the watch on the
  fourth is accurate.
- **Whether `SMOOTH` and `SHARP` were enough**, or whether anything wanted a
  combination the two named modes cannot express.
