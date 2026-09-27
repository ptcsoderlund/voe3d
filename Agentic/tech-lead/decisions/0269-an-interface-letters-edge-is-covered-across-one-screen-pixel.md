# 0269 — An interface letter's edge is covered across one screen pixel
date: 2026-09-27
by: planner

## Decision
For 034: the element pipeline's glyph (`render/shaders/elements.slang`) stops thresholding the
field with `step` and takes a coverage between nought and one, multiplied into the record's alpha.

- `field_per_pixel` and `dilation` stay exactly as ADR-0183 and 0212 define them, so the edge sits
  where 0212 put it: `edge = 0.5 - dilation * field_per_pixel`. A thin stroke still carries about a
  pixel and a quarter of ink; where strokes are wide the edge is the outline and the weight is the
  face's.
- The coverage is a linear ramp one screen pixel wide centred on that edge:
  `saturate((median - low) / (high - low))` with `low = max(edge - 0.5 * field_per_pixel, 1/255)`
  and `high = max(edge + 0.5 * field_per_pixel, low + 1/255)`. A texel encoded as nought is never
  covered however small the text; a derivative of nought cannot divide by nought.
- 0184's byte-step guard and 0212's clamps on the cutoff go: they existed to keep a hard cut exact
  and a ramp is not decided within a byte.
- Blending is the premultiplied blend the element pipeline already has (ADR-0069); nothing in
  `render/src` changes. `draw.slang`'s world text keeps its hard `step(0.5, …)`.

## Reasoning
034 asks for smooth letters at every size, editor and game alike, and for large text as sharp as
now, nothing moved. Both interfaces draw their text through the element pipeline (0259), so this
one function is the whole fix. A box filter over one pixel is what a distance field gives for free:
the field is a distance, so its value at the pixel centre over the field per pixel is how far the
centre is from the edge in pixels. At one texel per pixel a pixel centre half a pixel outside an
aligned outline reads about one per cent coverage, so large text stays sharp. The ideas list's
recommendation, which the sponsor took into 034, was this; supersampling (an offscreen texture and
a downscale per text) was rejected there on cost. Keeping 0212's dilation keeps a thin stroke dark
instead of letting it fade to a faint grey, which is what "as readable as its size allows" needs.
World text is not in 034's scope and has no caller that shows it small.

## Replaces
0182's hard edges for interface text (its scaling and its "no stroke vanishes" stand), and the
`step` and the byte-step guard of 0183, 0184 and 0212 on the element path; their dilation stands.
ADR-0078's "no anti-aliasing anywhere" no longer covers interface text.
