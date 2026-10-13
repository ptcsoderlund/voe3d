# 0408 — The sky's fill lights only the shade, and its reflection is whole
date: 2026-10-12
by: planner

## Decision
For 087 bug 02.

1. The sky's diffuse term is light 0's fill under a sky and fades as that fill does (0276): times
   `fill` and times light 0's fill weight, `1 − smoothstep(0, 0.3, reach)`, its reach N·L times its
   shadow, nought where its gate stops its direct light. A surface in light 0's full light gets none of
   it, whatever `fill` is.
2. The sky's reflection is the sky's look, not a fill: it is neither scaled by `fill` nor faded by the
   weight. It stays gated by `voe_render_sky_reaches` and occluded as before, and is nought with
   `drawn` 0.
3. Without a drawn sky nothing changes: every light's fill already fades by its own weight (0276).

## Reasoning
The human wants fill to lift only what the sun does not reach; 0407 point 7 added the whole sky term
times `fill` everywhere, so raising fill brightened sunlit faces too. Fading the reflection as well would
leave a metal sphere's sunlit half without the sky it should mirror (How to test step 6), and scaling it
by `fill` would make a shiny sunlit face change with fill; a reflection is what the sky looks like, so it
is whole wherever the sky reaches.

## Replaces
Amends 0407 point 7 and 0406 point 3: the sky term × `fill` becomes the diffuse half × `fill` × light
0's fill weight, the reflection unscaled.
