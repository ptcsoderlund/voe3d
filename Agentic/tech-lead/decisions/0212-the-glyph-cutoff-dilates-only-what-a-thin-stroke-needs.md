# 0212 — The glyph cutoff dilates only as far as a thin stroke needs, and not at all when strokes are wide

date: 2026-09-22
by: planner

## Decision
The element pipeline's glyph cutoff (`render/shaders/elements.slang`,
`voe_render_element_cutoff`) keeps 0184's shape but its dilation is no longer a fixed half pixel.
With `field_per_pixel = max(texels_per_pixel.x, texels_per_pixel.y) / (2 * spread)` as in ADR-0183:

- `thin_stroke_pixels = VOE_RENDER_ELEMENT_THIN_STROKE_TEXELS / max(texels_per_pixel.x, texels_per_pixel.y)`
- `dilation = clamp(0.5 * (1.25 - thin_stroke_pixels), 0.0, 0.5)`, in screen pixels
- `cutoff = clamp(0.5 - dilation * field_per_pixel + 1.0 / 255.0, 1.0 / 512.0, 0.5)`

`VOE_RENDER_ELEMENT_THIN_STROKE_TEXELS` is 2.0: the thinnest stroke the sheet holds, in sheet
texels, at `text/src/font.c`'s `ATLAS_EM` of thirty-two texels to the em. It is a third copied
constant beside the field's spread, and moves with `ATLAS_EM` and with the face.

Everything else of 0183 and 0184 stands: hard `step`, no partial coverage, derivatives of the sheet
coordinate taken before the kind branch, the byte-step guard, both clamps, world text in
`draw.slang` unchanged.

## Reasoning
0184's dilation is half a screen pixel whatever the text's size, so a stem comes out about one
screen pixel wider than the design everywhere. On the sponsor's laptop, whose panel puts more
pixels into the same letter, that pixel is a small share of a stem; on the desktop monitor at
1920x1080 a stem is about two and a half pixels and the extra one reads as bold — bug 05 of 009
measured four pixels at a cap height of twenty-three, identically on Windows and under WSL, so the
weight is in the drawing and the display is what differs.

A stroke only risks losing all its pixels when it is under a screen pixel across, which happens
only when text is small. So the dilation is spent where it is needed: the face's thinnest stroke is
two texels at `ATLAS_EM`, which is `2 / texels_per_pixel` screen pixels, and the cut moves out only
far enough to bring that to a pixel and a quarter. At about one texel per pixel — the desktop
editor — the thinnest stroke is already two pixels, the dilation is nought, and the letters come out
at the face's own weight. Below about 1.6 texels per pixel the dilation grows, reaching 0.47 pixel
at four texels per pixel, which is where 0184 already sat and where bug 03's strokes vanished.

A pixel and a quarter rather than exactly one: `step` counts a centre exactly on the dilated edge as
inside, but the byte-step guard eats a little of the dilation back, and the constant is the face's
thinnest stem and not a measurement of the stroke under this fragment. The quarter is the margin for
all three. Two texels rather than Oxanium's stem of about two and a half (0.08 em at thirty-two
texels to the em): the field cannot hold a feature under about one and a half texels at all
(`text/src/font.c`), so two is the floor worth protecting and it keeps a little dilation at the
middling sizes where a real feature is thinner than the stem.

Rejected: rediscovering the stroke's width per fragment from the median, which flattens on a thin
stroke's ridge — the reason 0183 takes the derivative from the sheet coordinate; and returning to
the plain 0.5 crossing, which is bug 03.

## Replaces
0184's fixed `0.5 * field_per_pixel` dilation. The rest of 0184, and 0183, stand.
