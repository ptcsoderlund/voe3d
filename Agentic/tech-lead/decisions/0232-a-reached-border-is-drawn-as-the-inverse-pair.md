# 0232 — A reached border is drawn as the inverse pair, and the root names it
date: 2026-09-23
by: planner

## Decision
For 022's bug 02, carrying out 0231 in `editor`. Every dock split's seam is drawn, no longer a gap: at rest
one fill of the palette's `border` the seam's full size; while it is the border the pointer hovers or holds,
the two stripes 0230 used, `inverse_ink` on the first child's side and `inverse` on the second's. Which
border that is comes from `resize.c` as a node index on its frame's result, and reaches the walk as a field
of `voe_editor_dock_root` that `main.c` fills every frame; UINT32_MAX (or the bar's index) lights none.
The seam keeps its size and place, so the grab band is unchanged. The top bar's lower edge is not a seam
and is left as it is.

## Reasoning
The inverse pair is legible by construction and on the theme's one hue (0194, 0196), and two stripes of
opposite lightness stand out against any picture, dark or light, in every theme including monochrome ones;
one colour cannot. The root already carries the frame's pointer and keyboard as caller-filled data, so the
reached border travels the same way and `interface.c` passes it through untouched.
- A single `inverse` fill when reached: vanishes against content of that lightness.
- A new parameter on `voe_editor_interface_draw` and the walk: more signatures for one number.

## Replaces
Nothing; carries out 0231.
