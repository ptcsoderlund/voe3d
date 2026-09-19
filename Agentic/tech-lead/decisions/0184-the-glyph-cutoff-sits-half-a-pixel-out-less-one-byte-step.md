# 0184 — The glyph cutoff sits half a screen pixel out, less one byte step of the field
date: 2026-09-19
by: planner

## Decision
The element pipeline's glyph cutoff (`render/shaders/elements.slang`, `voe_render_element_cutoff`)
is `0.5 - 0.5 * field_per_pixel + 1/255`, clamped to at most 0.5 and at least 1/512, where
`field_per_pixel = max(texels_per_pixel.x, texels_per_pixel.y) / (2 * spread)` as in ADR-0183.
Everything else in 0183 stands: hard `step`, derivatives of the sheet coordinates taken before the
kind branch, world text in `draw.slang` unchanged.

## Reasoning
0183's fixed 0.49 pixel left 0.01 pixel of room between the cutoff and the nearest outside pixel
centre of an aligned outline. The field is stored as bytes (rounded, `text/src/raster.c`), so a
value can sit up to half a byte step (0.00196) above its true value; at one texel per pixel 0.01
pixel is 0.00125 in field, less than that, and an 8-texel bar on pixel boundaries drew 10 rows
(card 07 of 009, blocked). Any fixed fraction fails at some magnification, because the byte error
is constant in field while a pixel's worth of field shrinks as the text grows. Guarding with one
whole byte step in field instead keeps an outline on pixel boundaries at its true width at every
scale (margin half a byte step over the worst rounding). The dilation it leaves is
`0.5 - (1/255) / field_per_pixel` pixel: 0.492 at four texels per pixel, where strokes are thin,
so no stroke loses all its pixels (ADR-0182); it only shrinks when text is magnified, where every
stroke is several pixels wide. The upper clamp stops the cut ever eroding past the outline.

## Replaces
0183's `0.49 * field_per_pixel` margin; the rest of 0183 stands.
