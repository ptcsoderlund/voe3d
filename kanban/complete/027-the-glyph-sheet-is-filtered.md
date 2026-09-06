# 027 — the glyph sheet is filtered

status: complete
claimed-by: claude-opus-5 (kanban-coder)
blocked-by: -

Written by the tech lead under the standing grant. **Not a spin-off**: it is not
carved out of 026, it is the card that follows it, so it takes the next number
rather than a letter.

**This card is owed to a defect the principal found by looking at an O.** Card 026
removed antialiasing engine-wide on his direct instruction and six of its seven
rows are correct and stay. The seventh gave the glyph sheet `VK_FILTER_NEAREST`
along with every picture in the engine, and a point-sampled distance field is not
a distance field. The decision behind this card is ADR-0079; everything you need
is restated below, so you should not have to go and read it.

## Goal

The glyph sheet is sampled with linear filtering, from one level, so the shader's
threshold lands on the letter's outline instead of on the nearest texel boundary.

**Nothing else in the engine gains a filter.** No mipmap chain comes back
anywhere, and no picture is filtered.

## The one thing to understand before starting

**Read this before you touch the sampler, because the change looks like exactly
the thing card 026 forbade and is not.**

The sheet holds three signed distances per texel, at 32 texels to the em
(`ATLAS_EM`, `text/src/font.c:131`). The shader takes their median and thresholds
it — `render/shaders/draw.slang:461-462`:

```
float field = voe_render_median(sheet.rgb);
float field_alpha = step(0.5, field);
```

**`step` returns 0.0 or 1.0 and there is no third value.** The filter runs before
the threshold and the threshold keeps only the sign, so no sampler setting can
produce a partially covered pixel. **There is no code path from this change to a
soft edge**, and the 1995 look card 026 bought is not being negotiated down. If
you find yourself able to produce a grey edge pixel, something else is wrong and
you should stop and report it.

What the filter changes is *where* the boundary falls, not how hard it is:

- **Point-sampled**, the field is a flat plateau across each texel, so the 0.5
  crossing can only fall on a texel boundary. A stem two and a half texels wide
  rounds to two or to three depending on its phase against the grid — which is
  the defect: an O whose left stem is three units and right stem two, at every
  distance including when the letter fills the screen.
- **Filtered**, the crossing falls where the distances say it falls, so the stem
  comes out two and a half texels wide and both sides of the O match.

Card 026's own section *What card 025 turns out to have been for* makes this
argument correctly and is worth reading: it says a hard cut through a field puts
the edge on the outline at any size, where a hard cut through a coverage atlas
*"would have put the edge on the nearest texel and the same letter would have
come out in blocks the size of the sheet's texels."* That is true only while the
field is interpolated. Restoring the interpolation is what makes the sentence
true.

**Interpolating a distance is not blurring a picture.** Between two colours it is
a blur; between two distances it is reconstruction of where the edge is. That
distinction is the whole rule, and it is the sentence to leave in the code for
whoever sweeps filtering out next.

## Scope — `render`

**Add a third sampling mode. Do not change the two that exist.**

- `render/include/render/device.h`, the `voe_render_sampling` enum (around line
  223). Add `VOE_RENDER_SAMPLING_FIELD` **after** `SHARP`, so `SMOOTH` stays 0 —
  the header states that a caller who meant nothing in particular gets 0, and that
  is load-bearing.
- **The two existing doc comments are wrong and card 026 left them that way.**
  They currently describe `SMOOTH` as linear over a generated mipmap chain and
  `SHARP` as *"Linear magnification and minification of one level"*. Neither is
  what the code does — both are `NEAREST`. Correct both while you are in here, and
  write the new one to say what it is for: **one level, linear, for texture data
  that is numbers rather than a picture**, with the reason on it.
- `render/src/device_internal.h:68` — `VOE_RENDER_SAMPLING_COUNT` is a
  hand-written `#define 2`, not derived from the enum. It must become 3. It sizes
  `samplers[]` at `device_internal.h:392` and the `infos[]` array at
  `texture.c:498`, and the creation and destruction loops at `texture.c:543` and
  `texture.c:618` run to it.
- `render/src/texture.c`, the `infos[]` initialiser (around line 498). Add the
  third entry: `magFilter` and `minFilter` `VK_FILTER_LINEAR`, `mipmapMode`
  `VK_SAMPLER_MIPMAP_MODE_NEAREST`, `minLod` and `maxLod` both 0, address modes
  `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE` on all three axes, anisotropy off.
  **One level. No chain is generated, uploaded or sampled, and none of the mipmap
  code card 026 deleted comes back.**
- **The comment block above that initialiser is the one to rewrite**, not just
  extend. It currently reads *"this engine has no antialiasing, and a linear filter
  and a mipmap chain are both antialiasing"*, which is the sentence that produced
  the defect. It is true of a picture and false of a distance field. Say both
  halves.

Per-slot selection already works — `texture.c:318` writes
`device->samplers[slot->sampling]` into the descriptor and `texture.c:400` stores
the slot's mode — so this is additive and no call site changes except the one
below.

## Scope — `text`

- `text/src/font.c:386` — the glyph sheet asks for `VOE_RENDER_SAMPLING_FIELD`
  instead of `SHARP`. The texture format stays `VOE_RENDER_TEXTURE_DATA` and must
  not change: distances uploaded as colour get the sRGB decode run over them and
  every edge in the font lands slightly wrong, uniformly, which reads as the
  spread being mistuned rather than as a format mistake.
- The file header comment at `font.c:48-55` currently ends *"Neither says anything
  about filtering: this engine samples every texture NEAREST with no mipmaps,
  sheets included."* That sentence is now false for this sheet and it is the one a
  future reader will trust. Replace it with what the mode means and why the sheet
  is the exception.

## What must not change

State in your report that you checked each of these, because every one of them
looks adjacent to the change and none of it is in scope:

- **`step(0.5, field)` stays.** No `smoothstep`, no `fwidth`, no derivative. The
  hard cut is what the principal asked for and this card does not revisit it.
- **`ATLAS_EM` stays at 32.** Raising it was considered and rejected: it shrinks
  the error without removing it, costs sheet memory as the square, and restores
  the resolution-guessing the distance field exists to end. **If you are tempted
  to tune a constant to make the O look better, that is this card failing.**
- **`SMOOTH` and `SHARP` stay `NEAREST`**, and every existing caller of them —
  `3d/src/import.c:149` and `:158` among them — is untouched and still correct.
- **No mipmap chain, anywhere.** `mip_levels_for`, `can_generate_mipmaps` and
  `generate_mipmaps` stay deleted.
- **The end-of-frame blit stays `VK_FILTER_NEAREST`** (`render/src/frame.c:470`).
- **`VOE_TEXT_RASTER_SAMPLES` and `voe_text_raster_fill` stay untouched**, as card
  026 left them.
- **Nothing is renamed.** The enum's values now describe combinations across two
  axes and one combination has no name; that is a known question and an API change,
  and it is not this card's.

## Where this card is likely to go wrong

- **`VOE_RENDER_SAMPLING_COUNT` is a hand-maintained 2.** Forget it and the third
  designated initialiser writes past the array. `-Werror` should catch it; do not
  rely on that being the thing that tells you.
- **Bilinear reads a 2×2 neighbourhood, so a sample near a glyph's edge now
  touches texels the point sample never did.** The packing puts `ATLAS_MARGIN`
  texels — the spread — on every side of each glyph and between shelves
  (`text/src/font.c:141`, `:255-266`), and the quad's UVs are computed to include
  that margin (`:305-308`), so it looks structurally sound. **Confirm it rather
  than assuming it**, and say in your report what you confirmed and how. A failure
  here is one glyph faintly bleeding into its neighbour at extreme magnification,
  which is easy to miss and hard to explain later.
- **A distance field must not be premultiplied and nothing here changes that.**
  The shader multiplies once, at the end, by an alpha this sheet does not supply.
- **The median is taken after interpolation, and that is correct** — it is what
  the three channels are for. Do not try to interpolate a median.

## Verify

- `cmake -P check.cmake` exits zero, all steps, all tests, analyser clean.
- **Screenshot the letter O, magnified until it fills much of the screen.** Its
  two vertical stems must be the same width as each other. This is the whole
  point of the card and it is the image the principal will look at.
- **Screenshot the same O before your change as well**, so the pair can be
  compared. The defect is plainly visible at that magnification.
- **Confirm the edge is still a hard cut** at that magnification: the steps are
  one screen pixel each and there are no grey pixels along the edge. If you see
  softening, stop and report rather than tuning.
- **Screenshot the dev scene's cube.** Its texture must still show its texels
  close up, unfiltered, exactly as card 026 left it. This is the check that the
  exception did not leak.
- **Look at the dev sign at distance.** Small text still breaking into specks is
  expected and is not this card's to fix; note what you see so the open question
  about small text gains an observation rather than a worry.
- Windows is the principal's.

## Report when this lands

- What the O looked like before and after, in words as well as screenshots.
- What you confirmed about the packing margin against the 2×2 read, and how.
- Whether anything in the engine other than the glyph sheet ended up filtered.
  The answer should be no.
- Anything you found in the comments of the four files above that still argues for
  the state before card 026 or before this card. Two rounds of sweeping have left
  comments describing code that changed underneath them, and a third reader
  deserves better than we have given.

## Done

`render` — `VOE_RENDER_SAMPLING_FIELD` added **after** `SHARP`, so `SMOOTH` is
still 0. `VOE_RENDER_SAMPLING_COUNT` is 3. The third `infos[]` entry is
`VK_FILTER_LINEAR` both ways, `MIPMAP_MODE_NEAREST`, `minLod`/`maxLod` 0,
`CLAMP_TO_EDGE` on all three axes, anisotropy off. `SMOOTH` and `SHARP` are
untouched and still `NEAREST`.

`text` — the glyph sheet asks for `FIELD`. Format is still
`VOE_RENDER_TEXTURE_DATA`.

Comments rewritten, not extended: the `infos[]` block in `texture.c`, both value
doc comments and the block above the enum in `device.h`, and the sampling
paragraph of the `font.c` header. `render.md` and `text.md` follow.

## What the O looked like

Camera parked so one letter of the sign's `VOE3D` filled the frame, the same
placement for both builds. Measured on the captured frame, three scanlines
through the bowl, all three agreeing:

| | left stem | right stem |
|---|---|---|
| before (`SHARP`) | 51 px | 35 px |
| after (`FIELD`) | 44 px | 44 px |

Before, the O is plainly lopsided — a heavy left side and a thin right one — and
its outline is a staircase of texel-sized blocks; the V beside it is a chunky
diagonal. After, the two stems are the same width to the pixel and the outline
is a smooth curve with one-pixel steps. This is the defect and it is gone.

## The edge is still a hard cut

The captured window is the compositor's 1.25x upscale of the 960x540 surface, so
a blended pixel appears at every edge in **both** builds. The control settles
it: across the O's left outer edge the ramp is byte-for-byte identical before and
after — `10 -> 132 -> 255`, one intermediate, and 132 is the exact 50/50 blend of
the two. The point-sampled build cannot soften anything, so that pixel is the
compositor and not the engine. No grey was introduced. `step(0.5, field)` is
untouched.

## The packing margin against the 2x2 read

Confirmed rather than assumed, by reading the packing arithmetic and the
encoding together:

- A bilinear sample at the quad's own UV edge lands exactly on a texel boundary,
  so it reads **one** texel outside the glyph's box on that side.
- `ATLAS_MARGIN` is `VOE_TEXT_FIELD_SPREAD` = 4 texels. `add_glyph` advances
  `shelf->x` by `width + ATLAS_MARGIN` and `shelf->y` by
  `shelf->height + ATLAS_MARGIN` (`font.c:255-266`, `:314`), so four blank texels
  separate every glyph box from the next one across and every shelf from the one
  below. The one texel the filter reaches into is a gap texel, never another
  glyph's.
- Those gap texels are not undefined: the sheet is cleared to RGB 0 before
  packing (`font.c:363-365`), and `encode` maps 0 to a distance of
  `-VOE_TEXT_FIELD_SPREAD`, fully outside (`raster.c:648-659`). The box's own
  outermost texel is at that same extreme, so the interpolation between them
  stays far below the 0.5 threshold and cannot produce ink.
- At the sheet's own borders there is no texel to read; `CLAMP_TO_EDGE` repeats
  the glyph's own outer margin texel, which is also fully outside.

No bleeding is possible at any magnification.

## Nothing else in the engine is filtered

Every call site was enumerated. `3d/src/import.c:149` and `:158` pass `SMOOTH`,
`dev/src/main.c:852` passes `SMOOTH`, `render/tests/offscreen.c:646` and
`render/tests/pools.c:168` pass `SMOOTH`, `render/tests/pools.c:203` passes
`SHARP`. `text/src/font.c:396` is the only `FIELD` in the tree. `SMOOTH` and
`SHARP` are both still `VK_FILTER_NEAREST`, the end-of-frame blit at
`render/src/frame.c:470` is still `VK_FILTER_NEAREST`, and `mip_levels_for`,
`can_generate_mipmaps` and `generate_mipmaps` are still absent.

Checked and untouched, each of them: `step(0.5, field)`, `ATLAS_EM` at 32,
`VOE_TEXT_RASTER_SAMPLES`, `voe_text_raster_fill`, and every name in the enum.

The dev scene's cube was screenshotted close up: its texture still shows flat
texel blocks with hard boundaries. The exception did not leak.

## The sign at distance

Looked at from the default orbit. The three lines break into disconnected
yellow specks with the line structure barely readable — no worse than before the
change, and no better. Filtering the field does not help here because the
failure is the threshold, not the reconstruction: at that size one screen pixel
spans several texels and the median lands either side of 0.5 almost at random.
Recorded as an observation, not touched.

## Stale comments still in the tree

Not fixed, because the card asked for them to be reported rather than repaired:

- **`text/src/font.c:42-44`** — the header still says what draws the sheet
  "works out its own alpha from how fast that median moves across one screen
  pixel". That is the `fwidth` reader from before card 026; the shader now takes
  a `step` and no derivative. It is the only sentence left in these four files
  arguing for the pre-026 state, and it sits three lines above the paragraph this
  card rewrote.
- **`render/render.md`** — "why the sampling modes' names now say more than they
  mean" is still true and stays, but the enum's names are now wrong in a second
  way as well: with `FIELD` present, `SHARP` is the one mode that is neither
  smooth nor sharp nor filtered. That is the known API question the card put out
  of scope, and it has got slightly worse rather than better.

## Verified

Linux, `cmake -P check.cmake` **exits zero** — every step, 31 tests, the analyser
over 86 files with no finding. Windows is the principal's.

Screenshots: the magnified O before and after, the dev scene from the orbit, and
the cube's texture close up.

To take the before/after pair from one fixed camera, `dev/src/main.c` was given a
temporary `getenv` in `orbit()` that parked the camera, and the sheet was flipped
back to `SHARP` for one build. Both were reverted; `git diff -- dev/` is empty
and `text/src/font.c` asks for `FIELD`. `check.cmake` above was run after the
revert.

**No `DEVIATION:` and no `BLOCKED:` markers.**
