# 0183 — A glyph's screen pixel counts when its centre is within 0.49 of a pixel outside the outline
date: 2026-09-19
by: planner

## Decision
The element pipeline's glyph cutoff (`render/shaders/elements.slang`) is no longer the field's 0.5
crossing. It is moved outward by 0.49 screen pixels, measured per fragment from the screen-space
derivative of the sheet coordinates: `cutoff = 0.5 - 0.49 * field_per_pixel`, where
`field_per_pixel = max(texels_per_pixel.x, texels_per_pixel.y) / (2 * spread)`, spread being the
field's encoded range in texels (4, `VOE_TEXT_FIELD_SPREAD`). The cutoff is clamped so a texel
encoded as 0 is never inside. Coverage stays `step`, hard, no partial pixel. The derivatives are
taken before the kind branch, in uniform control flow. World text in `draw.slang` is not changed.

## Reasoning
Decision 0182 asks for a cutoff under which no stroke loses all its pixels. A stroke of any width
w, dilated by d on each side, covers a pixel centre across it whenever w + 2d >= 1 pixel; at
d = 0.49 that holds for every stroke wider than 0.02 of a pixel, i.e. every stroke of both fonts
at any window height the editor reaches. d just under one half keeps an outline that falls exactly
on pixel boundaries (Pixel Operator at a whole-number scale, full screen) at its true width, since
the nearest outside centre is 0.5 away; strokes that do not line up come out at most one pixel
wider, which bug 01 of 009 accepts. The texel-to-pixel ratio comes from the sheet coordinates and
not from the median's own derivative, because the median's slope flattens on the ridge of a thin
stroke, exactly where the dilation is needed. The editor's panels are the whole scope of 0182, so
the world-text copy of the threshold stays as it is.

## Replaces
nothing
