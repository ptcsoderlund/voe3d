# 025 — text sharp at any size

status: complete
claimed-by: claude-opus-5 (kanban-coder)
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


---

# Notes — the coder's report, 2026-09-06

Verified on **Linux** (Wayland, Fedora, clang 22). Windows is the principal's and
nothing here was run on it.

`cmake -P check.cmake` **exits zero**: twenty-one steps, all `ok`, including the
eleven standalone configurations, 31 tests passed, and the analyser over 86 files
with no finding.

**Questions raised, none of them blocking:**
`kanban/025-questions-for-the-tech-lead.md`. Six entries, each saying which
reading was taken and what a different answer would cost. The two worth the tech
lead's time are **where the field's sign comes from** — this card's own sentence
about the winding rule cannot be taken literally without losing the corner
reconstruction, and the reading taken is written out there — and **whether the
now-uncalled coverage rasteriser should be deleted**.

## What the two constants ended up as, and how

**The spread is 4 texels** (`VOE_TEXT_FIELD_SPREAD`, in `text/src/raster.h`), so a
byte spans ±4 and the halfway mark is the outline. Arrived at from both ends
rather than by looking: it has to be wide enough that minified text still finds a
gradient to smooth across — one screen pixel spanning k texels moves the encoded
value by k/(2·spread), so four keeps the field meaningful down to about eight
texels to the pixel — and narrow enough that a byte resolves it, which at four is
a thirty-second of a texel. Every texel of it is padding around every glyph, and
that is what stopped it going higher.

**The corner threshold is 3 degrees** (`VOE_TEXT_FIELD_CORNER_DEGREES`). This one
had a trap in it worth recording: **the threshold cannot be applied to the
flattened pieces.** A curve broken to a tenth of a pixel turns by *tens of
degrees* per piece at these sizes, so any small threshold applied there calls
every curve a string of corners and the colouring becomes noise. The fix was
structural rather than numerical — the flattener now numbers the `glyf` edge each
piece came from, and the corner test asks about the join between one outline edge
and the next. At that level a smooth join turns by very nearly nothing and a real
corner turns by tens of degrees, the two populations are far apart, and three
degrees sits between them with room to spare. It is also msdfgen's default, for
the same reason.

**And `ATLAS_EM` is 32, down from 40.** The number answers a different question
now — how finely the *shape* has to be sampled, not how many pixels a letter will
land on — and the answer is set by the thinnest feature: Oxanium Regular's stems
are about 0.08 em, so 32 texels to the em is about two and a half texels of stem.
The range still packs into 512 square with margin 4 around every glyph.

## The third branch in `draw.slang` is spent

Confirmed. The uniform branches are now: the alpha mode, `unlit`, and
`base_colour_distance_field`. **The watch on the fourth is accurate and nothing
here added a fourth.** The distance-field alpha is computed *outside* every
branch — `fwidth` needs uniform control flow across the quad, and the quad
straddling a glyph's edge is precisely the one that matters — so the branch only
picks between two already-computed answers.

## Whether `SMOOTH` and `SHARP` were enough

**Yes, and nothing wanted a combination they cannot express.** Every existing
call site is `SMOOTH` and one test was switched to `SHARP` to prove a reused slot
takes the new mode as well as the new format. Anisotropy was never wanted:
`SHARP` has no chain to select from, and `SMOOTH` did not ask.

One shape did come up and was resolved inside the two modes: **the mode is
remembered per texture slot**, so the descriptor write picks
`device->samplers[slot->sampling]` and stays one loop over the whole table. An
unclaimed slot falls back to slot 0, which is explicitly `SMOOTH`. See question 5
in the questions file — the card's description of the existing per-slot `sampler`
field was not quite what the code had, and this is the shape that replaced it.

## How bad small text actually is

**Worse where it fails, and it fails visibly rather than softly.** The dev sign
seen from across the scene *at a steep angle* — the hardest case in the program —
now has thin, occasionally broken strokes where it used to have a soft
illegible blur. Neither is readable; the new one looks broken and the old one
looked smeared. Everything at ordinary sizes is better by a wide margin.

This is the case the card predicted and refused, and nothing was invented for it.

## What was verified by looking, and how

The dev program was run on Linux and screenshotted before and after the change,
at the same point in its orbit, with the same window size.

- **A letter walked up to until it fills much of the screen has clean edges** —
  the headline claim, and the difference is not subtle. `HUD_EM` was temporarily
  raised from 0.055 to 0.55 and the string set to `Ag` to get a letter that
  large; both were put back and are not in the diff. Before: the apex of the `A`
  is a soft blob three or four pixels of gradient wide. After: a clean flat-cut
  apex with square corners and a one-pixel edge.
- **Sharp corners are sharp.** Oxanium's flat-cut terminals and squared joins
  come out square at that size. This is the three channels doing their job; a
  single-distance field rounds them and that is what the `A`'s apex looked like
  before.
- **Text at a steep angle is legible and not softened in both directions** — the
  sign is at a steep angle throughout and reads better after than before at every
  size it is readable at.
- **Nothing in the world changed.** The cubes' `F`, the model's albedo and ORM,
  and the two blended quads are pixel-alike between the two runs, allowing for
  the sun having moved. Every other texture is on `SMOOTH`.
- **The heads-up line and the sign both still work**, in the overlay layer — 024
  has landed.
- **Text is still not lit** and arrives at the tint colour it asks for. The
  double-multiply from card 021b was not reintroduced: the shader computes alpha
  and `voe_render_premultiplied` multiplies once, and nothing in the sheet is
  premultiplied because there is nothing in it to premultiply.
- **A program that draws no text still builds no sheet** — unchanged, the sheet
  is still built by `voe_text_font_new` and nowhere else.
- **The sheet is still built once**; the second text block with the same font
  builds no second one. Unchanged.
- **The corner threshold and the spread are stated in the code with their
  reasoning**, in `text/src/raster.h`, and `font.c`'s superseded eighty-versus-
  forty comment is replaced rather than deleted — the replacement says what the
  resolution means now and says explicitly that the old argument described the
  sampler and is not to be re-derived.

The screenshots are not committed: they are `.png` and this repository holds no
binary evidence anywhere, which is not a convention to change from inside a card.
They were handed to the principal directly.

## The test is a corner, not a letter

`text/tests/raster.c` gained four field cases and its header says why they are
shaped the way they are. The claims are made about the **median**, because the
median is what the shader reads and a channel on its own means nothing — and the
one claim about the three separately is that at a corner they are not all equal,
measured *off* the diagonal, because on the diagonal they legitimately agree.

The load-bearing case is `check_a_corner_is_not_rounded_off`. Diagonally outside a
right angle an ordinary distance field gives 2.12 texels — the distance to the
corner *point*, which is what rounds it into an arc — and the median gives 1.5,
the distance to the nearer edge's *line*, which is what makes two lines cross at
the corner. **Those two numbers are the two designs' answers and no rounding gets
from one to the other**, so the case cannot pass with a one-channel field behind
it. Every number in the file was worked out on paper before it was run; the two
that were wrong the first time were wrong in my arithmetic and were corrected to
the derived value rather than to the observed one.

## Markers left behind

**None.** No `DEVIATION:` and no `BLOCKED:` in the diff. The two places where a
reading had to be taken are questions 1 and 2 in the questions file, and both are
readings of this card rather than of a rule.


---

# Follow-up, 2026-09-06 — the principal reported the heads-up line as not sharp

**Measured, and the text is as sharp as the design allows. The complaint was
real and the cause is contrast, not blur.** A dark panel was added behind the
line in `dev` so the question can be settled by looking rather than by argument.

## What was measured

Edge width from the steepest slope of a horizontal scanline through the line, in
linear light — the blend happens in linear light, and measuring sRGB bytes makes
every soft edge look wider than it is. A 10-90% pixel count was tried first and
**was wrong**: it cannot resolve below one pixel and reported 2.00 px for
everything, the compositor's own title-bar text included.

| | edge width |
|---|---|
| The compositor's own desktop text, same screenshot | 1.72 px |
| This engine's text, smoothing forced off — the pixel grid's floor | 1.47 px |
| This engine's text as this card ships it | **1.73 px** |
| This engine's text before this card | 2.16 px |

**The shipped text has the same edge width as ordinary desktop text**, and is
0.26 px off the sharpest a pixel grid can represent. It is scale-invariant:
letters filling half the screen measure the same as the heads-up line, which is
the distance field doing exactly what it is for.

## Why it looked soft anyway, and what the panel proves

Pale blue on teal is 12.8:1 in linear luminance. On the panel it is 18.5:1, and
**the edge width is unchanged — 1.72 px against 1.73** — so the panel moves
contrast and nothing else. That is the whole of the difference a person sees
between the two.

**What remains is pixel snapping, which this card and `text/include/text/font.h`
both refuse by name.** Desktop text is hinted and snapped so its stems land on
pixel boundaries; a stem here lands wherever a perspective camera puts it, which
is usually across two pixels. That reads as softer than a hinted stem of the same
edge width and no change to the sheet or the shader affects it.

## What was changed, and what was not

**Only `dev`.** One dark opaque quad in the overlay layer behind the heads-up
line, placed each frame from the same camera basis the line uses, one frame
behind in the same way and for the same reason. `add_quad` now hands its entity
back — `NULL` for the five that are placed once and never moved — and
`add_the_quads` hands back the square every quad shares, so the panel wears the
same geometry rather than a second copy of it.

**No change to `render`, `text`, the shader or the sheet.** Nothing about the
card's own claims moved, and `cmake -P check.cmake` still exits zero.

**There is no key to toggle the panel.** Every key `voe_platform_key` names is
already bound, so a toggle means adding one to `platform` — a different folder,
and not something to take on the way past. `BLOCKED: platform, no unbound key
for a heads-up panel toggle` if it is ever wanted.

## Superseded the same day by card 026 — read that section with this in mind

**`VOE_RENDER_FIELD_PIXELS` no longer exists.** The principal decided, after
seeing the measurements below, that this engine wants no antialiasing anywhere;
card 026 replaced the smoothstep with `step(0.5, field)` and removed the
constant. The table below is still the honest record of what the trade was worth
when there was one, and it is what the decision was taken on — but nothing in the
code answers to it now.

What survives unchanged is everything above this line: the sheet, the three
channels, the corner reconstruction and the sampling mode. Card 026's own notes
say why the field is what makes a hard cut look like a letter rather than blocks.

## The tuning was a named constant, and the trade was measured

`VOE_RENDER_FIELD_PIXELS` in `render/shaders/draw.slang` was how many screen
pixels the edge was smoothed over, and it shipped at 1.0.

| setting | edge width |
|---|---|
| 1.0 — shipped | 1.72 px |
| 0.7 | 1.62 px |
| smoothing off entirely | 1.47 px |
| the desktop's own hinted text, same screenshot | 1.72 px |

**The whole range is a quarter of a pixel and the bottom of it is not free.**
Turning it off was a bisection step during the follow-up, not a candidate: a
fixed width is some fraction of a *texel*, and a texel is a different number of
pixels at every distance, so it looks right at one size and aliases at every
smaller one. The dev sign at distance breaks into disconnected specks that pop as
the camera moves — the exact failure this card exists to prevent, and the reason
the width is taken from the derivative in the first place. Below about 0.7 that
is visible on the sign.

The file header said all of this at the constant. Card 026 then removed the
constant, and the header now says why a hard cut is the rule and what it costs.
